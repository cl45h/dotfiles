#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<fcntl.h>
#include<sys/select.h>
#include<sys/wait.h>
#ifndef NO_X
#include<X11/Xlib.h>
#endif
#ifdef __OpenBSD__
#define SIGPLUS			SIGUSR1+1
#define SIGMINUS		SIGUSR1-1
#else
#define SIGPLUS			SIGRTMIN
#define SIGMINUS		SIGRTMIN
#endif
#define LENGTH(X)               (sizeof(X) / sizeof (X[0]))
#define CMDLENGTH		80
#define MIN( a, b ) ( ( a < b) ? a : b )
#define STATUSLENGTH (LENGTH(blocks) * CMDLENGTH + 1)

typedef struct {
	char* icon;
	char* command;
	unsigned int interval;
	unsigned int signal;
} Block;
#ifndef __OpenBSD__
void dummysighandler(int num);
#endif
void getcmds(int time);
void getsigcmds(unsigned int signal);
void setupsignals();
void sighandler(int signum, siginfo_t *si, void *ucontext);
void runbutton(int sig, int button);
int getstatus(char *str, char *last);
void statusloop();
void termhandler(int signum);
void pstdout();
#ifndef NO_X
void setroot();
static void (*writestatus) () = setroot;
static int setupX();
static Display *dpy;
static int screen;
static Window root;
#else
static void (*writestatus) () = pstdout;
#endif


#include "config.h"

static char statusbar[LENGTH(blocks)][CMDLENGTH] = {0};
static char statusstr[2][STATUSLENGTH];
static int statusContinue = 1;
static int returnStatus = 0;
static int pipefd[2] = { -1, -1 };//self-pipe: handler avisa, el loop hace el trabajo

//opens process *cmd and stores output in *output
void getcmd(const Block *block, char *output)
{
	int off = 0;
	/* statuscmd: el primer byte es el numero de senal del bloque (< ' ', invisible).
	   dwm lo usa para saber sobre que bloque se hizo click y lo filtra al dibujar. */
	if (block->signal)
		output[off++] = block->signal;
	strcpy(output + off, block->icon);
	FILE *cmdf = popen(block->command, "r");
	if (!cmdf)
		return;
	int i = off + strlen(block->icon);
	fgets(output+i, CMDLENGTH-i-delimLen, cmdf);
	i = strlen(output);
	if (i == off) {//return if block and command output are both empty
		pclose(cmdf);//sin esto se fuga el fd y el hijo queda zombie
		return;
	}
	if (delim[0] != '\0') {
		//only chop off newline if one is present at the end
		i = output[i-1] == '\n' ? i-1 : i;
		strncpy(output+i, delim, delimLen); 
	}
	else
		output[i++] = '\0';
	pclose(cmdf);
}

void getcmds(int time)
{
	const Block* current;
	for (unsigned int i = 0; i < LENGTH(blocks); i++) {
		current = blocks + i;
		if ((current->interval != 0 && time % current->interval == 0) || time == -1)
			getcmd(current,statusbar[i]);
	}
}

void getsigcmds(unsigned int signal)
{
	const Block *current;
	for (unsigned int i = 0; i < LENGTH(blocks); i++) {
		current = blocks + i;
		if (current->signal == signal)
			getcmd(current,statusbar[i]);
	}
}

void setupsignals()
{
	/* SA_SIGINFO: dwm manda el boton con sigqueue() en si_value; pkill manda 0 */
	struct sigaction sa = { .sa_sigaction = sighandler, .sa_flags = SA_SIGINFO | SA_RESTART };
	sigemptyset(&sa.sa_mask);
#ifndef __OpenBSD__
	    /* initialize all real time signals with dummy handler */
    for (int i = SIGRTMIN; i <= SIGRTMAX; i++) {
        signal(i, dummysighandler);
        sigaddset(&sa.sa_mask, i);
    }
#endif

	for (unsigned int i = 0; i < LENGTH(blocks); i++) {
		if (blocks[i].signal > 0)
			sigaction(SIGMINUS+blocks[i].signal, &sa, NULL);
	}

}

int getstatus(char *str, char *last)
{
	strcpy(last, str);
	str[0] = '\0';
	for (unsigned int i = 0; i < LENGTH(blocks); i++)
		strcat(str, statusbar[i]);
	str[strlen(str)-strlen(delim)] = '\0';
	return strcmp(str, last);//0 if they are the same
}

#ifndef NO_X
void setroot()
{
	if (!getstatus(statusstr[0], statusstr[1]))//Only set root if text has changed.
		return;
	XStoreName(dpy, root, statusstr[0]);
	XFlush(dpy);
}

