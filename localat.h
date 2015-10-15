#ifndef CONNECTAT_H
#define CONNECTAT_H

#include<sys/types.h>
#include<sys/socket.h>    /* for sa_family_t */

#ifdef __cplusplus
extern "C" {
#endif

long localat_family(void);
#define AF_UNIXAT localat_family()
#define AF_LOCALAT localat_family()

struct sockaddr_unat
{
	sa_family_t sunat_family;
	int sunat_fd;
	char sunat_path[108];	
};

int socket2(int domain, int type, int protocol);
int connect2(int socket, void*sockaddr, socklen_t addrlen);
int bind2(int socket, void*sockaddr, socklen_t addrlen);

#ifdef __cplusplus
}
#endif

#endif

