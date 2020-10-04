#include <sys/socket.h>	// sockaddr_in, socket(), bind(), listen(), accept()
#include <netinet/in.h>	// sockaddr_in
#include <netinet/ip.h>	// sockaddr_in
#include <sys/types.h>	// socket(), bind(), listen(), accept() (some historical implementations required this)
#include <strings.h>	// bzero()
#include <arpa/inet.h>	// htonl(), htons()
#include <time.h>	// time_t, time(), ctime()
#include <stdio.h>	// snprintf()
#include <unistd.h>	// write(), close()
#include <string.h>	// strlen()

#define MAXLINE 1024
#define LISTENQ 5

/* headers for error handling */
#include <stdarg.h>	// varying arguments (va_~~~), vsnprintf()
#include <errno.h>	// errno
#include <stdlib.h> // exit()
//#include <stdio.h> // vsnprintf(), snprintf(), fflush() (already included)
#include <string.h>	// strlen(), strerror(), strcat()
#include <syslog.h>	// syslog(), LOG_ERR

#define MAXLINE_E 100


/**
 * for error handling 
 */
int daemon_proc;	// set nonzero by daemon_init(), only works when the process is daemonized

static void err_doit(int, int, const char*, va_list);

/* Fatal error related to system call
 * Print message, dump core, and terminate */
void err_sys(const char *fmt, ...){
	
	va_list ap;	// pointer indicating the first of variable arguments
	
	va_start(ap, fmt);	// make ap indicate after the last fixed argument
	// could simply get variable arguments like 'var = va_arg(ap, type)'
	err_doit(1, LOG_ERR, fmt, ap);	// 1/0: print errno or not, LOG_ERR: level-error conditions

	va_end(ap);	// the end of using variable arguments
	exit(1);	// abnormal termination
}

/* Fatal error unrelated to system call
 * Print message and terminate */
void err_quit(const char *fmt, ...){
	// similar to err_sys

	va_list ap;

	va_start(ap, fmt);
	// could simply get variable arguments like 'var = va_arg(ap, type)'
	err_doit(0, LOG_ERR, fmt, ap);

	va_end(ap);
	exit(1);
}

/* Print message and return to caller
 * Caller specifies "errnoflag" and "level" */
static void err_doit(int errnoflag, int level, const char* fmt, va_list ap){

	int errno_save, n;
	char buf[MAXLINE_E + 1];

	// errno: integer variable, set by system calls and some library functions on error
	// to use, errno should be saved to prevent from being changed by other calls
	errno_save = errno;

	vsnprintf(buf, MAXLINE_E, fmt, ap);	// vsnprintf() invokes va_arg macro
	// could use vsprintf when vsnprintf doesn't work, but not safe

	n = strlen(buf);
	if (errnoflag)
		snprintf(buf+n, MAXLINE_E-n, ": %s", strerror(errno_save));	// strerror() returns the string describes the errno_save
	strcat(buf, "\n");

	if (daemon_proc){	// only works when the process is daemonized
		syslog(level, "%s", buf);	// format string should be comes to 2nd argument
	} else {
		fflush(stdout);		// in case stdout and stderr are the same
		fputs(buf, stderr);
		fflush(stderr);
	}
	
	return;
}




/**
 * wrapper functions
 * Socket, Bind, Listen, Accept, Write, Close
 */
int Socket(int domain, int type, int protocol){
	
	int n;
	if ( (n = socket(domain, type, protocol)) < 0 )
		err_sys("socket error");

	return n;	// sockfd integer number on success
}

int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
	
	int n;
	if ( (n = bind(sockfd, addr, addrlen)) < 0 ) {
		err_sys("bind error");
	}

	return n;	// 0 on success
}

int Listen(int sockfd, int backlog){

	int n;
	if ( (n = listen(sockfd, backlog)) < 0 ) {
		err_sys("listen error");
	}

	return n;	// 0 on success
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen){

	int n;
	if ( (n = accept(sockfd, addr, addrlen)) < 0 )
		err_sys("accept error");

	return n;	// sockfd integer number on success
}

ssize_t Write(int fd, const void* buf, size_t count){

	int n;
	if ( (n = write(fd, buf, count)) < 0 )
		err_sys("write error");

	return n;	// the number of bytes written on success
}

int Close(int fd){

	int n;
	if ( (n = close(fd)) < 0 ) {
		err_sys("close error");
	}

	return n;	// 0 on success
}




int main(int argc, char **argv){

	int listenfd, connfd;
	struct sockaddr_in servaddr;
	char buff[MAXLINE];
	time_t ticks;

	// AF_INET : communication domian for IPv4 Internet Protocols
	// SOCK_STREAM : communication type for TCP Connections
	// 0 : protocol setting. Only single protocol exists for tcp stream sockets, so can use 0 instead of IPPROTO_TCP.
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));

	// all address/port should be manipulated to  network byte order
	// htonl()/htons() : host byte order to network byte order for unsigned long/unsigned short
	servaddr.sin_family = AF_INET;	// always set to AF_INET
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);	// INADDR_ANY -> bount to all local network interfaces
	servaddr.sin_port = htons(13);	// 13 is reserved port for daytime protocol

	// sockaddr structure only used for pointer casting
	bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

	listen(listenfd, LISTENQ);

	for(;;){
		// 2nd, 3rd arguments can be declared for getting clients information.
		connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

		ticks = time(NULL);
		
		//print formatted output
		snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
		
		// 2 functions below use different header since it is not only for network
		write(connfd, buff, strlen(buff));

		close(connfd);
	}
}

