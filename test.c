#include<stdio.h>
#include<stdlib.h>
#include<string.h>     /* for strcpy() */
#include<errno.h>
#include<fcntl.h>      /* for O_RDONLY */
#include<sys/stat.h>   /* for mkdir() */
#include<sys/socket.h> /* for sockaddr */
#include<sys/un.h>     /* for sockaddr_un */
#include<unistd.h>     /* fore read() write() */

#include"localat.h"

int test_localat(void)
{
	int err;
	char buf[8192];
	int i;
	struct sockaddr_unat addr;

	char*dir[8]={"tmpdir0","tmpdir1","tmpdir2","tmpdir3","tmpdir4","tmpdir5","tmpdir6","tmpdir7"};
	for(i=0;i<8;++i)
		system( (sprintf(buf,"rm %s/server 2>/dev/null",dir[i]),buf) ); 
	memset(buf,'\0',8192);

	int fs[8];
	for(i=0;i<8;++i)
	{
		mkdir(dir[i],0777);
		fs[i] = open(dir[i],O_RDONLY);
	}

	/* test for success */

	printf("test: create 8 AF_LOCALAT sockets\n");
	int srv[8];
	for(i=0;i<8;++i)
	{
		errno = 0;
		srv[i] = socket2(AF_LOCALAT,SOCK_STREAM,0);
		if(srv[i] == -1)
		{
			err = errno;
				fprintf(stderr,"fail: srv[%d] ",i);
			errno = err;
			perror("socket");
			exit(1);
		}
	}
	printf("pass: create 8 AF_LOCALAT sockets\n\n");

	printf("test: bind 8 AF_LOCALAT sockets to server files relative to different directories\n");
	addr.sunat_family = AF_LOCALAT;
	strcpy(addr.sunat_path,"server"); 
	for(i=0;i<8;++i)
	{
		addr.sunat_fd = fs[i];
		errno = 0;
		err = bind2(srv[i],&addr,sizeof(struct sockaddr_unat));
		if(err == -1)
		{
			err = errno;
				fprintf(stderr,"fail: srv[%d] ",i);
			errno = err;
			perror("bind2");
			exit(1);
		}
		err = listen(srv[i],128);
		if(err == -1)
		{
			err = errno;
				fprintf(stderr,"fail: srv[%d] ",i);
			errno = err;
			perror("listen");
		}
	}
	printf("pass: bind 8 AF_LOCALAT sockets to server files relative to different directories\n\n");

	printf("test: create 8 AF_LOCALAT sockets\n");
	int cln[8];
	for(i=0;i<8;++i)
	{
		errno = 0;
		cln[i] = socket2(AF_LOCALAT,SOCK_STREAM,0);
		if(cln[i] == -1)
		{
			err = errno;
				fprintf(stderr,"fail: cln[%d] ",i);
			errno = err;
			perror("socket");
			exit(1);
		}
	} 
	printf("pass: create 8 AF_LOCALAT sockets\n\n");

	printf("test: connect 8 AF_LOCALAT sockets to server files relative to different directories\n");
	addr.sunat_family = AF_LOCALAT;
	strcpy(addr.sunat_path,"server"); 
	for(i=0;i<8;++i)
	{
		addr.sunat_fd = fs[i];
		errno = 0;
		err = connect2(cln[i],&addr,sizeof(struct sockaddr_unat));
		if(err == -1)
		{
			err = errno;
				fprintf(stderr,"fail: cln[%d] ",i);
			errno = err;
			perror("connect2");
			exit(1);
		}
	}
	printf("pass: connect 8 AF_LOCALAT sockets to server files relative to different directories\n\n");

	printf("test: accept 8 connections to localat servers (should report themselves as plain local sockets)\n");
	int con[8]; 
	struct sockaddr_un*addrun;
	addrun = (struct sockaddr_un*)buf;
	socklen_t len;
	for(i=0;i<8;++i)
	{
		len = sizeof(struct sockaddr_un);
		//len = 8192;
		con[i] = accept(srv[i],(struct sockaddr*)addrun,&len);
		if(con[i]==-1)
		{
			err = errno;
				fprintf(stderr,"fail: srv[%d] ",i);
			errno = err;
			perror("accept");
			exit(1);
		}

		printf("  debug: peer sockaddr has length: %d\n",len);
		printf("  debug: peer socket is bound to filename '%s'\n",addrun->sun_path);
		if(addrun->sun_path[0] == '\0')
			printf("  debug: peer socket is reporting as an abstract-socket\n");
		else if(addrun->sun_path[0] == ' ')
			printf("  debug: peer socket has a leading space in its path\n");
		if(addrun->sun_family != AF_LOCAL)
		{
			fprintf(stderr,"fail: peer socket is not AF_LOCAL\n");
			fprintf(stderr,"fail: AF_LOCAL = %d\n",AF_LOCAL);
			fprintf(stderr,"fail: peer = %d\n",addrun->sun_family);
			exit(1);
		}
	}
	printf("pass: accept 8 connections to localat servers (should report themselves as non-abstract local sockets)\n\n");

	printf("test: send data from server to 8 clients\n");
	sprintf(buf,"data");
	for(i=0;i<8;++i)
	{
		errno = 0;
		err = write(cln[i],buf,5);
		if(err==1)
		{
			perror("fail: write");
			exit(1);
		}
		if(err<5)
		{
			fprintf(stderr,"fail: write sent %d bytes ",err);
			exit(1);
		}
	}
	printf("pass: send data from server to 8 clients\n\n");

	printf("test: recv data from server to 8 clients\n");
	sprintf(buf,"data");
	for(i=0;i<8;++i)
	{
		memset(buf,'\0',8192);
		errno = 0;
		err = read(con[i],buf,5);
		if(err==1)
		{
			perror("fail: read");
			exit(1);
		}
		if(err<5)
		{
			fprintf(stderr,"fail: read recv %d bytes ",err);
			exit(1);
		}
		if(strcmp(buf,"data"))
		{
			fprintf(stderr,"fail: read recv wrong message ('%s'!='data')\n",buf);
			exit(1);
		}
	}
	printf("pass: recv data from server to 8 clients\n\n");
	
	printf("test: send data from 8 clients to server\n");
	sprintf(buf,"data");
	for(i=0;i<8;++i)
	{
		errno = 0;
		err = write(con[i],buf,5);
		if(err==1)
		{
			perror("fail: write");
			exit(1);
		}
		if(err<5)
		{
			fprintf(stderr,"fail: write sent %d bytes ",err);
			exit(1);
		}
	}
	printf("pass: send data from 8 clients to server\n\n");

	printf("test: recv data from 8 clients to server\n");
	sprintf(buf,"data");
	for(i=0;i<8;++i)
	{
		memset(buf,'\0',8192);
		errno = 0;
		err = read(cln[i],buf,5);
		if(err==1)
		{
			perror("fail: read");
			exit(1);
		}
		if(err<5)
		{
			fprintf(stderr,"fail: read recv %d bytes ",err);
			exit(1);
		}
		if(strcmp(buf,"data"))
		{
			fprintf(stderr,"fail: read recv wrong message ('%s'!='data')\n",buf);
			exit(1);
		}
	}
	printf("pass: recv data from 8 clients to server\n\n");

	/* test for failure */

	printf("test: create 8 AF_LOCALAT sockets\n");
	for(i=0;i<8;++i)
	{
		errno = 0;
		srv[i] = socket2(AF_LOCALAT,SOCK_STREAM,0);
		if(srv[i] == -1)
		{
			err = errno;
				fprintf(stderr,"fail: srv[%d] ",i);
			errno = err;
			perror("socket");
			exit(1);
		}
	}
	printf("pass: create 8 AF_LOCALAT sockets\n\n");

	printf("test: attempt to bind 8 AF_LOCALAT sockets to existing server files\n");
	addr.sunat_family = AF_LOCALAT;
	strcpy(addr.sunat_path,"server"); 
	for(i=0;i<8;++i)
	{
		addr.sunat_fd = fs[i];
		errno = 0;
		err = bind2(srv[i],&addr,sizeof(struct sockaddr_unat));
		if(err == 0)
		{
			fprintf(stderr,"fail: srv[%d] reports it bound to an existing socket\n",i);
			exit(1);
		}
		if(err == -1 && errno!=EADDRINUSE)
		{
			err = errno;
				fprintf(stderr,"fail: srv[%d] ",i);
			errno = err;
			perror("bind2");
			exit(1);
		}
	}
	printf("pass: failed to bind 8 AF_LOCALAT sockets to existing server files relative to different directories\n\n");

	printf("test: create 8 AF_LOCALAT sockets\n");
	for(i=0;i<8;++i)
	{
		errno = 0;
		cln[i] = socket2(AF_LOCALAT,SOCK_STREAM,0);
		if(cln[i] == -1)
		{
			err = errno;
				fprintf(stderr,"fail: cln[%d] ",i);
			errno = err;
			perror("socket");
			exit(1);
		}
	} 
	printf("pass: create 8 AF_LOCALAT sockets\n\n");

	printf("test: attempt to connect 8 AF_LOCALAT sockets to non-existant server files relative to different directories\n");
	addr.sunat_family = AF_LOCALAT;
	strcpy(addr.sunat_path,"server2"); 
	for(i=0;i<8;++i)
	{
		addr.sunat_fd = fs[i];
		errno = 0;
		err = connect2(cln[i],&addr,sizeof(struct sockaddr_unat));
		if(err == 0)
		{
			fprintf(stderr,"fail: cln[%d] reports it connected to an non-existant socket\n",i);
			exit(1);
		}
		if(err == -1 && errno!=ENOENT)
		{
			err = errno;
				fprintf(stderr,"fail: cln[%d] ",i);
			errno = err;
			perror("connect2");
			exit(1);
		}
	}
	printf("pass: failed to connect 8 AF_LOCALAT sockets to non-existant server files relative to different directories\n\n");

	for(i=0;i<8;++i)
	{
		system( (sprintf(buf,"rm %s/server 2>/dev/null",dir[i]),buf) ); 
		system( (sprintf(buf,"rmdir %s 2>/dev/null",dir[i]),buf) ); 
	} 

	return 0; 
}

int test_local(void)
{
	return 0;
}

int main(int argc,char*args[])
{ 
	printf("\nliblocalat test\n\n");
	test_localat();
	test_local();
	printf("all tests pass\n\n");
	return 0;
}

