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

#define SERVER_PORT 6000
#define SERVER_ADDR "192.168.28.96"

#define STR_SIZE 256
#define TRUE 1
#define FALSE 0

typedef struct {
	int data;
	int control;
} ftp_socket_info_t;

typedef	struct {
	char* host;
	char* path;
	char* username;
	char* password;
	char* filename;
	char* ip;
} url_t;

int parsePath(char * fullPath, url_t *url);

int getIpByHost(url_t* url);

static int connectSocket(const char* ip, int port);

int connectFTP( const char* ip, int port, ftp_socket_info_t* ftp);

int loginFTP(const char* user, const char* password, ftp_socket_info_t* ftp);

int changeDirFTP(const char* path, ftp_socket_info_t* ftp);

int passiveModeFTP(ftp_socket_info_t* ftp);

int copyFileFTP(const char* filename, ftp_socket_info_t* ftp);

int downloadFileFTP(const char* filename, ftp_socket_info_t* ftp);

int disconnectFromFTP(ftp_socket_info_t* ftp);

int sendToFTP(int ftpControl, char* str, size_t size);

int readFromFTP(int ftpControl, char* str, size_t size);

int main(int argc, char** argv);

#endif /* FTP_H */
