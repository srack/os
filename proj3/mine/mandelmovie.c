/* Samantha Rack
 * CSE 30341
 * Project 3
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <math.h>

#define NUM_FRAMES 50

int main (int argc, char **argv) {
	// command line check
	if (argc != 2) {
		printf("mandelmovie: Usage: mandelmovie <numProcesses>\n");
		exit(1);
	}

	// make sure argv[1] is an int
	int i;
	for (i = 0; argv[1][i] != '\0'; ++i) {
		if (!isdigit(argv[1][i])) {
			printf("mandelmovie: numProcesses must be an integer.\n");
			exit(1);
		}
	}
	
	int maxP = atoi(argv[1]);
	if (maxP <= 0) maxP = 1;
	int numP = 0;
	double zoom = 2;
	double endZoom = .0000000001;
	
	for (i = 0; i < NUM_FRAMES; ++i) {
		while (numP >= maxP) {
			int status;	
			pid_t pidR = wait(&status);
			if (pidR == -1) {
				if (errno == ECHILD) {
					// this should not happen, but if no children then reset numP
					numP = 0;
				}
				else {
					printf("mandelmovie: wait: %s\n\n", strerror(errno));
					exit(1);
				}
			}
			if (status != 0) {
				printf("mandelmovie: ./mandel failed. Exit status = %d.\n", status);
				exit(1);
			}
			--numP;
		}

		int pid = fork();
		if (pid < 0) {
			printf("mandelmovie: fork: %s\n", strerror(errno));
			exit(1);
		}
		else if (pid == 0) {
			//is child
			char cmd[256];
			char *cmdWords[32];
			sprintf(cmd, "./mandel -x .3855 -y .15 -s %lf -m 1500 -W 1360 -H 1360 -o mandel%d.bmp", zoom, i);
			int j;
			cmdWords[0] = strtok(cmd, " ");
			for (j = 1; j < 32; ++j) {
				cmdWords[j] = strtok(0, " ");
				if (cmdWords[j] == NULL) break;
			}

			execvp(cmdWords[0], cmdWords);
			// if get here, there is some error
			printf("mandelmovie: execvp: %s\n", strerror(errno));
			exit(1);
		}
		else {
			// update numP and zoom for next iteration
			++numP;
			zoom *= exp(log(endZoom / zoom)/NUM_FRAMES);

		}
	}

	// wait for the remaining processes
	while (numP > 0) {
		int status;	
		pid_t pidR = wait(&status);
		if (pidR == -1) {
			if (errno == ECHILD) {
				// this should not happen, but if no children then reset numP
				numP = 0;
			}
			else {
				printf("mandelmovie: wait: %s\n\n", strerror(errno));
				exit(1);
			}
		}
		if (status != 0) {
			printf("mandelmovie: ./mandel failed. Exit status = %d.\n", status);
			exit(1);
		}
		--numP;	
	}

	return 0;
}
