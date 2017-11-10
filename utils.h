#ifndef __UTILS_H
#define __UTILS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <math.h>

#define MODEMDEVICE "/dev/ttyS1"
#define BAUDRATE 57600 //default - B9600
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define SYNC_ERROR 0xFD //done
#define BCC_ERROR 0xFE  //done
#define PACKET_SIZE 1024

#define FLAG_RECEIVER 0x7E //done
#define ADDRESS_SENDER_TO_RECEIVER 0x03
#define CONTROL_UA 0x07
#define CONTROL_DISC 0x0B
#define CONTROL_SET 0x03

#define CONTROL_REJ 0x01
#define CONTROL_RR  0x05

#define CONTROL_I_START_FRAME 0x00

#define FLAG_ESC 0x7D

#define SYNC_ERROR 0xFD
#define BCC_ERROR 0xFE
#define PACKET_MAX_SIZE 2048 
//packet size to test : 128, 256 , 512, 1024, 2048

#define FRAME_START 0x02
#define FRAME_DATA  0x01
#define FRAME_END 0x03

int status = TRUE;
volatile int STOP = FALSE;
volatile int readFile = FALSE;
volatile int readStart = FALSE;
volatile int packetValidated = FALSE;

typedef struct{
	char arr[5];
} ReplyArray;

typedef struct{
	char* arr;
	int size;
} InformationPacket;

typedef struct{
	char* arr;
	int namelength;
	int fileSize;
} FileInformation;

typedef struct{
	int sentMessages;
	int rrReceived;
	int rejReceived;
	int timeoutNumber;
} SenderStatistics;

typedef struct{
	int successfulMessages;
	int receivedMessages;
	int rrSent;
	int rejSent;
} ReceiverStatistics;

FileInformation file;
FILE *fp;
SenderStatistics senderStats;
ReceiverStatistics receiverStats;
#endif
