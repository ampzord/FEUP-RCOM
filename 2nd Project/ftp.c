#include "ftp.h"

static int connectSocket(const char* ip, int port) {
	int sockfd;
	struct sockaddr_in server_addr;

	// server address handling
	bzero((char*) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip); /*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port); /*server TCP port must be network byte ordered */

	// open an TCP socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		return -1;
	}

	// connect to the server
	if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr))
			< 0) {
		perror("connect()");
		return -1;
	}

	return sockfd;
}

int connectFTP( const char* ip, int port, ftp_sockets* ftp){
	
	char rd[STR_SIZE];

	ftp->data = 0;

	if ((ftp->controlFlag = connectSocket(ip, port)) < 0) {
		printf("Socket cannot be connected - connectFTP.\n");
		return 1;
	}

	if(readFromFTP(ftp->controlFlag, rd, sizeof(rd))){
		printf("Read from FTP failed - connectFTP.\n");
		return 1;
	}
	return 0;
}

int loginFTP(const char* user, const char* password, ftp_sockets* ftp){
	char userTest[STR_SIZE];
	char passTest[STR_SIZE];

	sprintf(userTest, "USER %s\r\n", user);
	sprintf(passTest, "PASS %s\r\n", password);


	if(sendToFTP(ftp->controlFlag, userTest, strlen(userTest))) {
		printf("Sending to FTP failed - loginFTP.\n");
		return 1;
	}

	if(readFromFTP(ftp->controlFlag, userTest, STR_SIZE)){
		printf("Read from FTP failed - loginFTP.\n");
		return 1;
	}

	if(sendToFTP(ftp->controlFlag, passTest, strlen(passTest))){
		printf("Sending to FTP failed - loginFTP.\n");
		return 1;
	}


	if(readFromFTP(ftp->controlFlag, passTest, STR_SIZE)){
		printf("Read from FTP failed - loginFTP().\n");
		return 1;
	}

	return 0;
}

int changeDirFTP(const char* path, ftp_sockets* ftp)
{
	char currPath[STR_SIZE];

	sprintf(currPath, "CWD %s\r\n", path);

	if(sendToFTP(ftp->controlFlag, currPath, strlen(currPath))){
		printf("Sending to FTP failed - changeDirFTP().\n");
		return 1;
	}


	if(readFromFTP(ftp->controlFlag, currPath, STR_SIZE)){
		printf("Read from FTP failed - changeDirFTP().\n");
		return 1;
	}

	return 0;
}



