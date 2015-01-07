#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <errno.h>
#include "ldsec_nlclient.h"

nlsock_t *alloc_nlsock(const char *nlsock_name)
{
	nlsock_t *sk;
	size_t slen;

	sk = calloc(1, sizeof(*sk));
	if (!sk )
		return NULL;

	sk->sock = -1;
	sk->s_local.nl_family = AF_NETLINK;
	sk->s_peer.nl_family = AF_NETLINK;
	sk->seq = 0;
	sk->s_local.nl_pid = getpid();
	if (nlsock_name && (slen = strlen(nlsock_name)))
		sk->name = strndup(nlsock_name, slen);

	return sk;
}

void free_nlsock(nlsock_t *sk)
{
	if (!sk)
		return;
	
	if (sk->name)
		free(sk->name);
	
	if (sk->sock >= 0)
		close(sk->sock);
	
	free(sk);
}

/**
 * Create file descriptor and bind socket.
 * @arg sk		Netlink socket (required)
 * @arg protocol	Netlink protocol to use (required): NETLINK_LDSEC_TRACE
 * @arg groups    Netlink multicast groups
 *
 * Creates a new Netlink socket using `socket()` and binds the socket to the
 * protocol. Fails if  the socket is already connected.
 */
int nlsock_connect(nlsock_t *sk, int protocol, unsigned long groups)
{
	int err;
	socklen_t addrlen;

	if (sk->sock != -1)
		return -EEXIST;

	sk->sock = socket(AF_NETLINK, SOCK_RAW, protocol);
	if (sk->sock < 0) {
		err = -errno;
		TRACE_ARGS("%s:%i socket error: %s\n", 
			__FUNCTION__, __LINE__, strerror (errno));		
		goto errout;
	}	

	err = fcntl (sk->sock, F_SETFL, O_NONBLOCK);
	if (err < 0) {
		err = -errno;
		goto errout;
	}

	err = fcntl(sk->sock, F_SETFD, FD_CLOEXEC);
	if (err < 0) {
		err = -errno;
		goto errout;
	}

	sk->s_local.nl_groups = groups;
	err = bind(sk->sock, (struct sockaddr*) &sk->s_local,
		   sizeof(sk->s_local));	
	if (err < 0) {
		err = -errno;
		TRACE_ARGS("%s:%i bind error: %s\n", 
			__FUNCTION__, __LINE__, strerror (errno));			
		goto errout;
	}	

	addrlen = sizeof(sk->s_local);
	err = getsockname(sk->sock, (struct sockaddr *) &sk->s_local, 
		&addrlen);
	if (err < 0 ) {
		err = -errno;
		TRACE_ARGS("%s:%i getsockname error: %s\n", 
			__FUNCTION__, __LINE__, strerror (errno));			
		goto errout;
	}
	if (addrlen != sizeof(sk->s_local)) {
		/* EFAULT: bad address.  */
		err = -EFAULT;
		goto errout;
	}
	if (sk->s_local.nl_family != AF_NETLINK) {
		err = -EPROTO;
		goto errout;
	}

	sk->s_proto = protocol;
	return 0;
errout:
	if (sk->sock != -1) {
		close(sk->sock);
		sk->sock = -1;
	}
	return err;
}

/**
  * Receive message from netlink interface and pass those information
  *   to the given function. 
  */
static int nl_parse_info(nlsock_t *sk, void *filter_param, 
	dump_filter_func filter, int ack)
{
	int status;
	int ret = 0;
	char recv_buff[TRACE_RECV_BUF_LEN];
	struct iovec iov = { recv_buff, sizeof recv_buff };
	struct msghdr msg = { (void*)&sk->s_peer, sizeof sk->s_peer, 
						&iov, 1, NULL, 0, 0};
	struct nlmsghdr *h;

	while(1) {
retry:		
		status = recvmsg (sk->sock, &msg, 0);
		if (status < 0){
			if (errno == EINTR)
				continue;
			if (errno == EWOULDBLOCK || errno == EAGAIN)
				break;
			continue;
		}	

		if (sk->s_peer.nl_pid != 0) {
			TRACE_ARGS("Ignoring non kernel message from pid %u\n",	
					sk->s_peer.nl_pid);
			continue;
		}	

		if (status == 0) {
			TRACE_ARGS("%s EOF\n", sk->name);
			return -1;
		}	

		if (msg.msg_namelen != sizeof(sk->s_peer)) {
			TRACE_ARGS("%s sender address length error: length %d\n",
				sk->name, msg.msg_namelen);
			return -1;
		}	

		for (h = (struct nlmsghdr *) recv_buff; NLMSG_OK (h, status); 
			h = NLMSG_NEXT (h, status)) {
			/* Finish of reading. */
			if (h->nlmsg_type == NLMSG_DONE) {
				ret = h->nlmsg_len - sizeof(struct nlmsghdr) ;			
				if(!ack) 
					return ret;
				else 
					goto retry;
			}

			/* Error handling. */
			if (h->nlmsg_type == NLMSG_ERROR) {
				struct nlmsgerr *err = (struct nlmsgerr *) NLMSG_DATA (h);
				/* If the error field is zero, then this is an ACK */
				if (err->error == 0) {
					/* return if not a multipart message */  
					if(!(h->nlmsg_flags & NLM_F_MULTI)) 
						return 0;    
					return ret; 
				}

				if (h->nlmsg_len < NLMSG_LENGTH (sizeof (struct nlmsgerr))) {
					TRACE_ARGS("%s error: message truncated.\n", sk->name);
					return -1;
				}
				return -1;
			}
			
			if(filter)
				ret = filter(NLMSG_DATA(h), 
					h->nlmsg_len - sizeof(struct nlmsghdr), filter_param);
		}	
		/* After error care. */
		if (msg.msg_flags & MSG_TRUNC) {
			TRACE_ARGS("%s error: message truncated.\n", sk->name);
			continue;
		}
		if (status) {
			TRACE_ARGS("%s error: data remnant size %d\n", sk->name, status);
			return -1;
		}		
	}
	return ret;
}