int setupX()
{
	dpy = XOpenDisplay(NULL);
	if (!dpy) {
		fprintf(stderr, "dwmblocks: Failed to open display\n");
		return 0;
	}
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	return 1;
}
#endif

void pstdout()
{
	if (!getstatus(statusstr[0], statusstr[1]))//Only write out if text has changed.
		return;
	printf("%s\n",statusstr[0]);
	fflush(stdout);
}


void statusloop()
{
	if (pipe(pipefd) < 0) {
		fprintf(stderr, "dwmblocks: pipe failed\n");
		exit(1);
	}
	/* ambos extremos no bloqueantes: el handler nunca se cuelga escribiendo,
	   y el drenado del loop termina limpio con EAGAIN. */
	fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
	fcntl(pipefd[1], F_SETFL, O_NONBLOCK);
	setupsignals();
	int i = 0;
	getcmds(-1);
	writestatus();
	while (statusContinue) {
		struct timespec ts = { .tv_sec = 1, .tv_nsec = 0 };
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(pipefd[0], &fds);
		int r = pselect(pipefd[0] + 1, &fds, NULL, NULL, &ts, NULL);
		if (r > 0 && FD_ISSET(pipefd[0], &fds)) {
			/* llegaron una o mas senales: drenar el pipe y actualizar esos bloques */
			int msg[2];/* {senal, boton} */
			while (read(pipefd[0], msg, sizeof(msg)) == (ssize_t)sizeof(msg)) {
				if (msg[1])
					runbutton(msg[0], msg[1]);/* click: corre el script con BUTTON, el refresco llega despues */
				else
					getsigcmds(msg[0]);
			}
			writestatus();
		} else if (r == 0) {
			/* tick de 1 segundo: bloques por intervalo */
			getcmds(i++);
			writestatus();
		}
		/* r < 0 (EINTR de SIGTERM/SIGINT): la condicion del while corta */
	}
}

#ifndef __OpenBSD__
/* this signal handler should do nothing */
void dummysighandler(int signum)
{
    return;
}
#endif

void sighandler(int signum, siginfo_t *si, void *ucontext)
{
	/* {senal, boton}: boton != 0 solo cuando viene de dwm por sigqueue() (click en la barra).
	   write() es async-signal-safe y 8 bytes son atomicos (<= PIPE_BUF).
	   El trabajo pesado (popen/fork/Xlib) lo hace el loop principal, nunca aca. */
	int msg[2] = { signum - SIGPLUS, (si && si->si_code == SI_QUEUE) ? si->si_value.sival_int : 0 };
	if (write(pipefd[1], msg, sizeof(msg)) < 0)
		return;
}

/* Click sobre un bloque: corre su comando con BUTTON/BLOCK_BUTTON en el entorno, en un
   nieto (doble fork, lo adopta init) para no tocar SIGCHLD ni pelearse con pclose().
   Al terminar, el script le manda la senal del bloque a dwmblocks para refrescarlo. */
void runbutton(int sig, int button)
{
	unsigned int i;
	for (i = 0; i < LENGTH(blocks) && (int)blocks[i].signal != sig; i++);
	if (i == LENGTH(blocks))
		return;
	pid_t parent = getpid(), pid = fork();
	if (pid == 0) {
		if (fork() == 0) {
#ifndef NO_X
			if (dpy)
				close(ConnectionNumber(dpy));
#endif
			close(pipefd[0]);
			close(pipefd[1]);
			char shcmd[1024], b[2] = { '0' + button, '\0' };
			snprintf(shcmd, sizeof(shcmd), "%s; kill -%d %d", blocks[i].command, SIGRTMIN + sig, parent);
			setenv("BUTTON", b, 1);
			setenv("BLOCK_BUTTON", b, 1);/* nombre que usan los scripts (clock, internet) */
			setsid();
			execl("/bin/sh", "sh", "-c", shcmd, (char *)NULL);
			_exit(1);
		}
		_exit(0);
	}
	if (pid > 0)
		waitpid(pid, NULL, 0);
}

void termhandler(int signum)
{
	statusContinue = 0;
}

int main(int argc, char** argv)
{
	for (int i = 0; i < argc; i++) {//Handle command line arguments
		if (!strcmp("-d",argv[i]))
			strncpy(delim, argv[++i], delimLen);
		else if (!strcmp("-p",argv[i]))
			writestatus = pstdout;
	}
#ifndef NO_X
	if (!setupX())
		return 1;
#endif
	delimLen = MIN(delimLen, strlen(delim));
	delim[delimLen++] = '\0';
	signal(SIGTERM, termhandler);
	signal(SIGINT, termhandler);
	statusloop();
#ifndef NO_X
	XCloseDisplay(dpy);
#endif
	return 0;
}
