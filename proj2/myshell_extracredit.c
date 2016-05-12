/* Sam Rack
 * CSE 30341
 * Project 2
 * myshell.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_INPUT_CHARS 4096
#define MAX_WORDS 100

/// BEGIN MAIN ///
int main(int argc, char **argv) {

	char input[MAX_INPUT_CHARS];
	char *words[MAX_WORDS + 1];		//ensure there is a NULL char* at index 100 if a command has 100 words

	
	/// ENTER LOOP FOR SHELL ///
	while (1) {		// note: loop exits on break statements for EOF or exit/quit commands
	
		// used only if there is an input or output redirection 
		char *iFile = NULL;
		char *oFile = NULL;

		//////////////////////
		/// GET USER INPUT ///
		//////////////////////

		// display a prompt
		printf("myshell>> ");
		// flush stdout to ensure the user can see the prompt
		if (fflush(stdout) != 0) {
			printf("myshell: fflush: %s\n", strerror(errno));
			exit(1);
		}

		// read in the command from the user
		if (fgets(input, sizeof(input), stdin) == NULL) {
			if (feof(stdin) != 0) {
				// have reached EOF
				printf("\n\n");

				//// EXITING WHILE LOOP ////
				break;
			}
			else {
				// error with fgets
				printf("myshell: fgets: %s\n", strerror(errno));
				exit(1);
			}
		}


		///////////////////////
		/// PARSE THE INPUT ///
		///////////////////////
		
		// extract the command (first word)
		words[0] = strtok(input, " \t\n");
		if (words[0] == NULL) {
			// the user did not enter anything, so display the prompt again
			continue;
		}

		// split the rest of the input into words
		int numWords = 1;
		int numCmds = 1;
		for ( ; numWords < MAX_WORDS; ++numWords) {
			char *temp = strtok(0, " \t\n");
			
			// strtok returns NULL when no further tokens, so break from the loop
			if (temp == NULL) break;
			// check for outfile
			else if (temp[0] == '>') {
				oFile = strtok(temp, ">");
			} 
			// check for infile 
			else if (temp[0] == '<') {
				iFile = strtok(temp, "<");\
			}
			// otherwise it is just a normal command or argument
			else {
				words[numCmds] = temp;
				++numCmds;
			}
		}
		// check if user has too many input words
		if (numWords == MAX_WORDS) {
			char *dummy = strtok(0, " \t\n");
			if (dummy != NULL) {
				printf("myshell: Too many arguments on the command line. Max is 100.\n");
				continue;
			}
		}

		words[numCmds] = NULL;


		///////////////////////////////
		/// EXECUTE COMMAND ENTERED ///
		///////////////////////////////
		
		/*** start -- begin a process that will run concurrently with the shell ***/
		if (strcmp(words[0], "start") == 0) {
			// check arg count
			if (numWords < 2) {
				printf("myshell: Wrong number of arguments.\n\n");
				printf("Usage: start <program> [<arg1> <arg2> ...]\n\n");
				continue;
			}
			
			// fork the process
			pid_t pid;
			pid = fork();
		
			// error check	
			if (pid < 0) {
				printf("myshell: fork: %s.\n\n", strerror(errno));
				continue;
			}
			// code the child executes
			else if (pid == 0) {
				// if input or output direction, take care of this
				if (iFile != NULL) {
					int iFd = open(iFile, O_RDONLY);
					if (iFd < 0) {
						printf("myshell: open: %s\n\n", strerror(errno));
						continue;
					}
					if (dup2(iFd, STDIN_FILENO) != STDIN_FILENO) {
						printf("myshell: dup2: %s\n\n", strerror(errno));
						continue;	
					}
				
				}
				if (oFile != NULL) {
					int oFd = open(oFile, O_RDWR | O_CREAT);
					if (oFd < 0) {
						printf("myshell: open: %s\n\n", strerror(errno));
						continue;
					}
					if (dup2(oFd, STDOUT_FILENO) != STDOUT_FILENO) {
						printf("myshell: dup2: %s\n\n", strerror(errno));
						continue;
					} 
				}

				// exec call
				if (execvp(words[1], words + 1) < 0) {
					printf("myshell: execvp: %s.\n\n", strerror(errno));
					continue;
				}
			}
			// code the parent executes, pid = child's pid
			else {
				printf("myshell: Process %d started.\n\n", pid);
			}
		}

		/*** wait -- block until a process exits and prints exit status ***/
		else if (strcmp(words[0], "wait") == 0) {
			// check arg count
			if (numWords != 1) {
				printf("myshell: Too many arguments.\n");
				printf("Usage: wait\n\n");
				continue;
			}

			// wait for one process to finish
			pid_t pid;
			int status;
			pid = wait(&status);
			
			// error - either no processes left or other wait() error
			if (pid == -1) {
				if (errno == ECHILD) {
					printf("myshell: No processes left.\n\n");
				}
				else {
					printf("myshell: wait: %s\n\n", strerror(errno));
				}
			}
			// wait() successful
			else if (status == 0) printf("myshell: Process %d exited normally with status %d.\n\n", pid, status);
			else printf("myshell: Process %d exited abnormallly with status %d: %s.\n\n", pid, status, strsignal(status));
		}

		/*** run -- combination fo start and wait, process runs in fg ***/
		else if (strcmp(words[0], "run") == 0) {
			// check arg count
			if (numWords < 2) {
				printf("myshell: Wrong number of arguments.\n");
				printf("Usage: run <program> [<arg1> <arg2> ...]\n\n");
				continue;
			}

			// fork the process
			pid_t pid;
			pid = fork();

			// error check
			if (pid < 0) {
				printf("myshell: fork: %s.\n\n", strerror(errno));
				continue;
			}
			// code the child executes
			else if (pid == 0) {
				// if input or output direction, take care of this
				if (iFile != NULL) {
					int iFd = open(iFile, O_RDONLY);
					if (iFd < 0) {
						printf("myshell: open: %s\n\n", strerror(errno));
						continue;
					}
					if (dup2(iFd, STDIN_FILENO) != STDIN_FILENO) {
						printf("myshell: dup2: %s\n\n", strerror(errno));
						continue;	
					}
				
				}
				if (oFile != NULL) {
					int oFd = open(oFile, O_RDWR | O_CREAT | O_APPEND);
					if (oFd < 0) {
						printf("myshell: open: %s\n\n", strerror(errno));
						continue;
					}
					if (dup2(oFd, STDOUT_FILENO) != STDOUT_FILENO) {
						printf("myshell: dup2: %s\n\n", strerror(errno));
						continue;
					} 
				}

				// exec call
				if (execvp(words[1], words + 1) < 0) {
					printf("myshell: execvp: %s.\n\n", strerror(errno));
					continue;	
				}
			}
			// code the parent executes
			else {
				int status = 0;
				// wait and update status 
				if (waitpid(pid, &status, 0) != pid) {
					printf("myshell: waitpid: %s.\n\n", strerror(errno));
				}										

				if (status == 0) printf("myshell: Process %d exited normally with status %d.\n\n", pid, status);
				else printf("myshell: Process %d exited abnormally with status %d: %s.\n\n", pid, status, strsignal(status));
			}
		}

		/*** kill -- ends a process, pid of process as arg ***/
		else if (strcmp(words[0], "kill") == 0) {
			// check arg count
			if (numWords != 2) {
				printf("myshell: Wrong number of arguments.\n");
				printf("Usage: kill <pid>\n\n");
				continue;
			}

			// is the pid valid? atoi is undefined on a nonnumber string
			int i;
			int validPid = 1;
			for (i =0 ; words[1][i] != '\0'; ++i) {
				if (!isdigit(words[1][i])) {
					validPid = 0;
					printf("myshell: Invalid argument: pid must be an integer.\n");
					printf("Usage: kill <pid>\n\n");
					break;
				}
			}
			if (!validPid) continue;

			// use sigkill
			if (kill((pid_t)(atoi(words[1])), SIGKILL) != 0) {
				printf("myshell: kill: %s.\n\n", strerror(errno));
			}
			else {
				// successfully killed
				printf("myshell: Process %s killed.\n\n", words[1]);
			}
		}

		/*** stop -- stops a process, pid as arg ***/
		else if (strcmp(words[0], "stop") == 0) {
			// check arg count
			if (numWords != 2) {
				printf("myshell: Wrong number of arguments.\n");
				printf("Usage: stop <pid>\n\n");
				continue;
			}

			// is the pid valid? atoi is undefined on a nonnumber string
			int i;
			int validPid = 1;
			for (i =0 ; words[1][i] != '\0'; ++i) {
				if (!isdigit(words[1][i])) {
					validPid = 0;
					printf("myshell: Invalid argument: pid must be an integer.\n");
					printf("Usage: stop <pid>\n\n");
					break;
				}
			}
			if (!validPid) continue;

			// use sigstop
			if (kill((pid_t)(atoi(words[1])), SIGSTOP) != 0) {
				printf("myshell: stop: %s.\n\n", strerror(errno));
			}
			else {
				// successfully stopped
				printf("myshell: Process %s stopped.\n\n", words[1]);
			}

		}

		/*** continue -- resumes a stopped process, pid as arg ***/
		else if (strcmp(words[0], "continue") == 0) {
			// check arg count
			if (numWords != 2) {
				printf("myshell: Wrong number of arguments.\n");
				printf("Usage: continue <pid>\n\n");
				continue;
			}

			// is the pid valid? atoi is undefined on a nonnumber string
			int i;
			int validPid = 1;
			for (i =0 ; words[1][i] != '\0'; ++i) {
				if (!isdigit(words[1][i])) {
					validPid = 0;
					printf("myshell: Invalid argument: pid must be an integer.\n");
					printf("Usage: continue <pid>\n\n");
					break;
				}
			}
			if (!validPid) continue;

			// use sigcont
			if (kill((pid_t)(atoi(words[1])), SIGCONT) != 0) {
				printf("myshell: continue: %s.\n\n", strerror(errno));
			}
			else {
				// successfully continued
				printf("myshell: Process %s continued.\n\n", words[1]);
			}

		}

		/*** quit/exit -- exits the program ***/
		else if (strcmp(words[0], "quit") == 0 || strcmp(words[0], "exit") == 0) {
			// check arg count
			if (numWords != 1) {
				printf("myshell: Too many arguments.\n");
				printf("Usage: %s\n\n", words[0]);
				continue;
			}

			//// EXITING WHILE LOOP ////
			break;	
		}

		// if none of the above, the user entered an invalid command
		else {
			printf("myshell: unknown command: %s\n\n", words[0]);
		}

	}

	return 0;
}
