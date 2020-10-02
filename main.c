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

