/* Samantha Rack
 * CSE 30341
 * Project 1
 * copyit_extracredit.c
 * The following C program exactly recursively copies all files in the source directory
 * recurively into the destination directory.  The source and destination directory are
 * identified by the user via command line arguments.
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
#include <sys/stat.h>
#include <dirent.h>
 
#define BUFFERSIZE 4096

/// prototypes	///
int copyit_recur(char *pathToSrc, char *pathToDest);
int copyit(char *srcFile, char *destFile);
void display_message(int s);


/// main ///
int main (int argc, char **argv) {
	// argc -- 3 for correct usage
	// argv[1] -- srcDirectory
	// argv[2] -- destDirectory

	// check user argument count
	if (argc != 3) {
		printf("%s: Wrong number of arguments!\n", argv[0]);
		printf("Usage: %s <sourceDirectory> <targetDirectory>\n", argv[0]);
		exit(1);
	}

	// set up the message display
	if (signal(SIGALRM, display_message) == SIG_ERR) {
		printf("copyitr: Error setting signal: %s\n", strerror(errno));
		exit(1);
	}
	
	// set first alarm	
	alarm(1);

	int totBytes = 0;

	// check if the srcDirectory is actually a directory
	//	if it is a file, then simply copy the file
	struct stat s;
	if (stat(argv[1], &s) != 0) {
			printf("copyitr: %s: %s\n", argv[1], strerror(errno));
			exit(1);
	}
	// for a file as srcDirectory argument
	if (s.st_mode & S_IFREG) {
		totBytes = copyit(argv[1], argv[2]);		
		printf("copytir: Copied %d bytes from %s to %s.\n", totBytes, argv[1], argv[2]);
	}	
	// for a directory as srcDirectory argument
	else {
		// create the destDirectory, if necessary
		if (mkdir(argv[2], S_IRWXU) < 0 && errno != EEXIST) {
			printf("copyitr: Unable to create %s: %s\n", argv[2], strerror(errno));
			exit(1);
		}
		// call recursive function to travel through the srcDirectory
		totBytes = copyit_recur(argv[1], argv[2]);
		printf("copyitr: Copied %d total bytes from directory %s to directory %s.\n", totBytes, argv[1], argv[2]);
	}

	return 0;
}
	

/* Function Name: copyit_recur
 * Preconditions:	srcPath is a valid path to the directory to be copied
 *		destPath is a valid path to the directory to which the
 *		srcPath directory will be copied
 * Recursively calls itself on other directories in srcPath, calls function
 *		copyit on files in srcPath directory.	
 */
int copyit_recur(char *srcPath, char *destPath) {
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(srcPath)) == NULL) {
		printf("copyitr: Unable to open directory %s: %s\n", srcPath, strerror(errno));
		exit(1);
	}

	int totalBytes = 0;

	// for each item in this directory
	while ((ent = readdir(dir)) != NULL) {
		char dirItem[100];
		strcpy(dirItem, ent->d_name);

		if (strcmp(dirItem, ".") == 0 || strcmp(dirItem, "..") == 0) continue;

		// create new srcPath
		char *newSrc = malloc(sizeof(char) * (strlen(srcPath) + 1 + strlen(dirItem) + 1));
		if (newSrc == NULL) {
			printf("copyitr: Malloc error: %s\n", strerror(errno));
			exit(1);
		}
		strcpy(newSrc, srcPath);
		strcat(newSrc, "/");
		strcat(newSrc, dirItem);
		strcat(newSrc, "\0"); 

		// create new destPath
		char *newDest = malloc(sizeof(char) * (strlen(destPath) + 1 + strlen(dirItem) + 1));
		if (newDest == NULL) {
			printf("copyitr: Malloc error: %s\n", strerror(errno));
			exit(1);
		}
		strcpy(newDest, destPath);
		strcat(newDest, "/");
		strcat(newDest, dirItem);
		strcat(newDest, "\0");

		// get what kind of item it is (directory or file)
		struct stat s;
		if (stat(newSrc, &s) != 0) {
			printf("copyitr: %s: %s\n", newSrc, strerror(errno));
			exit(1);
		}

		// if it is a directory
		if (s.st_mode & S_IFDIR) {
			//printf("Found a directory: %s\n", srcPath);
			// create destination folder to which this folder's contents will be copied
			if (mkdir(newDest, S_IRWXU) < 0) {
				printf("copyitr: Unable to create directory %s: %s\n", newDest, strerror(errno));
				exit(1);
			}
			// make recursive call
			totalBytes += copyit_recur(newSrc, newDest);

		}
		// if it is a file
		else if (s.st_mode & S_IFREG) {
			//printf("Found a file.\n");
			totalBytes += copyit(newSrc, newDest);		
		}
		else {
			printf("copyitr: Unknown kind of file not handled: %s\n", srcPath);
		}

		// free the allocated memory
		free(newSrc);
		free(newDest);
	}

	free(dir);	
	
	return totalBytes;
}


/* Function Name: copyit
 * Preconditions: srcFile is a valid path to the file to be copied
 *		destFile is a valid path for the location to which the srcFile 
 *		file will be copied, a file in path destFile does not yet exist
 * Creates a copy of file at path srcFile at destFile location.		*/ 
int copyit(char *srcFile, char *destFile) {
	// open the sourceFile, check for error
	FILE *srcF = fopen(srcFile, "r");
	if (srcF == NULL) {
		printf("copyitr: Unable to open %s: %s\n", srcFile, strerror(errno));
		exit(1);
	}
	
	// check to see if the destinationFile already exists, if yes, give error message
	if (access(destFile, F_OK) != -1) {
		printf("copyitr: File %s already exists. Please rename the original file before copying.\n", destFile);
		exit(1);
	}
 
	// open the destinationFile, check for error
	FILE *destF = fopen(destFile, "w");
	if (destF == NULL) {
		printf("copyitr: Unable to open %s: %s\n", destFile, strerror(errno));
		exit(1);
	} 
	 
	int stillReading = 1;
	int totalBytes = 0;
	
	// set the alarm for the first display
	alarm(1);
 

	char *buffer = malloc(sizeof(char) * BUFFERSIZE);
	if (buffer == NULL) {
		printf("copyitr: Malloc error: %s\n", strerror(errno));
		exit(1);
	}
	//enter loop for reading and copying file
	while (stillReading) {
		int justRead = 0;
		int tryAgain;
		
		// read from max of BUFFERSIZE bytes from sourceFile
		do {
			justRead = fread(buffer, sizeof(char), BUFFERSIZE, srcF);
			if (justRead >= 0) tryAgain = 0;	// successful read
			else if (errno == EINTR) tryAgain = 1;	//read failed because of an interrupt
			else {		//read failed for a fatal reason and prog should exit with error message
				printf("copyitr: Error reading: %s\n", strerror(errno));
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
					printf("copyitr: Error writing: %s\n", strerror(errno));
					exit(1);
				}
			} while (tryAgain);

			totalBytes += justRead;
		}
	}
	free(buffer);
	 
	 // close sourceFile, check for error closing
	if (fclose(srcF) < 0){
		printf("copyitr: Error closing %s: %s\n", srcFile, strerror(errno));
		exit(1);
	}
	 
	// close destinationFile, check for error closing
	if (fclose(destF) < 0) {
		printf("copyitr: Error closing %s: %s\n", destFile, strerror(errno));
		exit(1);
	}

	return totalBytes;
}


/* Function Name: display_message
 */
void display_message(int s) {
	// print message
	printf("copyit: still copying...\n");
	
	// reset the alarm so it will display again in 1 second
	alarm(1);
}
 
