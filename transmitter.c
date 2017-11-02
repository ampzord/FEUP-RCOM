#include "utils.h"

int RR_RECEIVED = FALSE;
int REJ_RECEIVED = FALSE;

FileData file;

unsigned char C1 = 0x40;
int fsize;
FILE *fp;
int flag=0, conta=1;

void printSenderStats() {
	printf("\n\n---------------------------------------\n");
	printf("---------------------------------------\n");
	printf("---------------------------------------\n");
	printf("      Program Sender Statistics        \n");
	printf("---------------------------------------\n");
	printf("---------------------------------------\n");
	printf("---------------------------------------\n\n");

	printf("      Messages sent      : %d\n", senderStats.sentMessages);
	printf("      RR frame received  : %d\n", senderStats.rrReceived);
	printf("      REJ frame received : %d\n", senderStats.rejReceived);
	printf("      Timeout count      : %d\n", senderStats.timeoutNumber);
}

/*
* switch for the C1 byte used in I frame, control byte field
*/
void switchC1()
{
	if (C1 == 0x00)
		C1 = 0x40;
	else
		C1 = 0x00;
}

/*
* resets the flags telling if RR or REJ have been received. called after receiving the receiver's answer to I frames
*/
void resetRRRejFlags()
{
	RR_RECEIVED = FALSE;
	REJ_RECEIVED = FALSE;
}

void endOfLL()
{
	conta = 0;
	STOP = FALSE;
	flag = 0;
}

/*
* alarm handler
*/
void atende()
{
	printf("Alarm #%d\n", conta);
	flag=TRUE;
	conta++;
	senderStats.timeoutNumber++;
}

/*
* reads the supervision frame (SET, UA, DISC, RR, REJ) received in fd. Needs a control byte which defines which supervision frame it is
*/
int readSupervisionPacket(unsigned char C, int fd)
{
	char buf[1];
	char sup[5] = {FLAG, 0x03, C, 0x03^C, 0x7E}; //creates a temporary frame that matches the "correct" answer
	int counter = 0;
	int errorflag =0;	//checks for any errors during reading. if any byte does not correspond to the "correct" answer, this flag gets <0 value

	while (STOP==FALSE && counter < 5)
	{
		read(fd,buf,1); //reads one by one

		switch(counter)
		{
			case 0:
			if(buf[0]!=sup[0])
				errorflag=-1;
			break;
			case 1:
			if(buf[0]!=sup[1])
				errorflag=-1;
			break;
			case 2:
			if(buf[0]!=sup[2])
				errorflag=-1;
			break;
			case 3:
			if(buf[0] != sup[3])
				errorflag=-1;
			break;
			case 4:
			if(buf[0]!=sup[4])
				errorflag=-1;
			break;
		};
		counter++;

		if (counter==5 && errorflag ==0) //if everything matches the "correct" answer
		{
			STOP=TRUE;
			return 0;
		}
	}
	return -1;
}

int writeBytes(int fd)
{
	char ua[5] = {0x7E,0x03,0x07,0x03^0x07,0x7E};
	int res;

	res = write(fd,ua,5);
	printArray(ua,5);
	printf("UA frame sent with the following values: %02x %02x %02x %02x %02x\n", ua[0], ua[1], ua[2], ua[3], ua[4]);
	return res;
}

void writeSet(int fd)
{
	int res;
	char set[5] = {0x7E,0x03,0x03,0x00,0x7E};
	res=write(fd,set,5);
	printf("SET frame sent with the following values: %02x %02x %02x %02x %02x\n", set[0], set[1], set[2], set[3], set[4]);
}

/*
* reads or sends the DISC frame, according to the toRead flag. if true, reads a DISC frame then sends one back, otherwise just sends one. used in llclose. returns the number of bytes read.
*/
int sendReadDISC(int fd,int toRead)
{
	if (toRead == TRUE)
	{
		unsigned char C = 0x0B;
		int k = readSupervisionPacket(C,fd);
		return k;
	}

	unsigned char disc[5];
	unsigned char A=0x03;
	unsigned char C = 0x0B;
	unsigned char BCC1 = A^C;
	disc[0] = FLAG;
	disc[1] = A;
	disc[2] = C;
	disc[3] = BCC1;
	disc[4] = FLAG;

	write(fd,disc,5);
	printf("DISC frame sent with the following values: %02x %02x %02x %02x %02x\n", disc[0], disc[1], disc[2], disc[3], disc[4]);
	return 0;
}

