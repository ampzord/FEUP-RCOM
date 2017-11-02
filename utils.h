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
#define BAUDRATE B9600
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define ERR 0x36
#define ERR2 0x35
#define PACKET_SIZE 2048

#define FLAG_RECEIVER 0x7E
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
#define PACKET_MAX_SIZE 1024 

#define FRAME_START 0x02
#define FRAME_DATA  0x01
#define FRAME_END 0x03




int status = TRUE;
volatile int STOP=FALSE;
volatile int readFile=FALSE;
volatile int readStart = FALSE;
volatile int packetValidated=FALSE;

typedef struct{
	char arr[5];
} ResponseArray;

typedef struct{
	char* arr;
	int size;
} DataPack;

typedef struct{
	char* arr;
	int namelength;
	int fileSize;
} FileData;


FileData file;
FILE *fp;

void printArray(char* arr,size_t length){

	int index;
	for( index = 0; index < length; index++){
			printf( "0x%X\n", (unsigned char)arr[index] );
	}
}

#endif
