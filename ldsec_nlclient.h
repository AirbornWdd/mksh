
#ifndef LDSEC_NLCLIENT_H
#define LDSEC_NLCLIENT_H

#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/socket.h>

#define TRACE_RECV_BUF_LEN      4096
#define TRACE_MAX_MSG_LEN		1056

/* Types of messages */
enum {
	TRACE_MSG_BASE = 1024, 
	TRACE_MSG_TESTSELF = 1024,
	
	TRACE_MSG_SETTUPLE,
	TRACE_MSG_GETTUPLE,
	TRACE_MSG_ONOFFMATCH,
	TRACE_MSG_GETINFO,
	TRACE_MSG_COREID,
	TRACE_MSG_MAC,
	TRACE_MSG_L3PROTO,
	TRACE_MSG_DEV,
	TRACE_MSG_VLANID,
	TRACE_MSG_PROBETIME,
	
	TRACE_MSG_GETVAR,
	TRACE_MSG_SETVAR,
	TRACE_MSG_SETSNIFF,
	TRACE_MSG_GETCTRL,	
	TRACE_MSG_SETCTRL,
	TRACE_MSG_SETTHRESH,
	TRACE_MSG_GETMEMADDR,
	TRACE_MSG_HOOK_BYPASS,
	TRACE_MSG_DROPSTAT,
	TRACE_MSG_GET_GFLAG,
	TRACE_MSG_SET_DPTIMES,
	TRACE_MSG_GET_DPTIMES,
	TRACE_MSG_GET_DPINFO,
	TRACE_MSG_GET_MEMUSG,
	TRACE_MSG_GET_PRSTACK,
	TRACE_MSG_LOGX_BYPASS,
	
	_TRACE_MSG_MAX,
};

#define TRACE_MSG_MAX  (_TRACE_MSG_MAX - 1)
#define TRACE_NR_MSGTYPES (TRACE_MSG_MAX - TRACE_MSG_BASE + 1)

typedef struct trace_msg {
	__u8	cmd;
	__u8	version;
	__u16	reserved;
	__u8 data[TRACE_MAX_MSG_LEN];
} trace_msg_t;

/* multicast groups */
enum trace_groups {
	TRACE_GRP_NONE,

	
	_TRACE_GRP_MAX,
};
#define TRACE_GRP_MAX (_TRACE_GRP_MAX - 1)

#define NETLINK_LDSEC_TRACE 28 /*debug and  trace  */

#define TRACE_DEBUG
#ifdef TRACE_DEBUG
#define PRINT	printf
#define TRACE() \
do{\
        PRINT("[%s:%d] \n\n", __FUNCTION__, __LINE__);\
}while(0)

#define TRACE_ARGS(format, ...) \
do{\
    PRINT("[%s:%d] ", __FUNCTION__, __LINE__);\
    PRINT(format, ##__VA_ARGS__);\
}while(0)

#else
#define TRACE()
#define TRACE_ARGS(format, ...)
#endif


/*netlink socket control structure. */
typedef struct nlsock
{
	int sock;
	int s_proto;
	unsigned int seq;
	struct sockaddr_nl	s_local;
	struct sockaddr_nl	s_peer;	
	char *name;
} nlsock_t;

typedef int (*dump_filter_func)(char *, int, void*);
nlsock_t *alloc_nlsock(const char *nlsock_name);
void free_nlsock(nlsock_t *sk);
int nlsock_connect(nlsock_t *sk, int protocol, unsigned long groups);
int nlsock_kernel_comm(nlsock_t *sk, int type, char *buff, int len, 
	int flags, void *filter_param, dump_filter_func filter);
void request_comm(int type, char *buff, int len);

#endif /* LDSEC_NLCLIENT_H */

