#include "utils.h"

char lastBCC2=0xFF;


void printReceiverStats() {
	printf("\n\n---------------------------------------\n");
	printf("---------------------------------------\n");
	printf("---------------------------------------\n");
	printf("      Program Receiver Statistics      \n");
	printf("---------------------------------------\n");
	printf("---------------------------------------\n");
	printf("---------------------------------------\n\n");

	printf("           Messages received  : %d\n", receiverStats.receivedMessages);
	printf("Successfull Messages received : %d\n", receiverStats.successfulMessages);
	printf("           RR frame sent      : %d\n", receiverStats.rrSent);
	printf("           REJ frame sent     : %d\n", receiverStats.rejSent);
}

void writeBytes(int fd, char* message){

	printf("SendBytes Initialized\n");
    int size=strlen(message);
	int sent = 0;

    while( (sent = write(fd,message,size+1)) < size ){
        size -= sent;
    }

}

char * readBytes(int fd){
   	char* collectedString=malloc (sizeof (char) * 255);
	char buf[2];
	int counter=0,res=0;
    while (STOP==FALSE) {       /* loop for input */

	  res = read(fd,buf,1);   /* returns after 1 chars have been input */

	  if(res==-1)
		exit(-1);
	  buf[1]='\0';
      collectedString[counter]=buf[0];

      if (buf[0]=='\0'){
	  collectedString[counter]=buf[0];
	  STOP=TRUE;
      }
      counter++;
    }
    printf("end result:%s\n",collectedString);
	return collectedString;

}

/*
* A global state machine to read one by one depending on C
*/
char readSupervision(int fd, int counter, char C){

	char set[5]={0x7E,0x03,C,0x03^C,0x7E};
	char buf[1];
	int res=0;
  res = read(fd,buf,1);

	if(res==-1){
		printf("Error reading, returning ERROR...\n");
		return ERR;
	}

	printf("buf[0] = %02x\n", buf[0]);

	switch(counter){
	case 0:
		if(buf[0]==set[0]){
			return 0x7E;
		}
		return ERR;
	case 1:
		if(buf[0]==set[1]){
			return 0x03;
		}
		return ERR;
	case 2:
		if(buf[0]==set[2]){
			return C;
		}
		//special case;
		if(C==0x07 && buf[0]==0x0B){
			return 0x0C;
		}
		return ERR;
	case 3:
		if(buf[0]==set[3]){
			return buf[0];
		}
		return ERR2;
	case 4:
		if(buf[0]==set[4]){
			return 0X7E;
		}
		return ERR;
  default:
		return ERR;
	}
}

/*
* Reads SET frame and replies with UA frame
*/
void llopen(int fd, int type){
 char ua[5]={0x7E,0x03,0x07,0x03^0x07,0x7E};
 char readchar[2];
 int counter = 0;
 if(type == 0){
	 printf("Reading SET frame with value: ");
	 while (STOP == FALSE) {

	  readchar[0]=readSupervision(fd,counter,0x03);
	  printf("0x%02x",(unsigned char) readchar[0]);
	  readchar[1]='\0';
	  counter++;

	  if(readchar[0] == ERR){
	  	counter=0;
	  }

	  if(readchar[0] == ERR2){
	  	counter=-1;
	  }

	  if(counter == 5){
		 STOP = TRUE;
		 printf("\nSET frame read successfully!\n");
	  }
	 }
 }
 writeBytes(fd,ua);
 printf("\nUA frame sent!\n");
}

/*
* Creates an error packet struct
*/
DataPack makeErrorPack(int errno)
{
	DataPack errpack;
	errpack.size = 1;
	errpack.arr = malloc(errpack.size);
	errpack.arr[0] = errno;
	return errpack;
}

/*
* Checks if the received packet BBC2 is valid
*/
int validateBCC2(DataPack dataPacket,unsigned char BCC2){
	int i;
	unsigned char makeBCC2;
	makeBCC2 = dataPacket.arr[0]^dataPacket.arr[1];

	for (i = 2; i < dataPacket.size;i++)
	{
		makeBCC2 = makeBCC2^dataPacket.arr[i];
	}

	if(BCC2 == makeBCC2)
		return 0;
	return -1;
}

