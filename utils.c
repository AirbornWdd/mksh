#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include "utils.h"
#include "command.h"

/**
  * checking  the mac address is valid?
  * @arg mac_addr     the mac address string
  * return value:
  *         < 0  : invalid address.
  *         = 0  : valid address.
  */  
int is_mac_valid (const char *mac_addr)
{
	int i;
	char temp[20];

	if (strlen(mac_addr) != MAC_LENGTH)
		return -1;

	strcpy(temp, mac_addr);
	strcat(temp, ":");
	for(i = 0; i < 6; i++) {
		if(!isdigit(temp[i * 3]) && 
			(toupper(temp[i * 3]) < 'A' || toupper(temp[i * 3]) > 'F'))
			return -1;
		
		if(!isdigit(temp[i * 3 + 1]) && 
			(toupper(temp[i * 3 + 1]) < 'A' || toupper(temp[i * 3 + 1]) > 'F'))
			return -1;
		
		if( (temp[i * 3 + 2] != MAC_SEP_1) && (temp[i * 3 + 2] != MAC_SEP_2) )
			return -1;
	}
	return(0);	
}

/**
  * checking  the mac address is valid and convert the 
  * mac address string to mac addr
  * @arg mac_addr  the mac address string
  * return value:
  *         < 0  : invalid address.
  *         = 0  : valid address.
  */ 
int macstr_to_macaddr (unsigned char *addr, char *src_mac)
{
	unsigned char *ptr = NULL;
	int num = 6;
	int total;
	unsigned char mac[20];

	memset (mac, 0, 20);
	strncpy ((char *)mac, src_mac, 20);
	mac[strlen ((char *)mac)] = '\0';
	ptr = (unsigned char *) strrchr ((const char *)mac, MAC_SEP_2);
	while (ptr)
	{
		*ptr = '\0';
		total = 0;

		if (!isdigit (*(ptr + 1)) && !isdigit (*(ptr + 2))
				&& !isalpha (*(ptr + 1)) && !isalpha (*(ptr + 2)))
			return -1;

		if ((*(ptr + 3) != '\0') || (*(ptr + 2) == '\0'))
			return -1;

		if (isalpha (*(ptr + 1)))
		{
			if (tolower (*(ptr + 1)) > 'f')
				return -1;
			total += (tolower (*(ptr + 1)) - 'a' + 10) * 16;
		}
		else
			total += (*(ptr + 1) - '0') * 16;

		if (isalpha (*(ptr + 2)))
		{
			if (tolower (*(ptr + 2)) > 'f')
				return -1;
			total += tolower (*(ptr + 2)) - 'a' + 10;
		}
		else
			total += *(ptr + 2) - '0';

		num--;
		*(addr + num) = total;

		ptr = (unsigned char *) strrchr ((char *)mac, MAC_SEP_2);
	}

	if (!isdigit (*(mac + 1)) && !isdigit (*(mac)) && !isalpha (*(mac + 1))
			&& !isalpha (*(mac)))
		return -1;

	if ((*(mac + 2) != '\0') && (*(mac + 1) == '\0'))
		return -1;

	total = 0;
	if (isalpha (*(mac)))
	{
		if (tolower (*(mac)) > 'f')
			return -1;
		total += (tolower (*(mac)) - 'a' + 10) * 16;
	}
	else
		total += (*(mac) - '0') * 16;

	if (isalpha (*(mac + 1)))
	{
		if (tolower (*(mac + 1)) > 'f')
			return -1;
		total += tolower (*(mac + 1)) - 'a' + 10;
	}
	else
		total += *(mac + 1) - '0';

	*(addr) = total;

	if (num == 1)
		return 0;
	else
		return -1;
}  


/**
  * checking  the ipv4  address is valid?
  * @arg str     the ipv4  address string
  * return value:
  *         < 0  : invalid address.
  *         = 0  : valid address.
  */  
int bad_ip_address(char *str)
{
	char *sp;
	int dots = 0, nums = 0;
	char buf[4];

	if (str == NULL)
		return -1;

	for (;;) {
		memset (buf, 0, sizeof (buf));
		sp = str;
		
		while (*str != '\0') {
			if (*str == '.') {
				if (dots >= 3)
					return -1;

				if (*(str + 1) == '.')
					return -1;

				if (*(str + 1) == '\0')
					return -1;

				dots++;
				break;
			}
			
			if (!isdigit ((int) *str))
				return -1;

			str++;
		}

		if (str - sp > 3)
			return -1;

		strncpy (buf, sp, str - sp);
		if (atoi (buf) > 255)
			return -1;

		nums++;

		if (*str == '\0')
			break;

		str++;
	}

	if (nums < 4)
		return -1;

	return 0;
}

/**
  * Check for a valid number.  This should be elsewhere.
  * @arg p      string 
  * @return value:
  *      0          invalid number string
  *      1          valid 
  */
int is_number(const char *p)
{
	do {
		if (! is_digit(*p))
			return 0;
	} while (*++p != '\0');
	return 1;
}

/**
  * execute system command, one executing as child process.the other excuting
  * as parent process. the child process is producer, and the parent process is 
  * customer, they communicate via pipe. it work like this: " cat something | grep 'filte' "
  * @arg command_producer      child process command name 
  * @arg argc_p                          number of child process command paramter  
  * @arg argv_p                          child process command parameter
  * @arg command_customer      parent process command name 
  * @arg argc_c                           number of child child command paramter  
  * @arg argv_c                           parent process command parameter
  * @return value:
  *      < 0       failed
  *      0          success 
  */
