/* Samantha Rack
 * CSE 30341
 * Project 1
 * copyit.c
 * The following C program exactly copies a source file into a destination file, with
 * 		both of these arguments provided as arguments on the command line.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
 
#define BUFFERSIZE 4096 
  
  
/// DISPLAY_MESSAGE ///
void display_message(int s) {
	// print message
	printf("copyit: still copying...\n");
	
	// reset the alarm so it will display again in 1 second
	alarm(1);
}
 
 
///// MAIN /////
int main (int argc, char **argv) {
	// argc -- 3 for correct usage
	// argv[1] -- sourceFile
	// argv[2] -- destinationFile

	// check user argument count
	if (argc != 3) {
		printf("%s: Wrong number of arguments!\n", argv[0]);
		printf("Usage: %s <sourceFile> <targetFile>\n", argv[0]);
		exit(1);
	}
	 
	// set up the message display
	if (signal(SIGALRM, display_message) == SIG_ERR) {
		printf("%s: Error setting signal: %s\n", argv[0], strerror(errno));
		exit(1);
	}	
 
	// open the sourceFile, check for error
	FILE *srcF = fopen(argv[1], "r");
	if (srcF == NULL) {
		printf("%s: Unable to open %s: %s\n", argv[0], argv[1], strerror(errno));
		exit(1);
	}
	
	// check to see if the destinationFile already exists, if yes, give error message
	if (access(argv[2], F_OK) != -1) {
		printf("%s: File %s already exists. Please rename the original file before copying.\n", argv[0], argv[2]);
		exit(1);
	}
 
	// open the destinationFile, check for error
	FILE *destF = fopen(argv[2], "w");
	if (destF == NULL) {
		printf("%s: Unable to open %s: %s\n", argv[0], argv[2], strerror(errno));
		exit(1);
	} 
	 
	int stillReading = 1;
	int totalBytes = 0;
	
	// set the alarm for the first display
	alarm(1);
 
	//enter loop for reading and copying file
	while (stillReading) {
		char buffer[BUFFERSIZE];
		int justRead = 0;
		int tryAgain;
		
		// read from max of BUFFERSIZE bytes from sourceFile
		do {
			justRead = fread(buffer, sizeof(char), BUFFERSIZE, srcF);
			if (justRead >= 0) tryAgain = 0;	// successful read
			else if (errno == EINTR) tryAgain = 1;	//read failed because of an interrupt
			else {		//read failed for a fatal reason and prog should exit with error message
				printf("%s: Error reading: %s\n", argv[0], strerror(errno));
				exit(1);
			}
		} while (tryAgain);

		if (justRead == 0) stillReading = 0;
		else {
			// write to destinationFile
			do {
				//if (write(destFd, buffer, justRead) >= 0) tryAgain = 0;	//successful write
				if (fwrite(buffer, justRead, sizeof(char), destF) >= 0) tryAgain = 0;
				else if (errno == EINTR) {
					printf("Had to try again\n");
					tryAgain = 1;	//write failed because of an interrupt
				}
				else {	//write failed for a fatal reason and prog should exit with error message
					printf("%s: Error writing: %s\n", argv[0], strerror(errno));
					exit(1);
				}
			} while (tryAgain);

			totalBytes += justRead;
		}
	}
	 
	 // close sourceFile, check for error closing
	if (fclose(srcF) < 0){
		printf("%s: Error closing %s: %s\n", argv[0], argv[1], strerror(errno));
		exit(1);
	}
	 
	// close destinationFile, check for error closing
	if (fclose(destF) < 0) {
		printf("%s: Error closing %s: %s\n", argv[0], argv[2], strerror(errno));
		exit(1);
	}
	 
	printf("%s: Copied: %d bytes from file %s to %s\n", argv[0], totalBytes, argv[1], argv[2]);
	return 0;
}
 