/*
* Destuffs a information packet received
*/
DataPack destuffPack(DataPack todestuff)
{

	char* dbuf=malloc(todestuff.size-6);
	DataPack dataPacket;

	// counter for finding all bytes to destuff, starts on 4 because from 0 to 3 is the header
	//j is a counter to put bytes on dbuf;
	int i=4;
	int j=0;

	//Finding content that needs destuffing
	while(i<todestuff.size-2)
	{

		if(todestuff.arr[i] == 0x7E){
					printf("Stuffing failed, found 0x7E before final position of packet\n");
					return makeErrorPack(-1);
		}

		//no flags
		if(todestuff.arr[i] != 0x7D){

		 dbuf[j] = todestuff.arr[i];
		 i++;
		 j++;
		 continue;
		}

		if(todestuff.arr[i] == 0x7D){

			if(todestuff.arr[i+1] == 0x5E){
				dbuf[j] = 0x7E;
				i = i+2;
				j++;
				continue;
		 	}

			if(todestuff.arr[i+1] == 0x5d){
				dbuf[j] = 0x7D;
				i = i+2;
				j++;
				continue;
			}
			printf("Destuffing failed, found unstuffed 0x7D\n");
			return makeErrorPack(-1);
		}

	}

	if(todestuff.arr[todestuff.size-1] != 0x7E){
		printf("Invalid Packet, found 0x%02x at final position of packet, should be 0x7E\n",(unsigned char)todestuff.arr[todestuff.size-1]);
		return makeErrorPack(-1);
	}

  dataPacket.size=j;
  dataPacket.arr=dbuf;

	if(validateBCC2(dataPacket,(unsigned char)todestuff.arr[todestuff.size-2]) == -1){
		printf("BCC2 doesn't match with BCC2 of received contents, please resend Packet\n");
		return makeErrorPack(-1);
	}

	if(lastBCC2 != (unsigned char)todestuff.arr[todestuff.size-2]){
		lastBCC2 = (unsigned char)todestuff.arr[todestuff.size-2];
		return dataPacket;
	}
	else
		return makeErrorPack(-2);
}

/*
* checks if the Information frame header is correct (with the FLAG, ADDRESS, CONTROL, BCC1)
*/
ResponseArray readInfPackHeader(int fd, char* buf){

	//Verifying that the header of the package is correct
	ResponseArray response;
	char c1alt;
	char REJ[5]={0x7E,0x03,0x01,0x03^0x01,0x7E};
	char restartERR2[5]={ERR2,ERR2,ERR2,ERR2,ERR2};

	//FIRST FLAG
	if(buf[0] != 0x7E){

		printf("Error reading the FLAG_RECEIVER: 0x7E flag!\n");
		memcpy(response.arr,REJ,5);
		return response;
	}

	//ADDRESS
	if(buf[1] != 0x03){
		printf("Error reading the ADDRESS byte!\n");
		memcpy(response.arr,REJ,5);
		return response;
	}

	//CONTROL1
	if(buf[2] != 0x00 && buf[2] != 0x40){
		if(buf[2]== 0x03){
			if(buf[3]==0x00){
				llopen(fd,1);
				memcpy(response.arr,restartERR2,5);
				return response;
			}
		}
		printf("Error reading the CONTROL byte!\n");
		memcpy(response.arr,REJ,5);
		return response;
	}

	//Verifying BCC1
	if((buf[1]^buf[2]) != buf[3]){
		printf("Error! BCC byte is not equal to ADDRESS XOR CONTROL\n");
		memcpy(response.arr,REJ,5);
		return response;

	}

	//C1 switch
	if(buf[2] == 0x00){
		c1alt = 0x40;
	}
	else if(buf[2] == 0x40){
		c1alt = 0x00;
	}

	//creating header of start package to send
	char RR[5] = {0x7E,0x03,c1alt,0x03^c1alt,0x7E};
	memcpy(response.arr,RR,5);
	printf("Information frame header successfully read!\n");

	return response;
}

