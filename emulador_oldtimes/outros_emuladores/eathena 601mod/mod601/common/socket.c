// $Id: socket.c,v 1.4 2003/06/29 05:49:50 lemit Exp $
// original : core.c 2003/02/26 18:03:12 Rev 1.7

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "socket.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

fd_set readfds;
int fd_max;

int rfifo_size = 32768;
int wfifo_size = 32768;

struct socket_data *session[FD_SETSIZE];

static int null_parse(int fd);
static int (*default_func_parse)(int) = null_parse;

/*======================================
 *	CORE : Set function
 *--------------------------------------
 */
void set_defaultparse(int (*defaultparse)(int))
{
	default_func_parse = defaultparse;
}

/*======================================
 *	CORE : Socket Sub Function
 *--------------------------------------
 */

static int recv_to_fifo(int fd)
{
	int len;

	//printf("recv_to_fifo : %d %d\n",fd,session[fd]->eof);
	if(session[fd]->eof)
		return -1;

	len=read(fd,session[fd]->rdata+session[fd]->rdata_size,RFIFOSPACE(fd));
	//{ int i; printf("recv %d : ",fd); for(i=0;i<len;i++){ printf("%02x ",RFIFOB(fd,session[fd]->rdata_size+i)); } printf("\n");}
	if(len>0){
		session[fd]->rdata_size+=len;
	} else if(len<=0){
		printf("set eof :%d\n",fd);
		session[fd]->eof=1;
	}
	return 0;
}

static int send_from_fifo(int fd)
{
	int len;

	//printf("send_from_fifo : %d\n",fd);
	if(session[fd]->eof)
		return -1;

	len=write(fd,session[fd]->wdata,session[fd]->wdata_size);
	//{ int i; printf("send %d : ",fd);  for(i=0;i<len;i++){ printf("%02x ",session[fd]->wdata[i]); } printf("\n");}
	if(len>0){
		if(len<session[fd]->wdata_size){
			memmove(session[fd]->wdata,session[fd]->wdata+len,session[fd]->wdata_size-len);
			session[fd]->wdata_size-=len;
		} else {
			session[fd]->wdata_size=0;
		}
	} else {
		printf("set eof :%d\n",fd);
		session[fd]->eof=1;
	}
	return 0;
}

static int null_parse(int fd)
{
	printf("null_parse : %d\n",fd);
	RFIFOSKIP(fd,RFIFOREST(fd));
	return 0;
}

/*======================================
 *	CORE : Socket Function
 *--------------------------------------
 */

static int connect_client(int listen_fd)
{
	int fd;
	struct sockaddr_in client_address;
	int len;
	int result;

	//printf("connect_client : %d\n",listen_fd);

	len=sizeof(client_address);

	fd=accept(listen_fd,(struct sockaddr*)&client_address,&len);
	if(fd_max<=fd) fd_max=fd+1;
	result = fcntl(fd, F_SETFL, O_NONBLOCK);
	setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,NULL,0);
#ifdef SO_REUSEPORT
	setsockopt(fd,SOL_SOCKET,SO_REUSEPORT,NULL,0);
#endif
	setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,NULL,0);

	if(fd==-1){
		perror("accept");
	} else {
		FD_SET(fd,&readfds);
	}
	result = fcntl(fd, F_SETFL, O_NONBLOCK);
	session[fd] = malloc(sizeof(*session[fd]));
	if(session[fd]==NULL){
		printf("out of memory : connect_client\n");
		exit(1);
	}
	memset(session[fd],0,sizeof(*session[fd]));
	session[fd]->rdata       = malloc(rfifo_size);
	if(session[fd]->rdata==NULL){
		printf("out of memory : connect_client rdata\n");
		exit(1);
	}
	session[fd]->wdata       = malloc(wfifo_size);
	if(session[fd]->wdata==NULL){
		printf("out of memory : connect_client wdata\n");
		exit(1);
	}
	session[fd]->max_rdata   = rfifo_size;
	session[fd]->max_wdata   = wfifo_size;
	session[fd]->func_recv   = recv_to_fifo;
	session[fd]->func_send   = send_from_fifo;
	session[fd]->func_parse  = default_func_parse;
	session[fd]->client_addr = client_address;

  //printf("new_session : %d %d\n",fd,session[fd]->eof);
  return fd;
}

int make_listen_port(int port)
{
	struct sockaddr_in server_address;
	int fd;
	int result;

	fd = socket( AF_INET, SOCK_STREAM, 0 );
	if(fd_max<=fd) fd_max=fd+1;
	result = fcntl(fd, F_SETFL, O_NONBLOCK);
	setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,NULL,0);
