#include<sys/types.h>
#include<sys/socket.h>			/* for connect() bind() sendmsg() recvmsg() */
#include<sys/un.h>			/* for sockaddr_un */
#include<unistd.h>			/* for close() fstat() fork() */
#include<sys/wait.h>			/* for waitpid() */
#include<fcntl.h>			/* for fcntl() AT_FDCWD */
#include<stdlib.h>			/* for _exit() */
#include<string.h>			/* for strcpy()) */
#include<limits.h>			/* for LONG_MAX */
#include<errno.h>

/* helper functions */

static int popproperties(int fd)
{
	int getfd;
	getfd = fcntl(fd, F_GETFD);
	if(getfd==-1)
		return -1;

	int getfl;
	getfl = fcntl(fd, F_GETFL);
	if(getfl==-1)
		return -1;

	int properties;
	if((getfd & FD_CLOEXEC) == FD_CLOEXEC) 
		properties = O_CLOEXEC;
	else
		properties = 0;

	properties = properties | (getfl & O_NONBLOCK);

	return properties;
}
static int pushproperties(int fd, int properties)
{
	int err;

	int setfd;
	if((properties&O_CLOEXEC)==O_CLOEXEC)
		setfd = FD_CLOEXEC;

	err = fcntl(fd,F_SETFD,setfd);
	if(err==-1)
		return -1;

	int setfl;
	if((properties&O_NONBLOCK)==O_NONBLOCK)
		setfl = O_NONBLOCK;

	return fcntl(fd,F_SETFL,setfl);
}

static int give(int fd, int rw)	
{
	char byte;
	byte = '\0';
	struct iovec unused;
	unused.iov_base = &byte;
	unused.iov_len = sizeof(char);

	struct body
	{
		struct cmsghdr body;
		int rwfd;
		int rofd;
	};

	struct msghdr head;
	struct body body;
	memset(&body,0,sizeof(struct body));

	head.msg_name = NULL;
	head.msg_namelen = 0;
	head.msg_flags = 0;
	head.msg_iov = &unused;
	head.msg_iovlen = 1;

	head.msg_control = &body;
	head.msg_controllen = sizeof(body);

	struct cmsghdr *interface;
	interface = CMSG_FIRSTHDR(&head);

	interface->cmsg_level = SOL_SOCKET;
	interface->cmsg_type = SCM_RIGHTS;

	interface->cmsg_len = CMSG_LEN(1*sizeof(int));

	int*data;
	data = (int*)CMSG_DATA(interface);
	data[0] = rw;

	long err;
	for(;;)
	{
		err = sendmsg(fd, &head, MSG_WAITALL);
		if(err==-1)
		{
			if(errno==EWOULDBLOCK|| errno==EAGAIN|| errno==EINTR)
				continue;
			else
				return -1;
		}
		else if(err==1)
			break;
	}

	return 0;
}
static int take(int fd, int*rw)	
{
	char byte;
	byte = '\0';
	struct iovec unused;
	unused.iov_base = &byte;
	unused.iov_len = sizeof(char);

	struct body
	{
		struct cmsghdr body;
		int fd;
	};

	struct msghdr head;
	struct body body;
	memset(&body,0,sizeof(struct body));

	head.msg_name = NULL;
	head.msg_namelen = 0;
	head.msg_flags = 0;
	head.msg_iov = &unused;
	head.msg_iovlen = 1;

	head.msg_control = &body;
	head.msg_controllen = sizeof(body);

	struct cmsghdr *interface;
	interface = CMSG_FIRSTHDR(&head);

	interface->cmsg_level = SOL_SOCKET;
	interface->cmsg_type = SCM_RIGHTS;

	interface->cmsg_len = CMSG_LEN(1*sizeof(int));

	long err;
	for(;;)
	{
		err = recvmsg(fd, &head, MSG_WAITALL);
		if(err==-1)
		{
			if(errno==EWOULDBLOCK|| errno==EAGAIN|| errno==EINTR)
				continue;
			else
				return -1;
		}
		else if(err==1)
			break;
	}

	if(interface->cmsg_len != CMSG_LEN(1*sizeof(int)))
		/* 
		FIXME check and see if we have 
		actually received them?  do we 
		need to close them?  how do we 
		find out how many we recieved? 
		*/
		return -1;

	int*data;
	data = (int*)CMSG_DATA(interface);

	if(rw!=NULL)
		*rw = data[0];
	else
		close(data[0]);

	return 0; 
}

