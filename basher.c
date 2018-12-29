#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/unistd.h>
#include <string.h>
#include <fcntl.h>

#define TRUE 1
#define FALSE 0
#define MAXARGS 16

#define PERMS 0666

#define STDIN_FILE0 0
#define STDOUT_FILE0 1
#define REDIR_IN 0
#define REDIR_OUT 1
#define NOREDIR 2

int main() {
	char separators[] = {' ', '\n', '<', '>', '\0', '|'};
	while(TRUE) {
		char *argv[MAXARGS + 1];
		char *argvAdditional[MAXARGS + 1]; //Array pointer for 2nd process
		char *string = NULL;
		char *tempPointer;
		int c;
		int memMult = 1;
		int lineIterator = -1;
		int additionalLineIterator = -1;
		unsigned int charPosition = 0;
		int iFd = -1;
		int oFd = -1;
		short alreadySpaced = FALSE;
		short redirectionState = NOREDIR; // State of redirection
		char *iFilename = NULL; // IO File name
		char *oFilename = NULL;
		char *gFilename = NULL; //Generic Filename for 2nd process
		int pipeEnabled = FALSE;
		int pipefd[2];

		putchar('>');

		while(lineIterator < MAXARGS) {
			c = getchar();
			if(c == EOF) {
				putchar('\n');
				return EXIT_SUCCESS;
			}
			if((c == ' ') && alreadySpaced) {
				continue;
			} else if((c != ' ') && (c != '>') && (c != '<') && alreadySpaced) {
				alreadySpaced = FALSE;
			} else if((c == ' ') || (c == '>') || (c == '<')) {
				alreadySpaced = TRUE;
				if(c == '>') {
					redirectionState = REDIR_OUT;
				} else if(c == '<') {
					redirectionState = REDIR_IN;
				}
			}
			if(strrchr(separators, c) == NULL) {
				if(((memMult - 1) == charPosition)) {
					memMult *= sizeof(char) * 2;
					tempPointer = (char*) realloc(string, memMult);
					if(tempPointer == NULL) {
						perror("Error allocation of memory");
						return EXIT_FAILURE;
					}
					string = tempPointer;
					tempPointer = NULL;
				}
				*(string + charPosition) = c;
			} else {
				
				if(charPosition != 0) {
					*(string + charPosition) = '\0';			
					if((redirectionState != NOREDIR) && !alreadySpaced) {
						if(redirectionState == REDIR_OUT) {
							oFilename = string;
						} else if(redirectionState == REDIR_IN) {
							iFilename = string;
						}
						redirectionState = NOREDIR;
					} else {
						if(!pipeEnabled) {
							argv[++lineIterator] = string;
							argv[lineIterator + 1] = NULL;
						} else {
							argvAdditional[++additionalLineIterator] = string;
							argvAdditional[additionalLineIterator + 1] = NULL;
						}
					}
					charPosition = 0;
					memMult = 1;
					string = NULL;
					if(c == '\n') {
						break;
					}
				}
				if (c == '|'){
					pipeEnabled = TRUE;
				} else if (c == '>'){
					redirectionState = REDIR_OUT;
				}else if (c == '<'){
					redirectionState = REDIR_IN;
				}
				continue;
			}
			charPosition++;
		}

		if(pipeEnabled) {
			if(pipe(pipefd) == -1) {
				perror("pipe");
				return EXIT_FAILURE;
			}
		}
		pid_t pid1 = fork();
		if(!pid1) {
			if(pipeEnabled) {
				close(pipefd[0]);
				if(dup2(pipefd[1], STDOUT_FILE0) == -1) {
					perror("dup2 pipefd[0]");
					return EXIT_FAILURE;
				}
			} else {
				if(oFilename != NULL) {
					if((oFd = open(oFilename, O_WRONLY | O_CREAT | O_TRUNC, PERMS)) != -1) {
						if(dup2(oFd, STDOUT_FILE0) == -1) {
							perror("dup2 out");
							return EXIT_FAILURE;
						}
					} else {
						perror("open error");
						return EXIT_FAILURE;
					}
				}
			}


			if(iFilename != NULL) {
				if((iFd = open(iFilename, O_RDONLY)) != -1) {

					if(dup2(iFd, STDIN_FILE0) == -1) {
						perror("dup2 in");
						return EXIT_FAILURE;
					}
				} else {
					perror("open error");
					return EXIT_FAILURE;
				}
			}
			
			if(execvp(*argv, argv) == -1) {
				perror("execvp error");
				return EXIT_FAILURE;
			}
		}
		pid_t pid2;
		if(pipeEnabled) {
			pid2 = fork();
			if(!pid2) {
				if(pipeEnabled) {
					close(pipefd[1]);
					if(dup2(pipefd[0], STDIN_FILE0) == -1) {
						perror("dup2 pipefd[1]");
						return EXIT_FAILURE;
					}
				}
				if(gFilename != NULL) {
					if((oFd = open(gFilename, O_WRONLY | O_CREAT | O_TRUNC, PERMS)) != -1) {
						if(dup2(oFd, STDOUT_FILE0) == -1) {
							perror("dup2 out");
							return EXIT_FAILURE;
						}
					} else {
						perror("open error");
						return EXIT_FAILURE;
					}
				}
				if(execvp(*argvAdditional, argvAdditional) == -1) {
					perror("execvp error");
					return EXIT_FAILURE;
				}
			}
		}
		if(pipeEnabled) {
			close(pipefd[0]);
			close(pipefd[1]);
			wait(NULL);
		}
		wait(NULL);

		for(int i = 0; i < lineIterator + 1; i++) {
			free(argv[i]);
		}
		for(int i = 0; i < additionalLineIterator + 1; i++) {
			free(argvAdditional[i]);
		}
		free(iFilename);
		free(oFilename);
		if(pipeEnabled && (gFilename != NULL)) {
			free(gFilename);
		}
		close(oFd);
		close(iFd);
	}
}