int cmd_exec_system_cmd_pipe(char *command_producer, 
		int argc_p, char **argv_p, 
		char *command_customer, int argc_c, char **argv_c)
{
	int pid_p, pid_c, pipe_id[2];
	int ret, status;
	
	if ((pid_p = fork()) < 0)
		return -1;
	
	if (pid_p == 0) {
		if (pipe(pipe_id) < 0)
			return -2;
			
		if ((pid_c = fork()) < 0)
			return -3;
		else if (pid_c == 0) {				/* child process is producer. */
			close(pipe_id[0]);

			if(dup2(pipe_id[1], 1) == -1)
				return -4;

			close(pipe_id[1]);
	        switch (argc_p)
	        {
	            case 0:
	                ret = execlp (command_producer, command_producer, NULL);
	                break;
	            case 1:
	                ret = execlp (command_producer, command_producer, argv_p[0], NULL);
	                break;
	            case 2:
	                ret = execlp (command_producer, command_producer, argv_p[0], 
						argv_p[1], NULL);
	                break;
	            case 3:
	                ret = execlp (command_producer, command_producer, argv_p[0], 
						argv_p[1], argv_p[2], NULL);
	                break;
	            case 4:
	                ret = execlp (command_producer, command_producer, argv_p[0], 
						argv_p[1], argv_p[2], argv_p[3], NULL);
	                break;
	            case 5:
	                ret = execlp (command_producer, command_producer, argv_p[0], 
						argv_p[1], argv_p[2], argv_p[3], argv_p[4], NULL);
	                break;
	            case 6:
	                ret = execlp (command_producer, command_producer, argv_p[0], 
						argv_p[1], argv_p[2], argv_p[3], argv_p[4], argv_p[5], NULL);
	                break;
	            case 7:
	                ret = execlp (command_producer, command_producer, argv_p[0], 
						argv_p[1], argv_p[2], argv_p[3], argv_p[4], argv_p[5], 
						argv_p[6], NULL);
	                break;
	            case 8:
	                ret = execlp (command_producer, command_producer, argv_p[0], 
						argv_p[1], argv_p[2], argv_p[3], argv_p[4], argv_p[5], 
						argv_p[6], argv_p[7], NULL);
	                break;
	        }		
			return -5;
		} else {							/* parent process is customer.*/
			close(pipe_id[1]);
			if(dup2(pipe_id[0], 0) == -1)
				return -6;

			close(pipe_id[0]);
	        switch (argc_c)
	        {
	            case 0:
	                ret = execlp (command_customer, command_customer, NULL);
	                break;
	            case 1:
	                ret = execlp (command_customer, command_customer, argv_c[0], NULL);
	                break;
	            case 2:
	                ret = execlp (command_customer, command_customer, argv_c[0], 
						argv_c[1], NULL);
	                break;
	            case 3:
	                ret = execlp (command_customer, command_customer, argv_c[0], 
						argv_c[1], argv_c[2], NULL);
	                break;
	            case 4:
	                ret = execlp (command_customer, command_customer, argv_c[0], 
						argv_c[1], argv_c[2], argv_c[3], NULL);
	                break;
	            case 5:
	                ret = execlp (command_customer, command_customer, argv_c[0], 
						argv_c[1], argv_c[2], argv_c[3], argv_c[4], NULL);
	                break;
	            case 6:
	                ret = execlp (command_customer, command_customer, argv_c[0], 
						argv_c[1], argv_c[2], argv_c[3], argv_c[4], argv_c[5], NULL);
	                break;
	            case 7:
	                ret = execlp (command_customer, command_customer, argv_c[0], 
						argv_c[1], argv_c[2], argv_c[3], argv_c[4], argv_c[5],
						argv_c[6], NULL);
	                break;
	            case 8:
	                ret = execlp (command_customer, command_customer, argv_c[0], 
						argv_c[1], argv_c[2], argv_c[3], argv_c[4], argv_c[5], 
						argv_c[6], argv_c[7], NULL);
	                break;
	        }
		}
		return -7;		
	} else {
		/* This is parent. */
		ret = wait4 (pid_p, &status, 0, NULL);
	}
	return 0;
}

/**
  *  read  number type proc file
  *  @args proc_file    file path name of proc
  *  @args value         the value will be read from proc file
  *   return value
  *          < 0   error
  *          = 0   success
  */
int proc_num_read(const char *proc_file, unsigned long *value)
{
	FILE *fp = NULL; 
	char buffer[MAX_KEYWORD_LEN] = {0}, *retstr = NULL;
 
	fp = fopen(proc_file, "r");
	if (fp == NULL) {
		printf("open turn file error : %s,%s \n", \
			proc_file, strerror(errno));
		return -ERR_FILE;
	}

	if ( (retstr = fgets(buffer, MAX_KEYWORD_LEN - 1, fp)) != NULL) {
		*value = (unsigned long)atol(buffer);
		fclose(fp);
		return SUCCESS;
	}

	fclose(fp);
	return -ERR_INVAL;
}

/**
  *  write  number type proc file
  *  @args proc_file    file path name of proc
  *  @args value         the value will be write to proc file
  *   return value
  *          < 0   error
  *          = 0   success
  */
int proc_num_write(const char *proc_file, unsigned long value)
{
	char cmd_buff[CMD_STR_BUFF] = {0};
	int retcode ;

	snprintf(cmd_buff, CMD_STR_BUFF - 1, "%s %lu >%s", \
		CMD_ECHO, value, proc_file);
	retcode = system(cmd_buff);
	if (retcode != 0) {
		printf("echo retcode: %d, cmdstr: %s\n", \
			retcode, cmd_buff);
		return -ERR_SYSCMD;
	}

	return SUCCESS;
}

