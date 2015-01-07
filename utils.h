#ifndef _UTILS_H_
#define _UTILS_H_

#include <sys/time.h>
#include <asm/param.h>

#define MAC_SEP_1    '-'
#define MAC_SEP_2    ':'
#define MAC_LENGTH   17

/* control console output  */
#define NONE         "\033[m"  
#define RED          "\033[0;32;31m"
#define RED_BLINK    "\033[5;32;31m"
#define LIGHT_RED    "\033[1;31m"
#define GREEN        "\033[0;32;32m"
#define LIGHT_GREEN  "\033[1;32m"
#define BLUE         "\033[0;32;34m"
#define LIGHT_BLUE   "\033[1;34m"
#define DARY_GRAY    "\033[1;30m"
#define CYAN         "\033[0;36m"
#define LIGHT_CYAN   "\033[1;36m"
#define PURPLE       "\033[0;35m"
#define LIGHT_PURPLE "\033[1;35m"
#define BROWN        "\033[0;33m"
#define YELLOW       "\033[1;33m"
#define LIGHT_GRAY   "\033[0;37m"
#define WHITE        "\033[1;37m"

#define ON_STR       GREEN"ON"NONE
#define OFF_STR      RED_BLINK"OFF"NONE
#define ANY_STR      CYAN"Any"NONE
#define TBL_HDR_FMT  "\033[4m%s\033[24m"
#define SPLIT_FMT	 "\033[41\033[1m\033[30m\033[40m""%s"NONE
#define DATA_PRE     "\033[7m"

/* Error Code */
#define SUCCESS                  0
#define ERR_CMDLINE              1
#define ERR_SYSTEM               2
#define ERR_DB                   3
#define ERR_DB_ENTRY_EXIST       4
#define ERR_DB_ENTRY_NOT_EXIST   5
#define ERR_MEM                  6
#define ERR_SYSCMD               7
#define ERR_DB_IP_EXIST          8 
#define ERR_INVAL                9
#define ERR_FILE                 10
#ifndef MAX_KEYWORD_LEN
#define MAX_KEYWORD_LEN		     16
#endif
#define CMD_ECHO                 "/bin/echo"

#define is_digit(c) ((unsigned)((c) - '0') <= 9)
int is_mac_valid (const char *mac_addr);
int bad_ip_address(char *str);
int macstr_to_macaddr (unsigned char *addr, char *src_mac);
int is_number(const char *p);
int proc_num_read(const char *proc_file, unsigned long *value);
int proc_num_write(const char *proc_file, unsigned long value);

#endif /* _UTILS_H_ */