static int bindat(int fs, int sck, struct sockaddr*data, size_t datalen)
{ 
	int err;
	int tmp;
	int com[2]; 
	int boundsocket;
	int status;
	int sock;

	boundsocket = -1;

	/* make a socket for communicating back to this process */
	err = socketpair(AF_LOCAL, SOCK_STREAM, 0, com);
	if(err==-1)
		return -1;	

	pid_t child;
	child = fork();
	if(child == -1)
	{
		tmp = errno;
			close(com[0]);
			close(com[1]);
		errno = tmp;
		return -1;
	}

	if(child == 0)
	{ 
		/* this code is run in the child process */ 
		close(com[0]);

		err = fchdir(fs);
		if(err==-1)
			goto failsubp;
		close(fs);

		/* create a new socket that we'll pass back to the parent. */
		sock = socket(AF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
		if(sock==-1)
			goto failsubp;

		err = bind(sock, data, datalen);
		if(err==-1)
			goto failsubp;
		
		err = give(com[1], sock);
		if(err==-1)
			goto failsubp;

		_exit(EXIT_SUCCESS); 
	}

	/* this code is run in the parent process */ 
	close(com[1]);

	/*
	first we check if the child failed or succeeded,
	if it suceeded, then we read the fd from the socket.
	*/

	/* wait for the child to exit */
	err = waitpid(child, &status, 0);
	if(err==-1)
		goto fail; 

	if(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS)
	{
		err = take(com[0], &boundsocket);
		/* FIXME manage err -- should crash with err if child has exited */ 
	}
	else
	{
		read(com[0],&tmp,sizeof(int));
		errno = tmp;
		goto fail;
	}

	close(com[0]);
	com[0] = -1;
	/* the child is now gone */
	
	/* cache the properties of socket */
	int cache;
	cache = popproperties(sck);
	if(cache==-1)
		/* FIXME retry iff EINTR or EBUSY */
		goto fail;

	/* rename boundsocket sck | replace the socket we've been given with the new bound socket */
	err = dup2(boundsocket, sck);
	if(err==-1)
		/* FIXME retry iff EINTR or EBUSY */
		goto fail; 
	close(boundsocket);
	boundsocket = -1;

	/* set the new socket to behave like the old one */
	err = pushproperties(sck, cache);
	if(err==-1)
		/* FIXME retry iff EINTR or EBUSY */
		goto fail;

	return 0;

fail:
	tmp = errno;
		if(com[0]!=-1) close(com[0]);
		if(boundsocket!=-1) close(boundsocket);
	errno = tmp;

	return -1;

failsubp:
	tmp = errno;
	write(com[1],&tmp,sizeof(int));
	_exit(EXIT_FAILURE);

	/* unreachable */
	return -1;
}
static int connectat(int fs, int sck, struct sockaddr*data, size_t datalen)
{
	int err;
	int tmp;
	int com[2]; 
	int connectedsocket;
	int status;

	connectedsocket = -1;

	/* make a socket for communicating back to this process */
	err = socketpair(AF_LOCAL, SOCK_STREAM, 0, com);
	if(err==-1)
		return -1;	

	pid_t child;
	child = fork();
	if(child == -1)
	{
		tmp = errno;
			close(com[0]);
			close(com[1]);
		errno = tmp;
		return -1;
	} 
	if(child == 0)
	{ 
		/* this code is run in the child process */ 
		close(com[0]);

		err = fchdir(fs);
		if(err==-1)
			goto failsubp;
		close(fs);

		/* create a new socket that we'll pass back to the parent. */
		int sock;
		sock = socket(AF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
		if(sock==-1)
			goto failsubp;

		err = connect(sock, data, datalen);
		if(err==-1)
			goto failsubp;

		err = give(com[1], sock);
		if(err==-1)
			goto failsubp;

		_exit(EXIT_SUCCESS);
	}

	/* this code is run in the parent process */ 
	close(com[1]);

	err = waitpid(child, &status, 0);
	if(err==-1)
		goto fail; 
	if(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS)
	{
		err = take(com[0],&connectedsocket);
		/* FIXME handle error */ 
	}
	else
	{
		read(com[0],&tmp,sizeof(int));
		errno = tmp;
		goto fail;
	}

	close(com[0]);
	com[0] = -1; 
	/* the child is now gone */

	/* cache the properties of the input socket */
	int cache;
	cache = popproperties(sck);
	if(cache==-1)
		/* FIXME retry iff EINTR or EBUSY */
		goto fail;

	/* replace the input socket with the new bound socket */
	err = dup2(connectedsocket, sck);
	if(err==-1)
		/* FIXME retry iff EINTR or EBUSY */
		goto fail;

	close(connectedsocket);
	connectedsocket = -1;

	err = pushproperties(sck, cache);
	if(err==-1)
		/* FIXME retry iff EINTR or EBUSY */
		goto fail;

	return 0;

fail:
	tmp = errno;
		if(com[0]!=-1) close(com[0]);
		if(connectedsocket!=-1) close(connectedsocket);
	errno = tmp;

	return -1;

failsubp:
	tmp = errno;
	write(com[1],&tmp,sizeof(int));
	_exit(EXIT_FAILURE);

	/* unreachable */
	return -1; 
}

/***
this is a temporary (and poor) mechanism for
getting a unique identifier for the AF_LOCALAT
socket, that won't clash with existing constants.
it should and will be improved later.
***/ 
static long unique = -1; 
long localat_family(void)
{
	if(unique!=-1)
		return unique;

	long i;
	for(i=0;i<LONG_MAX;++i)
		if(i!=AF_LOCAL && i!=AF_UNIX && i!=AF_INET && i!=AF_INET6 &&
		   i!=AF_IPX && 
		   i!=AF_NETLINK &&
		   i!=AF_X25 &&
		   i!=AF_AX25 &&
		   i!=AF_ATMPVC &&
		   i!=AF_APPLETALK &&
		   i!=AF_PACKET)
			return unique = i;

	return -1;
} 
#define AF_UNIXAT localat_family()
#define AF_LOCALAT localat_family()

struct sockaddr_unat
{
	sa_family_t sunat_family;
	int sunat_fd;
	char sunat_path[108];	
};

int socket2(int domain, int type, int protocol)
{
	if(domain == AF_LOCALAT)
		return socket(AF_LOCAL,type,protocol);
	else
		return socket(domain,type,protocol);
}
int connect2(int socket, void*sockaddr, socklen_t addrlen)
{
	struct sockaddr_unat*addr;
	addr = sockaddr;

	if(addr->sunat_family != AF_LOCALAT)
		return connect(socket,(struct sockaddr*)sockaddr,addrlen);

	/* FIXME this either clips the path if it runs over, or overflows, and mangles @len */
	struct sockaddr_un sock;
	sock.sun_family = AF_LOCAL;
	strcpy(sock.sun_path,addr->sunat_path);

	if(addr->sunat_fd == AT_FDCWD || addr->sunat_path[0]=='/')
		return connect(socket,(struct sockaddr*)&sock, sizeof(struct sockaddr_un));

	return connectat(addr->sunat_fd, socket, (struct sockaddr*)&sock, sizeof(struct sockaddr_un));
}
int bind2(int socket, void*sockaddr, socklen_t addrlen)
{ 
	struct sockaddr_unat*addr;
	addr = sockaddr;

	if(addr->sunat_family != AF_LOCALAT)
		return bind(socket,(struct sockaddr*)sockaddr,addrlen);

	/* FIXME this either clips the path if it runs over, or overflows, and mangles @len */
	struct sockaddr_un sock;
	sock.sun_family = AF_LOCAL;
	strcpy(sock.sun_path,addr->sunat_path);

	if(addr->sunat_fd == AT_FDCWD || addr->sunat_path[0]=='/')
		return bind(socket,(struct sockaddr*)&sock, sizeof(struct sockaddr_un));

	return bindat(addr->sunat_fd, socket, (struct sockaddr*)&sock, sizeof(struct sockaddr_un));
}