/*
* Reads the first information data frame (START) with information about file size and name
*/
ResponseArray readStartPacketInfo(char * startPacket, ResponseArray res)
{
	char tempI[5];
	char temp[50];
	char REJ[5]={0x7E,0x03,0x01,0x03^0x01,0x7E};

	tempI[0]=startPacket[6];
	tempI[1]=startPacket[5];
	tempI[2]=startPacket[4];
	tempI[3]=startPacket[3];

	int currentI=6;
	sprintf(temp,"%02x%02x%02x%02x",(unsigned char)tempI[0],(unsigned char)tempI[1],(unsigned char)tempI[2],(unsigned char)tempI[3]);

	//output is 00002ad8
	file.fileSize=strtol(temp,NULL,16);
	printf("File size is: %d bytes.\n", file.fileSize);
	if(file.fileSize<0 || file.fileSize>4*pow(10,9))
	{
		memcpy(res.arr,REJ,5);
		return res;
	}

	int fileNameSize = startPacket[currentI+2];
	printf("Size of the file name is: %d bytes.\n",fileNameSize);
	file.arr=malloc(fileNameSize+1);

	if(fileNameSize <= 0)
	{
		memcpy(res.arr,REJ,5);
		return res;
	}

	int i =0;

	while( i < fileNameSize)
	{
		file.arr[i] = startPacket[currentI+3+i];
		i++;
	}

	printf("File name is: %s.\n",file.arr);

	return res;
}

/*
* Moves read packet to struct InformationPacket
*/
DataPack getPacketRead(int fd,int wantedsize){
	DataPack sp;
	sp.size = wantedsize;
	sp.arr = malloc(sp.size);
	int res = -1;
	int counter = 0;
	int first7E = FALSE;

	while(counter < wantedsize)
	{
		res = read(fd,&sp.arr[counter],1);
		if(counter < 5){
			printf("%d %02x\n",counter,sp.arr[counter]);
		}
		if(res == -1)
		{
			printf("Error reading exiting program...\n");
			exit(-1);
		}

		//2nd 7E Found
		if(first7E == TRUE){
			if(sp.arr[counter] == 0x7E)
			{
				sp.size=counter+1;
				sp.arr = realloc(sp.arr,sp.size);
				break;
			}
		}

		if(sp.arr[counter] == 0x7E)
		{
			if(counter!=0)
				return makeErrorPack(-1);
			first7E = TRUE;
		}
		counter++;
	}

	return sp;
}

/*
* validates the start packet from sender
*/
void validateStartPack(int fd){

	DataPack sp=getPacketRead(fd,50);
	ResponseArray response =readInfPackHeader(fd,sp.arr);

	if(response.arr[0] == ERR2)
	{
		printf("Detected SET, Resent UA, going to try and read new Start Pack...\n");
		readStart = FALSE;
		return;
	}


  if(response.arr[2] == 0x01){
		readStart = FALSE;
		return;
	}


	switch(sp.arr[2])
	{
		case 0x00:
		sp = destuffPack(sp);
		if(sp.arr[0]==-1){
			readStart=FALSE;
			return;
		}
		if(sp.arr[0]==-2){
			readStart=FALSE;
			response = readStartPacketInfo(sp.arr,response);
			writeBytes(fd,response.arr);
			return;
		}
		response = readStartPacketInfo(sp.arr,response);
		writeBytes(fd,response.arr);
		if(response.arr[2]==0x01){
			readStart=FALSE;
			return;
		}
		readStart=TRUE;
		break;

    default:
		printf("Rejecting invalid Starter Packet, try again \n");
		writeBytes(fd,response.arr);
		readStart=FALSE;
		break;
	 }
}

void writeFileInfo(DataPack data){
	printf("Writing to file %s -> %d bytes\n",file.arr,data.size);
	fwrite(data.arr,1,data.size,fp);
	receiverStats.successfulMessages++;
}

void openFile()
{
	fp=fopen(file.arr,"wb");
	if(fp==NULL)
	{
		printf("Cannot read file! Exiting... \n");
		exit(-1);
	}
}

