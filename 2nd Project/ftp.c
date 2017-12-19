#include "ftp.h"

static int connectSocket(const char* ip, int port) {
	int sock_fd;
	struct sockaddr_in server_addr;

	//Server handling
	bzero((char*) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;

	//Internet address network
	server_addr.sin_addr.s_addr = inet_addr(ip);

	//TCP server port
	server_addr.sin_port = htons(port); 

	//Open TCP socket
	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		return -1;
	}

	//Connect to server
	if (connect(sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		perror("connect()");
		return -1;
	}

	return sock_fd;
}

int connectThroughFTP( const char* ip, int port, ftp_socket_info_t* ftp){

	char rd[URL_SIZE];
	ftp->data = 0;

	if ((ftp->command_port = connectSocket(ip, port)) < 0) {
		printf("ERROR - Cannot connect Socket - connectFTP.\n");
		return 1;
	}

	if(readFromFTP(ftp->command_port, rd, sizeof(rd))){
		printf("ERROR - Cannot read from FTP - connectFTP.\n");
		return 1;
	}
	return 0;
}

int loginThroughFTP(const char* user, const char* password, ftp_socket_info_t* ftp){
	char userTest[URL_SIZE];
	char passTest[URL_SIZE];

	sprintf(userTest, "USER %s\r\n", user);
	sprintf(passTest, "PASS %s\r\n", password);


	if(sendToFTP(ftp->command_port, userTest, strlen(userTest))) {
		printf("Sending to FTP failed - loginFTP.\n");
		return 1;
	}

	if(readFromFTP(ftp->command_port, userTest, URL_SIZE)){
		printf("Read from FTP failed - loginFTP.\n");
		return 1;
	}

	if(sendToFTP(ftp->command_port, passTest, strlen(passTest))){
		printf("Sending to FTP failed - loginFTP.\n");
		return 1;
	}


	if(readFromFTP(ftp->command_port, passTest, URL_SIZE)){
		printf("Read from FTP failed - loginFTP().\n");
		return 1;
	}

	return 0;
}

int changeDirectoryThroughFTP(const char* path, ftp_socket_info_t* ftp)
{
	char currentPath[URL_SIZE];
	sprintf(currentPath, "CWD %s\r\n", path);

	if(sendToFTP(ftp->command_port, currentPath, strlen(currentPath))){
		printf("ERROR - Cannot send to FTP.\n");
		return 1;
	}

	if(readFromFTP(ftp->command_port, currentPath, URL_SIZE)){
		printf("ERROR - Cannot read from FTP.\n");
		return 1;
	}

	return 0;
}

int enterPassiveModeFTP(ftp_socket_info_t* ftp)
{
	char passive[URL_SIZE];
	char passiveIp[URL_SIZE];
	sprintf(passive, "PASV\n");

	if(sendToFTP(ftp->command_port, passive, strlen(passive)))
	{
		printf("ERROR - Cannot send to FTP - passiveModeFTP().\n");
		return 1;
	}

	if(readFromFTP(ftp->command_port, passive,URL_SIZE))
	{
		printf("ERROR - Cannot read from FTP - passiveModeFTP().\n");
		return 1;
	}

	int ip_1,ip_2,ip_3,ip_4;
	int port_1, port_2;
	if((sscanf(passive,"227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &ip_1,&ip_2,&ip_3,&ip_4,&port_1,&port_2)) < 0)
	{
		printf("ERROR - Passive mode response incorret.\n");
		return 1;
	}

	sprintf(passiveIp,"%d.%d.%d.%d",ip_1,ip_2,ip_3,ip_4);
	int port = (port_1 * 256) + port_2;

	if((ftp->data = connectSocket(passiveIp,port)) < 0)
	{
		printf("ERROR - Cannot enter passive mode.\n");
		return 1;
	}
	return 0;
}

int copyFileThroughFTP(const char* filename, ftp_socket_info_t* ftp)
{
	char retr[URL_SIZE];
	sprintf(retr, "RETR %s\n", filename);

	if(sendToFTP(ftp->command_port, retr, strlen(retr))){
		printf("ERROR - Cannot send to FTP - copyFileFTP().\n");
		return 1;
	}


	if(readFromFTP(ftp->command_port, retr, URL_SIZE)){
		printf("ERROR - Cannot read from FTP - copyFileFTP().\n");
		return 1;
	}
	return 0;
}

int downloadFileThroughFTP(const char* filename, ftp_socket_info_t* ftp)
{
	FILE* fp;
	int bytes;
	char buf[URL_SIZE];

	if (!(fp = fopen(filename, "w")))
	{
		printf("ERROR - Cannot open file to write information- downloadFileFTP().\n");
		return 1;
	}

	while((bytes = read(ftp->data, buf, sizeof(buf))))
	{
		if (bytes < 0) {
			printf("ERROR - No information was received - downloadFileFTP().\n");
			return 1;
		}

		if((bytes = fwrite(buf,bytes, 1 , fp)) < 0 )
		{
			printf("ERROR - Cannot write to file - downloadFileFTP().\n");
			return 1;
		}
	}

	fclose(fp);
	close(ftp->data);
	return 0;
}

int disconnectFromFTP(ftp_socket_info_t* ftp)
{
	char disc[URL_SIZE];
	sprintf(disc, "QUIT\n");

	if(sendToFTP(ftp->command_port, disc, strlen(disc)))
	{
		printf("ERROR - Cannot send to FTP - disconnectFromFTP().\n");
		return 1;
	}

	if(readFromFTP(ftp->command_port, disc, URL_SIZE))
	{
		printf("ERROR - Cannot read from FTP - disconnectFromFTP().\n");
		return 1;
	}

	return 0;
}

int sendToFTP(int ftp_control, char* str, size_t size)
{
	write(ftp_control,str,size);
	printf("Sending to FTP : %s", str);
	return 0;
}

int readFromFTP(int ftp_control, char* str, size_t size)
{
	FILE* fp = fdopen(ftp_control, "r");
	fgets(str, size, fp);
	printf("Reading from FTP : %s", str);

	return 0;
}

int parseURLPath(char * fullPath, url_info_t *url)
{
	int authenticatedLogin = 0;
	int counter = 0;
	char model[URL_SIZE];
	
	//Check if the first part of url is "ftp://"
	sprintf (model, "ftp://");
	int i = 0;
	while (i <= 5)
	{
		if (fullPath[i] != model[i])
		{
			printf("The first part of the URL path is not correct, expected : \"ftp://\" \n");
			return 1;
		}
		i++;
	}

	counter = 6;
	//Check if URL is authenticated or anonymous
	if (fullPath[counter] == '[')
		authenticatedLogin = TRUE;
	else
		authenticatedLogin = FALSE;

	int ctrl = 0;
	//Check if username and password are stored correctly :[<user>:<password>@]
	if (authenticatedLogin)
	{
		char* tempUser = malloc(URL_SIZE);
		counter++;
		ctrl = 0;

		//Username
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
		char* tempPass = malloc(URL_SIZE);

		//Password
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
	// Anonymous login : username ftp , password ftp
	else {
		url->username = malloc(strlen("ftp")+1);
		strncpy(url->username,"ftp",strlen("ftp")+1);

		url->password = malloc(strlen("ftp")+1);
		strncpy(url->password,"ftp",strlen("ftp")+1);
	}

	ctrl = 0;
	char* tempHost = malloc(URL_SIZE);
	while (fullPath[counter] != '/') {
		tempHost[ctrl] = fullPath[counter];
		counter++;
		ctrl++;
	}
	url->host=malloc(ctrl+1);
	strncpy(url->host,tempHost,ctrl);
	free(tempHost);

	ctrl = 0;
	char * tempPath = malloc(URL_SIZE);
	char* filename = malloc(URL_SIZE);
	int fileCounter = 0;
	counter++;

	while (fullPath[counter] != '\0') {
		tempPath[ctrl] = fullPath[counter];

		if(tempPath[ctrl]!='/'){
			filename[fileCounter] = tempPath[ctrl];
			fileCounter++;
		}
		else{
			memset(filename, 0, URL_SIZE);
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

int getIPFromHost(url_info_t* url)
{
	struct hostent *h;

	printf("Host Name URL  : %s\n", url->host);

	if ((h=gethostbyname(url->host)) == NULL) {
		herror("gethostbyname");
		exit(1);
	}

	char* ip_address = inet_ntoa(*((struct in_addr *)h->h_addr));
	url->ip=malloc(sizeof(ip_address));
	strcpy(url->ip, ip_address);
	printf("%s\n",url->ip);
	return 0;
}

void printProgramUsage(char* argv) {
	printf("1 - Anonymous usage: %s ftp://<host>/<url-path>\n", argv);
	printf("2 - Authenticated usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n\n", argv);
}


int main(int argc, char** argv){
	
	url_info_t url;
	ftp_socket_info_t ftp;
	int port = 21;
	
	//Checking number of arguments
	if(argc != 2){
		printProgramUsage(argv[0]);
		perror("\nIncorrect number of arguments\n");
		exit(0);
	}

	//URL
	if(parseURLPath(argv[1] , &url)){
		perror("ERROR - parsing URL path");
		exit(0);
	}

	//DEBUG MODE
	printf("Username: %s\n", url.username);
	printf("Password: %s\n", url.password);
	printf("Host : %s\n", url.host);
	printf("Path : %s\n", url.path);
	printf("Filename: %s\n", url.filename);

	//Getting IP from Hostname
	getIPFromHost(&url);

	//Connecting to FTP
	connectThroughFTP(url.ip, port, &ftp);

	//logging in FTP
	loginThroughFTP(url.username,url.password, &ftp);

	//Changing directory to file's directory
	changeDirectoryThroughFTP(url.path,&ftp);

	//Entering Passive mode 
	enterPassiveModeFTP(&ftp);

	//Copying file
	copyFileThroughFTP(url.filename,&ftp);

	//Downloading File 
	downloadFileThroughFTP(url.filename,&ftp);

	//Disconneting from FTP
	disconnectFromFTP(&ftp);

	return 0;
}
