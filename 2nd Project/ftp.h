#ifndef FTP_H
#define FTP_H

#include <stdio.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

#define STR_SIZE 256
#define TRUE 1
#define FALSE 0

#define SERVER_PORT 6000
#define SERVER_ADDR "192.168.28.96"

typedef struct{
	int controlFlag;
	int information;
} fileTransferProtocolSockets;

typedef	struct 
{	
	char* host;
	char* path;
	char* username;
	char* password;
	char* filename;
	char* ip;
}url_t;

int parsePath(char * fullPath, url_t *url);

int getIpByHost(url_t* url);

static int connectSocket(const char* ip, int port);

int connectFTP( const char* ip, int port, fileTransferProtocolSockets* ftp);

int loginFTP(const char* user, const char* password, fileTransferProtocolSockets* ftp);

int changeDirFTP(const char* path, fileTransferProtocolSockets* ftp);

int passiveModeFTP(fileTransferProtocolSockets* ftp);

int copyFileFTP(const char* filename, fileTransferProtocolSockets* ftp);

int downloadFileFTP(const char* filename, fileTransferProtocolSockets* ftp);

int disconnectFromFTP(fileTransferProtocolSockets* ftp);

int sendToFTP(int ftpControl, char* str, size_t size);

int readFromFTP(int ftpControl, char* str, size_t size);

int main(int argc, char** argv);

#endif /* FTP_H */