void llread(int fd)
{
	char REJ[5]={0x7E,0x03,0x01,0x03^0x01,0x7E};

	while(readStart == FALSE)
	{
		//validated the start pack from I frame
		validateStartPack(fd);

		if(readStart == TRUE)
		{
			openFile();
			int sizeRead=0;

		 	while (sizeRead < file.fileSize)
			{
				DataPack filepacket;
				while(packetValidated == FALSE)
				{
					receiverStats.receivedMessages++;
					filepacket=getPacketRead(fd,PACKET_SIZE+PACKET_SIZE/2);
					if(filepacket.arr[0] == -1){
						printf("Invalid frame!\n");
						packetValidated = FALSE;
						continue;
					}

					ResponseArray response =readInfPackHeader(fd,filepacket.arr);

					if(response.arr[0] == ERR2)
					{
					  printf("Detected SET, Resent UA, going to try and read new Start Pack\n");
					  sizeRead=0;
					  readStart=FALSE;
					  break;
				  }

				    switch(response.arr[2])
					{
						case 0x00:
							filepacket = destuffPack(filepacket);
							if(filepacket.arr[0]==-1){
								packetValidated=FALSE;
							  free(filepacket.arr);
							  memcpy(response.arr,REJ,5);
							  printf("Sending REJ\n");
								write(fd,response.arr,5);
								receiverStats.rejSent++;
								packetValidated=FALSE;
								continue;
							}
							if(filepacket.arr[0] == -2){
								printf("Resending RR0\n");
								write(fd,response.arr,5);
								receiverStats.rrSent++;
								packetValidated=FALSE;
								continue;
							}
							printf("Sending RR0\n");
							write(fd,response.arr,5);
							receiverStats.rrSent++;
							packetValidated = TRUE;
							break;
						case 0x40:

							filepacket = destuffPack(filepacket);
							if(filepacket.arr[0]==-1){
								packetValidated=FALSE;
							  free(filepacket.arr);
							  memcpy(response.arr,REJ,5);
							  printf("Sending REJ\n");
								write(fd,response.arr,5);
								receiverStats.rejSent++;
								packetValidated=FALSE;
								continue;
							}
							if(filepacket.arr[0]==-2){
								printf("Resending RR1\n");
								write(fd,response.arr,5);
								packetValidated=FALSE;
								continue;
							}
						  printf("Sending RR1\n");
							write(fd,response.arr,5);
							receiverStats.rrSent++;
							packetValidated=TRUE;
							break;
						case 0x01:

							printf("Sending REJ\n");
							write(fd,response.arr,5);
							receiverStats.rejSent++;
							packetValidated=FALSE;
							continue;
					}
				}


				if(readStart==FALSE){
					printf("Starting from beginning...\n");
					break;
				}

				sizeRead += filepacket.size;
				writeFileInfo(filepacket);
				printf("current size read is %d \n",sizeRead);
				packetValidated = FALSE;
				if(sizeRead >= filepacket.size)
					readFile = TRUE;
			}
			printf("readStart %d \n",readStart);
			printf("readFile %d \n",readFile);
		}

	}
	fclose(fp);

	printf("llread ended with success!\n");
	return;
}

void llclose(int fd){
 char ua[5] = {0x7E,0x03,0x07,0x03^0x07,0x7E};
 int readDISC =FALSE;
 char DISC[5] ={0x7E,0x03,0X0B,0X03^0X0B,0X7E};
 char readchar[2];
 int counter = 0;

 while (STOP == FALSE) {
 	  if(readDISC == FALSE)
	  	readchar[0] = readSupervision(fd,counter,DISC[2]);
	  else readchar[0] = readSupervision(fd,counter,ua[2]);
	  	printf("0x%02x \n",(unsigned char) readchar[0]);
	  readchar[1]='\0';

	  counter++;

	  if(readchar[0] == ERR){
	  	counter=0;
	  }

	  if(readchar[0] == ERR2){
	  	counter=-1;
	  }

	  if(readchar[0] == 0x0C){
	  	counter = 0;
	    printf("Sending DISC...\n");
	 		writeBytes(fd,DISC);
	  }

	  if (counter == 5){

		 if(readDISC == TRUE){
		    printf("Read UA, terminating...\n");
		 		STOP = TRUE;
		 }

		 if(readDISC == FALSE){
	 	  printf("Resending DISC...\n");
	 	  writeBytes(fd,DISC);
	 	  counter = 0;
    	 }
		 readDISC = TRUE;

	  }
	}
 }

int main(int argc, char** argv)
{
    int fd;
    struct termios oldtio,newtio;

    if ( (argc < 2) ||
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(-1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 1 chars received */



  /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

  	llopen(fd,0);
	  llread(fd);
	  STOP = FALSE;
	  llclose(fd);
    sleep(2);
		printReceiverStats();

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