#ifdef SO_REUSEPORT
	setsockopt(fd,SOL_SOCKET,SO_REUSEPORT,NULL,0);
#endif
	setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,NULL,0);

	server_address.sin_family      = AF_INET;
	server_address.sin_addr.s_addr = htonl( INADDR_ANY );
	server_address.sin_port        = htons(port);

	result = bind(fd, (struct sockaddr*)&server_address, sizeof(server_address));
	if( result == -1 ) {
		perror("bind");
		exit(1);
	}
	result = listen( fd, 5 );
	if( result == -1 ) { /* error */
		perror("listen");
		exit(1);
	}

	FD_SET(fd, &readfds );
	session[fd] = malloc(sizeof(*session[fd]));
	if(session[fd]==NULL){
		printf("out of memory : make_listen_port\n");
		exit(1);
	}
	memset(session[fd],0,sizeof(*session[fd]));
	session[fd]->func_recv = connect_client;

	return fd;
}

int make_connection(long ip,int port)
{
	struct sockaddr_in server_address;
	int fd;
	int result;

	fd = socket( AF_INET, SOCK_STREAM, 0 );
	if(fd_max<=fd) fd_max=fd+1;
	setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,NULL,0);
#ifdef SO_REUSEPORT
	setsockopt(fd,SOL_SOCKET,SO_REUSEPORT,NULL,0);
#endif
	setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,NULL,0);

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = ip;
	server_address.sin_port = htons(port);

	result = fcntl(fd, F_SETFL, O_NONBLOCK);
	result = connect(fd, (struct sockaddr *)(&server_address),sizeof(struct sockaddr_in));

	FD_SET(fd,&readfds);
	session[fd] = malloc(sizeof(*session[fd]));
	if(session[fd]==NULL){
		printf("out of memory : make_connection\n");
		exit(1);
	}
	memset(session[fd],0,sizeof(*session[fd]));

	session[fd]->rdata      = malloc(rfifo_size);
	if(session[fd]->rdata==NULL){
		printf("out of memory : make_connection rdata\n");
		exit(1);
	}
	session[fd]->wdata      = malloc(wfifo_size);
	if(session[fd]->wdata==NULL){
		printf("out of memory : make_connection wdata\n");
		exit(1);
	}
	session[fd]->max_rdata  = rfifo_size;
	session[fd]->max_wdata  = wfifo_size;
	session[fd]->func_recv  = recv_to_fifo;
	session[fd]->func_send  = send_from_fifo;
	session[fd]->func_parse = default_func_parse;

	return fd;
}

int delete_session(int fd)
{
	if(fd<0 || fd>=FD_SETSIZE)
		return -1;
	FD_CLR(fd,&readfds);
	if(session[fd]){
		if(session[fd]->rdata)
			free(session[fd]->rdata);
		if(session[fd]->wdata)
			free(session[fd]->wdata);
		if(session[fd]->session_data)
			free(session[fd]->session_data);
		free(session[fd]);
	}
	session[fd]=NULL;
	//printf("delete_session:%d\n",fd);
	return 0;
}

int do_sendrecv(int next)
{
	fd_set rfd,wfd;
	struct timeval timeout;
	int ret,i;

	rfd=readfds;
	FD_ZERO(&wfd);
	for(i=0;i<fd_max;i++){
		if(!session[i] && FD_ISSET(i,&readfds)){
			printf("force clr fds %d\n",i);
			FD_CLR(i,&readfds);
			continue;
		}
		if(!session[i])
			continue;
		if(session[i]->wdata_size)
			FD_SET(i,&wfd);
	}
	timeout.tv_sec  = next/1000;
	timeout.tv_usec = next%1000*1000;
	ret = select(fd_max,&rfd,&wfd,NULL,&timeout);
	if(ret<=0)
		return 0;
	for(i=0;i<fd_max;i++){
		if(!session[i])
			continue;
		if(FD_ISSET(i,&wfd)){
			//printf("write:%d\n",i);
			if(session[i]->func_send)
				session[i]->func_send(i);
		}
		if(FD_ISSET(i,&rfd)){
			//printf("read:%d\n",i);
			if(session[i]->func_recv)
				session[i]->func_recv(i);
		}
	}
	return 0;
}

int do_parsepacket(void)
{
	int i;
	for(i=0;i<fd_max;i++){
		if(!session[i])
			continue;
		if(session[i]->rdata_size==0 && session[i]->eof==0)
			continue;
		if(session[i]->func_parse){
			session[i]->func_parse(i);
			if(!session[i])
				continue;
		}
		RFIFOFLUSH(i);
	}
	return 0;
}

void do_socket(void)
{
	FD_ZERO(&readfds);
}