int readUa(int fd)
{
	unsigned char C = 0x07;
	int k = readSupervisionPacket(C,fd);
	return k;
}


/*
* checks if a frame is RR or REJ. used in llwrite. returns -1 if there's an error reading, 0 if a RR was read, 1 if a REJ was read
*/
int detectRRorREJ(int fd)
{
	char buf[5];
	read(fd,buf,5);
	
	//FLAG
	if(buf[0] != 0x7E){
		//printf("Error reading the first 0x7E flag!\n");
		return -1;
	}

	//ADDRESS
	if(buf[1] != 0x03){
		printf("Error reading the ADDRESS byte!\n");
		return -1;
	}

	//CONTROL = 0x01/0x81 means that this frame is REJ
	if(buf[2] == 0x01 )
	{
		if((buf[1]^buf[2]) != buf[3]){
			printf("Error! BCC byte is not equal to ADDRESS XOR CONTROL\n");
			return -1;
		}

		//FLAG
		if (buf[4] != 0x7E) {
			printf("Error reading the second 0x7E flag!\n");
			return -1;
		}

		//SUCCESS
		printf("REJ read successfully with the values: %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
		REJ_RECEIVED=TRUE;
		printArray(buf,5);
		return 1;
	}

	//CONTROL = 0x00 or 0x40 means that this frame is RR
	if(buf[2] == 0x00 || buf[2]==0x40){

		//BCC
		if((buf[1]^buf[2]) != buf[3]){
			printf("Error! BCC byte is not equal to ADDRESS XOR CONTROL\n");
			return -1;
		}
		
		//FLAG
		if (buf[4] != 0x7E) {
			printf("Error reading the second 0x7E flag!\n");
			return -1;
		}

		//SUCCESS
		printf("RR read successfully with the values: %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
		RR_RECEIVED=TRUE;
		printArray(buf,5);
		switchC1();
		return 0;
	}
	return -1;
}

/*
* send the file packets through fd to the receiver
*/
int sendInfoFile(int fd, unsigned char *buf, int size)
{
	int newSize = (size + 6); //total size = original size + 6 because of the F, A, C, BCC1, BCC2, F bytes
	int i, j, res, k;
	unsigned char BCC2,BCC1;

	for (i = 0; i < size; i++) //cycles though the original packet to check if there's any 0x7E or 0x7D. if so, the total size of the packet post-stuffing is increased by one of each found
	{
		if (buf[i] == 0x7E || buf[i] == 0x7D)
		{
			newSize++;
		}
	}

	//BBC = buf[0] ^ buf[1] ^ ... ^ buf[size - 1]
	BCC2 = buf[0] ^ buf[1];
	for (i = 2; i < size;i++)
	{
		BCC2 = BCC2^buf[i];
	}

	//creates the new packet
	unsigned char *dataPacket = (unsigned char*)malloc(newSize);

	dataPacket[0] = FLAG;
	dataPacket[1] = 0x03;
	dataPacket[2] = C1;
	BCC1 = dataPacket[1]^C1;
	dataPacket[3] = BCC1;

	//STARTING STUFFING
	j = 5;
	k = 4;
	for (i = 0; i < size;i++)
	{
		if (buf[i] == 0x7E)
		{
			dataPacket[k] = 0x7D;
			dataPacket[j] = 0x5E;
			k++;
			j++;
		}

		if (buf[i] == 0x7D)
		{
			dataPacket[k] = 0x7D;
			dataPacket[j] = 0x5D;
			k++;
			j++;
		}

		if(buf[i] != 0x7D && buf[i] != 0x7E)
			dataPacket[k] = buf[i];

		j++;
		k++;
	}
	//ENDS STUFFING

	dataPacket[newSize-2] = BCC2;
	dataPacket[newSize-1] = FLAG;
	//ENDS I FRAME FLAGS

	//writes complete and stuffed packet to fd
	res = write(fd,dataPacket,newSize);
	if (res == 0 || res == -1)
	{
		printf("Sending file, wrote %d bytes.\n", res);
		return res;
	}
	return res;
}

/*
* Handles the process of dividing file into PACKET_MAX_SIZE bytes portions. Returns the starting byte number of the packet being read
*/
int getDataPacket(int fd)
{
	int bytesRead = 0,res,read = 0;

	unsigned char *dataPacket = (unsigned char *)malloc(PACKET_SIZE); //creates data packet with preset size

	//READ PACKET_SIZE BYTES "AT A TIME"
	while ((bytesRead = fread(dataPacket, sizeof(unsigned char), PACKET_SIZE, fp)) > 0) //reads from the fp FILE (global access) to the dataPacket to send, reading a char * PACKET_MAX_SIZE. this while cycles until it starts reading nothing (end of file)
	{
		while (conta < 4) //timeout limit
		{
			res = sendInfoFile(fd,dataPacket,bytesRead); //creates a "ready to send" data packet and writes it to fd, returning the number of bytes sent

			printf("Sending file, currently at: %d/%d (%.2lf%%/100%%).\n", read, fsize, ((double)read / fsize) * 100);

			//starts alarm
			flag = FALSE;
			alarm(3);

			if (res == -1) //error case
			{
				while (!flag){}
				continue;
			}
			else
			{
				senderStats.sentMessages++;
				while(!flag && (RR_RECEIVED == FALSE && REJ_RECEIVED == FALSE)) //while nothing has been received back
				{
					detectRRorREJ(fd);
				}
			}

			if (RR_RECEIVED)
			{
				//stops alarm
				conta = 0;
				alarm(0);

				senderStats.rrReceived++;
				printf("RR received! Sending next packet...\n\n");
				dataPacket = memset(dataPacket, 0, PACKET_SIZE); //"restarts" the packet buffer
				read += bytesRead; //update current byte being read
				resetRRRejFlags();
				break;
			}
			if (REJ_RECEIVED)
			{
				senderStats.rejReceived++;
				printf("REJ received! Resending the same packet...\n\n");
				resetRRRejFlags();
				continue;
			}
			printf("RR nor REJ received. Retrying...\n\n");
			resetRRRejFlags();
			continue;
		}
		if(conta>=4){
			printf("Maximum timeouts reached! Stopping sending file at byte number %d\n\n", read);
			return read;
		}
	}
	STOP = TRUE;
	return read;
}

/*
* builds and sends the starting I frame
*/
unsigned char *buildStartPacket(int fd)
{
	int i=0, j=0;

	fseek(fp,0,SEEK_END);
	fsize = ftell(fp);
	fseek(fp,0,SEEK_SET);

	unsigned char A = 0x03;
	unsigned char C = 0x00;
	unsigned char BCC1 = A^C;
	unsigned char BCC2;

	int startBufSize = 9+file.fileSize;

	unsigned char *startBuf = (unsigned char *)malloc(startBufSize);

	startBuf[0] = 0x02;
	startBuf[1] = 0x00;
	startBuf[2] = 0x04;
	int* intloc= (int*)(&startBuf[3]);
	*intloc=fsize;
	startBuf[7] = 0x01; //starts at index 7 because of the 0x04 at index 2. So file size occupies 4 bytes, indexes 3, 4, 5 and 6.
	startBuf[8] = file.fileSize;

	//INSERTS FILE NAME IN ARRAY
	for(i = 0; i < file.fileSize;i++)
	{
		startBuf[i+9] = file.arr[i];
	}

	//calculation packet final size (updated later in the function)
	int sizeFinal = startBufSize+6;
	for (i = 0; i < startBufSize;i++)
	{
		if (startBuf[i] == 0x7E || startBuf[i] == 0x7D)
			sizeFinal++;
	}

	//BCC2 value
	BCC2 = startBuf[0]^startBuf[1];
	for (i = 2; i < startBufSize;i++)
	{
		BCC2 = BCC2^startBuf[i];
	}

	unsigned char *dataPackage;

	//hard code stuffing and insertion of bcc2
	if (BCC2 == 0x7E)
	{
		dataPackage = (unsigned char *)malloc(sizeFinal+1);
		dataPackage[sizeFinal-3] = 0x7D;
		dataPackage[sizeFinal-2] = 0x5E;
	}

	if (BCC2 == 0x7D)
	{
		dataPackage = (unsigned char *)malloc(sizeFinal+1);
		dataPackage[sizeFinal-3] = 0x7D;
		dataPackage[sizeFinal-2] = 0x5D;
	}

	if (BCC2 != 0x7D && BCC2 != 0x7E)
	{
		 dataPackage = (unsigned char *)malloc(sizeFinal);
		 dataPackage[sizeFinal-2] = BCC2;
	}

	//STARTING FLAGS
	dataPackage[0] = FLAG;
	dataPackage[1] = A;
	dataPackage[2] = 0x00;
	BCC1 = A^dataPackage[2];
	dataPackage[3] = BCC1;

	//STARTING STUFFING
	j = 5;
	int k=4;
	for (i = 0; i < startBufSize;i++)
	{
		if (startBuf[i] == 0x7E)
		{
			dataPackage[k] = 0x7D;
			dataPackage[j] = 0x5E;
			k++;
			j++;
		}

		if (startBuf[i] == 0x7D)
		{
			dataPackage[k] = 0x7D;
			dataPackage[j] = 0x5D;
			k++;
			j++;
		}

		if(startBuf[i] != 0x7D && startBuf[i] != 0x7E)
			dataPackage[k] = startBuf[i];

		j++;
		k++;
	}
	//FINISH STUFFING

	dataPackage[sizeFinal-1] = FLAG;
	//FINISH FLAGS

	int res;
	res = write(fd, dataPackage, sizeFinal);
	printf("I start frame sent with the size of %d bytes!\n", res);
	return 0;
}

int llwrite(int fd)
{
	printf("Entering LLWRITE\n");
	int res;
	buildStartPacket(fd);
	res = getDataPacket(fd);
	endOfLL();
	return res;
}

int llopen(int fd)
{
	printf("Entering LLOPEN\n");
	while(conta < 4) //timeouts
	{
		writeSet(fd); //writes SET
		alarm(3); //starting alarm
		
		while(!flag && STOP == FALSE) //reads UA
		{
			readUa(fd);
		}
		if(STOP==TRUE)
		{
			alarm(0); //if successfully read, stops the alarm
			endOfLL();
			return 0;
		}
		else
			flag=0;
	}
	return -1;
}

int llclose(int fd)
{
	printf("Entering LLCLOSE\n");
	int receivedDISC = FALSE;
	int res = 0;
	while(conta < 4)
	{
		sendReadDISC(fd,FALSE); //sends DISC
		printf("DISC frame sent.\n");
		alarm(3); //starts alarm

		while(!flag && STOP == FALSE) //reads DISC from receiver
		{
			res = sendReadDISC(fd,TRUE);
			
			if (res == 0){
				printf("DISC frame received successfully!\n");
				STOP = TRUE;
				receivedDISC=TRUE;
			}
		}

		if(STOP==TRUE)
		{
			alarm(0);
			endOfLL();
			break;;
		}
		else
			flag=0;
	}
	if(receivedDISC==FALSE){
		printf("Did not received DISCONNECT from Receiver, try sending file again\n");
		return -1;	
	
	}
	res = writeBytes(fd); //writes UA
	if (res == 5)
	{
		printf("UA frame sent! Leaving LLCLOSE...\n");
	}

	return 0;
}

/*
* cycle which determines if file was sent
*/
int cycle(int fd)
{
	int fsize, finish = 0,res,counter=0,read=0;
	fseek(fp,0,SEEK_END);
	fsize = ftell(fp);
	fseek(fp,0,SEEK_SET);

	while (finish == 0)
	{
		if(counter >=4)
			return -1;
		counter++;
		read = 0;

		res = llopen(fd);

		if (res == -1)
		{
			printf("LLOPEN failed, finishing program...\n");
			return -1;
		}
		read = llwrite(fd);

		if (read < fsize)
		{
			continue;
		}
		else {
			printf("File sent successfully\n");
			finish = 1;
		}
	}

	if(llclose(fd)==-1)
		cycle(fd);
	return 0;
}

int main(int argc, char** argv)
{
	(void) signal(SIGALRM, atende);
	int fd;
	struct termios oldtio,newtio;

	if ( (argc < 3) ||
		((strcmp("/dev/ttyS0", argv[1])!=0) &&
			(strcmp("/dev/ttyS1", argv[1])!=0) ))
	{
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1 filename \n");
		exit(1);
	}

	file.fileSize = strlen(argv[2]);
	printf("File size %d , argv[2] = %s\n",file.fileSize,argv[2]);
	file.arr = (char*)malloc(file.fileSize);
	memcpy(file.arr,argv[2],file.fileSize);

	fp = fopen(argv[2],"rb");
	fd = open(argv[1], O_RDWR | O_NOCTTY );
	if (fd <0)
	{
		perror(argv[1]); exit(-1);
	}

	if ( tcgetattr(fd,&oldtio) == -1)
	{
		perror("tcgetattr");
		exit(-1);
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME]    = 3;
	newtio.c_cc[VMIN]     = 0;

	tcflush(fd, TCIOFLUSH);

	if ( tcsetattr(fd,TCSANOW,&newtio) == -1)
	{
		perror("tcsetattr");
		exit(-1);
	}
	cycle(fd);
	fclose(fp);
	printSenderStats();

	if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}
	close(fd);
	return 0;
}
