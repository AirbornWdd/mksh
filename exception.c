#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <execinfo.h>
#include <signal.h>
#include "command.h"
/* Initialization of signal handles. */
#include <readline/readline.h>
#include <readline/history.h>


#ifndef N_
#define N_(Arg) Arg
#endif

/* signal list */
#if 0
kill -l
 1) SIGHUP       2) SIGINT       3) SIGQUIT      4) SIGILL       5) SIGTRAP
 6) SIGABRT      7) SIGBUS       8) SIGFPE       9) SIGKILL     10) SIGUSR1
11) SIGSEGV     12) SIGUSR2     13) SIGPIPE     14) SIGALRM     15) SIGTERM
16) SIGSTKFLT   17) SIGCHLD     18) SIGCONT     19) SIGSTOP     20) SIGTSTP
21) SIGTTIN     22) SIGTTOU     23) SIGURG      24) SIGXCPU     25) SIGXFSZ
26) SIGVTALRM   27) SIGPROF     28) SIGWINCH    29) SIGIO       30) SIGPWR
31) SIGSYS      34) SIGRTMIN    35) SIGRTMIN+1  36) SIGRTMIN+2  37) SIGRTMIN+3
38) SIGRTMIN+4  39) SIGRTMIN+5  40) SIGRTMIN+6  41) SIGRTMIN+7  42) SIGRTMIN+8
43) SIGRTMIN+9  44) SIGRTMIN+10 45) SIGRTMIN+11 46) SIGRTMIN+12 47) SIGRTMIN+13
48) SIGRTMIN+14 49) SIGRTMIN+15 50) SIGRTMAX-14 51) SIGRTMAX-13 52) SIGRTMAX-12
53) SIGRTMAX-11 54) SIGRTMAX-10 55) SIGRTMAX-9  56) SIGRTMAX-8  57) SIGRTMAX-7
58) SIGRTMAX-6  59) SIGRTMAX-5  60) SIGRTMAX-4  61) SIGRTMAX-3  62) SIGRTMAX-2
63) SIGRTMAX-1  64) SIGRTMAX
#endif
const char *const siglist[] = {
	N_("Signal 0"),
	N_("Hangup"),
	N_("Interrupt"),
	N_("Quit"),
	N_("Illegal instruction"),
	N_("Trace/breakpoint trap"),
	N_("SIGABRT"),
	N_("Bus error"),
	N_("Floating point exception"),
	N_("Killed"),
	N_("User defined signal 1"),
	N_("Segmentation fault"),
	N_("User defined signal 2"),
	N_("Broken pipe"),
	N_("Alarm clock"),
	N_("Terminated"),  
	N_("SIGSTKFLT"),
	N_("Child exited"),	
	N_("Continued"),	
	N_("Stopped (signal)"),
	N_("Stopped"),	
	N_("Stopped (tty input)"),
	N_("Stopped (tty output)"),	
	N_("Urgent I/O condition"), 
	N_("CPU time limit exceeded"),
	N_("File size limit exceeded"),	
	N_("Virtual timer expired"),
	N_("Profiling timer expired"),	
	N_("Window changed"),	
	N_("I/O possible"),
	NULL
};

void trace_assert(void)
{
	int i;
	void *bt[CMD_STR_BUFF];
	int bt_size;
	char **bt_sym;

	bt_size = backtrace(bt, CMD_STR_BUFF);
	bt_sym = backtrace_symbols(bt, bt_size);
	fprintf(stderr, "\n########## Backtrace ##########\n");	
	fprintf(stderr, "Number of elements in backtrace: %d\n", bt_size);
	if (!bt_sym)
		return;

	for (i = 0; i < bt_size; i++) {
			fprintf(stderr, "%s\n", bt_sym[i]);
	}
	free(bt_sym);
	fprintf(stderr,"########## Done ##########\n");
}

void sig_handle (int sig)
{
	struct sigaction sa;
	
	fprintf(stderr, "Task execption for %s(%d)\n", siglist[sig], sig);
	trace_assert();

	/* Pass on the signal (so that a core file is produced).  */
	sa.sa_handler = SIG_DFL;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction (sig, &sa, NULL);
	raise (sig);
}

static void sigint_handle(int sig)
{
	rl_replace_line("", 0);
	rl_crlf();
	rl_forced_update_display();
}

/* Signale wrapper. */
void* signal_set (int signo, void (*func)(int))
{
	int ret;
	struct sigaction sig;
	struct sigaction osig;

	sig.sa_handler = func;
	sigemptyset (&sig.sa_mask);
	sig.sa_flags = 0;
#ifdef SA_RESTART
	sig.sa_flags |= SA_RESTART;
#endif /* SA_RESTART */

	ret = sigaction (signo, &sig, &osig);

	if (ret < 0) 
		return (SIG_ERR);
	else
		return (osig.sa_handler);
}

/* Initialization of signal handles. */
void signal_init(void)
{
	signal (SIGTSTP,    SIG_IGN);
	signal (SIGPIPE,    SIG_IGN);	
	signal_set(SIGINT,  sigint_handle);
	signal_set(SIGSEGV, sig_handle);/*("Segmentation fault")*/
	signal_set(SIGBUS,  sig_handle);/*("Bus error")*/
	signal_set(SIGILL,  sig_handle); /*("Illegal instruction")*/
	signal_set(SIGFPE,  sig_handle); /*("Floating point exception")*/
	signal_set(SIGABRT, sig_handle); 
}