int passiveModeFTP(ftp_sockets* ftp)
{
	char passive[STR_SIZE];
	char passiveIp[STR_SIZE];
	sprintf(passive, "PASV\n");

	if(sendToFTP(ftp->controlFlag, passive, strlen(passive)))
	{
		printf("Sending to FTP failed - passiveModeFTP().\n");
		return 1;
	}

	if(readFromFTP(ftp->controlFlag, passive,STR_SIZE))
	{
		printf("Read from FTP failed - passiveModeFTP().\n");
		return 1;
	}
	
	int ip1,ip2,ip3,ip4;
	int port1, port2;
	if((sscanf(passive,"227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &ip1,&ip2,&ip3,&ip4,&port1,&port2)) < 0)
	{
		printf("Response is wrong.");
		return 1;
	}

	sprintf(passiveIp,"%d.%d.%d.%d",ip1,ip2,ip3,ip4);
	int port = port1*256 + port2;

	if((ftp->data = connectSocket(passiveIp,port)) < 0)
	{
		printf("Passive mode cannot be entered.");
		return 1;
	}
	return 0;
}

int copyFileFTP(const char* filename, ftp_sockets* ftp)
{
	char retr[STR_SIZE];

	sprintf(retr, "RETR %s\n", filename);

	if(sendToFTP(ftp->controlFlag, retr, strlen(retr))){
		printf("Sending to FTP failed - copyFileFTP().\n");
		return 1;
	}


	if(readFromFTP(ftp->controlFlag, retr, STR_SIZE)){
		printf("Read from FTP failed - copyFileFTP().\n");
		return 1;
	}
	return 0;
}

int downloadFileFTP(const char* filename, ftp_sockets* ftp)
{
	FILE* fp;
	int bytes;

	if (!(fp = fopen(filename, "w"))) 
	{
		printf("Can't open file - downloadFileFTP().\n");
		return 1;
	}

	char buf[STR_SIZE];

	while((bytes = read(ftp->data, buf, sizeof(buf))))
	{
		if (bytes < 0) {
			printf("Received no data - downloadFileFTP().\n");
			return 1;
		}

		if((bytes = fwrite(buf,bytes, 1 , fp)) < 0 )
		{
			printf("Write failed - downloadFileFTP().\n");
			return 1;
		}
	}

	fclose(fp);
	close(ftp->data);
	return 0;
}

int disconnectFromFTP(ftp_sockets* ftp)
{
	char disc[STR_SIZE];
	sprintf(disc, "QUIT\n");

	if(sendToFTP(ftp->controlFlag, disc, strlen(disc)))
	{
		printf("Sending to FTP failed - disconnectFromFTP().\n");
		return 1;
	}

	if(readFromFTP(ftp->controlFlag, disc, STR_SIZE))
	{
		printf("Read from FTP failed - disconnectFromFTP().\n");
		return 1;
	}

	return 0;
}

int sendToFTP(int ftpControl, char* str, size_t size)
{
	write(ftpControl,str,size);
	printf("Sent: %s", str);
	return 0;
}


int readFromFTP(int ftpControl, char* str, size_t size)
{
	FILE* fp = fdopen(ftpControl, "r");

	fgets(str, size, fp);
	printf("%s", str);
	
	return 0;
}

int parsePath(char* fullPath, url_information *url)
{
	int i = 0, willHaveLogin = 0, counter = 0;
	
	char model[STR_SIZE];
	sprintf (model, "ftp://");

	//Check if Path starts with "ftp://"
	while (i <= 5)
	{
		if (fullPath[i] != model[i])
		{
			printf("Wrong input\n");
			return 1;
		}
		i++;
	}

	counter = 6;

	//Check if path will require username and password
	if (fullPath[counter] == '[')
		willHaveLogin = TRUE;
	else
		willHaveLogin = FALSE;

	int ctrl = 0;

	//This if statement makes sure that user and password are correctly stored in the struct
	if (willHaveLogin)
	{
		counter++;
		ctrl = 0;
		char * tempUser = malloc(STR_SIZE);

		//Reading username
		while (fullPath[counter] != ':')
		{
			tempUser[ctrl] = fullPath[counter];
			counter++;
			ctrl++;
		}
		url->username=malloc(ctrl);
		strncpy(url->username,tempUser,ctrl);
		free(tempUser);
		counter++;

		ctrl = 0;
		char * tempPass = malloc(STR_SIZE);

		//Reading password
		while (fullPath[counter] != '@')
		{
			tempPass[ctrl] = fullPath[counter];
			counter++;
			ctrl++;
		}
		url->password=malloc(ctrl);
		strncpy(url->password,tempPass,ctrl);
		free(tempPass);

		if (fullPath[counter] != '@' )
		{
			printf("Bad input - missing '@'.\n");
			return 1;
		}

		counter++;

		if (fullPath[counter] != ']' )
		{
			printf("Bad input - missing ']'.\n");
			return 1;
		}
		counter++;
	}
	else
	{
		//If there's no username and password, these fields become 'ftp'
		url->username = malloc(strlen("ftp")+1);
		strncpy(url->username,"ftp",strlen("ftp")+1);

		url->password = malloc(strlen("ftp")+1);
		strncpy(url->password,"ftp",strlen("ftp")+1);
	}

	ctrl = 0;
	char * tempHost = malloc(STR_SIZE);
	while (fullPath[counter] != '/')
	{
		tempHost[ctrl] = fullPath[counter];
		counter++;
		ctrl++;
	}
	url->host=malloc(ctrl+1);
	strncpy(url->host,tempHost,ctrl);
	free(tempHost);

	ctrl = 0;
	char * tempPath = malloc(STR_SIZE);
	char* filename = malloc(STR_SIZE);
	int fileCounter = 0;
	counter++;	
	while (fullPath[counter] != '\0')
	{
		
		tempPath[ctrl] = fullPath[counter];
		
		if(tempPath[ctrl]!='/'){
			filename[fileCounter] = tempPath[ctrl];
			fileCounter++;
		}
		else{
			memset(filename, 0, STR_SIZE);
			fileCounter = 0;
		}
		
		ctrl++;
		counter++;
	}

	url->path=malloc(ctrl);
	strncpy(url->path,tempPath,ctrl-fileCounter);
	url->filename=malloc(fileCounter+1);
	strncpy(url->filename,filename,fileCounter); 	

	free(tempHost);
	return 0;
}


int getIpByHost(url_information* url) 
{
	struct hostent *h;


	printf("Host name url  : %s\n", url->host);

	if ((h=gethostbyname(url->host)) == NULL) {  
		herror("gethostbyname");
		exit(1);
	}

    
	char* ip = inet_ntoa(*((struct in_addr *)h->h_addr));
	url->ip=malloc(sizeof(ip));
	strcpy(url->ip, ip);
	printf("%s\n",url->ip);
	return 0;
}

//added
void printUsage(char* argv0) {
	printf("Usage1 Normal: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv0);
	printf("Usage2 Anonymous: %s ftp://<host>/<url-path>\n\n", argv0);
}

int main(int argc, char** argv) {
	url_information url;
	ftp_sockets ftp;

	int port = 21;
	
	//Check program arguments
	if(argc != 2){
		perror("\nError: Incorrect number of arguments.\n");
		printUsage(argv[0]);
		exit(0);
	}
	
	//-----------------
	
	//check if path is correct
	if(parsePath(argv[1] , &url)){
		perror("Failed on parsing path");
		exit(0);
	}

	printf("path: %s\n", url.path);
	printf("host: %s\n", url.host);
	printf("username: %s\n", url.username);
	printf("pass: %s\n", url.password);
	printf("filename: %s\n", url.filename);

	getIpByHost(&url);
	
	connectFTP(url.ip, port, &ftp);
	
	loginFTP(url.username, url.password, &ftp);
	
	changeDirFTP(url.path, &ftp);

	passiveModeFTP(&ftp);
	
	copyFileFTP(url.filename, &ftp);
	
	downloadFileFTP(url.filename, &ftp);
	
	disconnectFromFTP(&ftp);
	
	return 0;
}