/**
 * Communicate between Userspace and Kernel with Netlink socket.
 * @arg sk		      Netlink socket (required)
 * @arg type 	      Message type (required): TRACE_MSG_SETTUPLE
 * @arg buff               Data buffer sendto of receive from Kernel  
 * @arg len                The length of data buffer  
 * @arg flags             Flags of Netlink  
 * @arg filter_param The parameter of dump filter function.
 * @arg filter             The dump filter function.
 * return value:
 *       < 0    failed
 *       >=0   success
 */
int nlsock_kernel_comm(nlsock_t *sk, int type, char *buff, int len, 
	int flags, void *filter_param, dump_filter_func filter)
{
	int ret, sock_flags, status = 0;
	struct req {
		struct nlmsghdr nlh;
		trace_msg_t data;
	} *msg;
	

	if (!sk)
		return -1;

	if (sk->sock < 0)
		return -1;
	
	if (len > sizeof(trace_msg_t)) 
		return -1;

	msg = calloc(1, sizeof(*msg));
	if (!msg)
		return -1;

	msg->nlh.nlmsg_len = sizeof(*msg) - sizeof(trace_msg_t) + len;
	msg->nlh.nlmsg_type = type;
	msg->nlh.nlmsg_flags = flags | NLM_F_REQUEST | NLM_F_ACK;
	msg->nlh.nlmsg_pid = getpid();
	msg->nlh.nlmsg_seq = ++sk->seq;
	if (buff)
		memcpy((void *)&msg->data, buff, len);

	ret = sendto(sk->sock, (void *)msg, sizeof(*msg) - sizeof(trace_msg_t) + len,
		0, (struct sockaddr *)&sk->s_peer, sizeof(sk->s_peer));
	if (ret < 0) {
		free(msg);
		return -1;
	}

	if ( msg->nlh.nlmsg_flags & NLM_F_ACK) {
		/*
		*Change socket flags for blocking I/O. 
		*This ensures we wait for a reply in nl_parse_info().
		*/
		if ((sock_flags = fcntl(sk->sock, F_GETFL, 0)) < 0) {
			TRACE_ARGS("F_GETFL error: %s\n", strerror (errno));
		}
		sock_flags  &= ~O_NONBLOCK;
		if (fcntl(sk->sock, F_SETFL, flags) < 0) {
			TRACE_ARGS("F_SETFL error: %s\n", strerror (errno));
		}

		/*
		* Get reply from netlink socket. 
		* The reply should either be an acknowlegement or an error.
		*/
		if(msg->nlh.nlmsg_flags & NLM_F_DUMP)
			status = nl_parse_info (sk, filter_param, filter, 0);
		else
			status = nl_parse_info (sk, NULL, NULL, 1);

		/* Restore socket flags for nonblocking I/O */
		flags |= O_NONBLOCK;
		if(fcntl(sk->sock, F_SETFL, flags) < 0) {
			TRACE_ARGS("F_SETFL error: %s\n", strerror (errno));
		}
	}
	
	free(msg);
	return status;
}

/**
 * Communicate between Userspace and Kernel with Netlink socket.
 * @arg sk		      Netlink socket (required)
 * @arg type 	      Message type (required): TRACE_MSG_SETTUPLE
 * @arg buff               Data buffer sendto of receive from Kernel  
 * @arg len                The length of data buffer  
 * @arg flags             Flags of Netlink  
 * @arg filter_param The parameter of dump filter function.
 * @arg filter             The dump filter function.
 * return value:
 *       < 0    failed
 *       >=0   success
 */
void request_comm(int type, char *buff, int len)
{
	nlsock_t *trace_nl;	
	int ret ;
	
	/* communication with kernel. */
	trace_nl = alloc_nlsock("trace_nlsock");
	if (!trace_nl) {
		TRACE_ARGS("alloc nlsock error.\n");
		goto out ;
	}	
	ret = nlsock_connect(trace_nl, NETLINK_LDSEC_TRACE, 0);
	if (ret < 0) {
		TRACE_ARGS("nlsock_connect failed.\n");
		goto out;
	}

	nlsock_kernel_comm(trace_nl, type, buff, len, 0, NULL, NULL);
	
	if (trace_nl) {
		free_nlsock(trace_nl);
		trace_nl = NULL;
	}
out:
	return;
}


