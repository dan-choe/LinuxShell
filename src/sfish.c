#include "sfish.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <wait.h>
#include <ucontext.h>

char currPath[maxLength-1]; // current PWD
int isPipe = 0;
int argumetsArraySize = 0;
int s_commandArrIndex = 0;	// for pipe cmd (ex: cmd1 | cmd2 | cmd3 ... )
char* new_commandArr[999 * sizeof(char)]; // make token from original cmd

int killID;	// only if it is gotten from kill signal from prompt

int parseCMD(char* cmd){

	currentPID = getpid();
	//printf("currentPID : %d\n", currentPID);

	removedSpaceCMD = (char*) calloc(maxLength, sizeof(char));
	removeleadingEndingSpace(cmd);
	//printf("removed all leading and ending space : %s\n", removedSpaceCMD);

	int hasOperatorResult = hasOperator(removedSpaceCMD);
	char* saveCMDPtr = removedSpaceCMD;

	int foundENDString = 0;
	int c = 0;
	char* new_commandArr[1000];
	char* tempBuffsingleCMD = (char*) calloc(255, sizeof(char));
	char* tempBuffsingleCMDPtr = tempBuffsingleCMD;

  	while(*saveCMDPtr != '+' && *saveCMDPtr != 0x00) {
    	if( *(saveCMDPtr) == ' ' )	{
      		if((saveCMDPtr+1) != NULL && *(saveCMDPtr+1) == ' '){
		        while(*(saveCMDPtr) == ' ' && *saveCMDPtr != 0x00 ){
		          saveCMDPtr++;
		        }
		        saveCMDPtr--;
	    	}
			new_commandArr[c] = (char*) calloc((int)(strlen(tempBuffsingleCMD) + 1), sizeof(char));
			strcpy(new_commandArr[c], tempBuffsingleCMD);
			memset(tempBuffsingleCMD,'\0',strlen(tempBuffsingleCMD));
			tempBuffsingleCMDPtr = tempBuffsingleCMD;
			c++;
	    	}else{
	      		if( (*(saveCMDPtr) == 34 || *(saveCMDPtr) == 39) ){
	        		saveCMDPtr++;
		        while(   *saveCMDPtr != '+' && (*(saveCMDPtr) != 34 && *(saveCMDPtr) != 39) ){
		          *(tempBuffsingleCMDPtr++) = *saveCMDPtr;
		          saveCMDPtr++;
		        }
		        saveCMDPtr++;
		        if( *(saveCMDPtr) == ' ' || *saveCMDPtr == '+'){
					if(*saveCMDPtr == '+')
						foundENDString = 1;
					if((saveCMDPtr+1) != NULL && *(saveCMDPtr+1) == ' '){
						while(*(saveCMDPtr) == ' ' && *saveCMDPtr != 0x00 ){
					  		saveCMDPtr++;
						}
						saveCMDPtr--;
					}
					new_commandArr[c] = (char*) calloc((int)(strlen(tempBuffsingleCMD) + 1), sizeof(char));
					strcpy(new_commandArr[c], tempBuffsingleCMD);
					memset(tempBuffsingleCMD,'\0',strlen(tempBuffsingleCMD));
					tempBuffsingleCMDPtr = tempBuffsingleCMD;
					c++;
		        }else{
					if(saveCMDPtr != NULL && *saveCMDPtr != '+'){
						*(tempBuffsingleCMDPtr++) = *saveCMDPtr;
					}
		        }
  		}else{
	        if(saveCMDPtr != NULL && *saveCMDPtr != '+'){
	          *(tempBuffsingleCMDPtr++) = *saveCMDPtr;
	        }
	        if(*saveCMDPtr == '+')
	            foundENDString = 1;
	      	}
		}
    	saveCMDPtr++;
  	}
  	//free(saveCMD);

	if(!foundENDString){
		new_commandArr[c] = (char*) calloc((int)(strlen(tempBuffsingleCMD) + 1), sizeof(char));
		strncpy(new_commandArr[c], tempBuffsingleCMD, strlen(tempBuffsingleCMD));
		memset(tempBuffsingleCMD,'\0',strlen(tempBuffsingleCMD));
		tempBuffsingleCMDPtr = tempBuffsingleCMD;
		c++;
  	}
	new_commandArr[c] = NULL;
	free(tempBuffsingleCMD);
	//printf( "total token: %d hasOperatorResult: %d \n\n", c, hasOperatorResult);

	if(c == 1){
		if((strcmp(cmd, "alarm")) == 0){
			fprintf(stderr, "alarm : arguments must be number as seconds\n");
			fflush(stderr);
			//alarm(1);
		}
		else if((strcmp(cmd, "help")) == 0){
			builtin_help();
		}
		else if((strcmp(cmd, "exit")) == 0){
			builtin_exit();
		}
		else if((strcmp(cmd, "pwd")) == 0){
			builtin_pwd();
		}else if((strcmp(cmd, "cd")) == 0){
			//char* oldPwd = getenv("OLDPWD");
			setenv("OLDPWD", getCurrPwd(), 1);
			builtin_cd();
			//chdir(currHome);
			updatePath();
		}else if((strcmp(cmd, "kill")) == 0){
			fprintf(stderr, "%s\n", "usage: kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... or kill -l [sigspec]");
			fflush(stderr);
		}else{
			int hasSlashResult = hasSlash(cmd);
			if( !hasOperatorResult && hasSlashResult){ 			//   /bin/ls
				char* fullTokenPath = (char*) calloc(maxLength, sizeof(char));
				strcpy(fullTokenPath, cmd);
				struct stat sb;
				if (stat(cmd, &sb) == -1) {
					perror("stat");
	          	}else{
	          		char* argvList[] = {cmd, NULL};
	          		//execv(cmd, argvList);
	          		forkExecArgvWithoutEnv(cmd, argvList);
	            }
	            free(fullTokenPath);
			}else if( !hasOperatorResult && !hasSlashResult){	//   ls
				char* argvList[] = {cmd, NULL};
				forkExecArgv(cmd, argvList);
			}
		}
	}

	if( c > 1 ) {
		argumetsArraySize = c;
		isPipe = hasPipe(new_commandArr, argumetsArraySize); // return number of |
		//printf("c > 1 isPipe %d \n", isPipe);

		if( (strcmp(new_commandArr[0], "alarm")) == 0 ){
			//printf("alarm requested. new_commandArr[1] %s\n", new_commandArr[1]);
			alarmTime = charToDecimal(new_commandArr[1], strlen(new_commandArr[1]));
			//printf("alarm requested. time: %i second\n", alarmTime);
			alarm(alarmTime);
		}
		else if( (strcmp(new_commandArr[0], "kill")) == 0 ){
			if( (strcmp(new_commandArr[1], "-s")) == 0 ){
				if(!new_commandArr[2]){
					fprintf(stderr, "%s\n", "kill: -s: option requires an argument");
					fflush(stderr);
				}else if( (strcmp(new_commandArr[2], "SIGUSR2")) == 0 || (strcmp(new_commandArr[2], "USR2")) == 0 ){
					if(new_commandArr[3]){
						int result = isAllNumber(new_commandArr[3], (int)strlen(new_commandArr[3]));
						if(result<=0){
							fprintf(stderr, "kill: %s : %s\n", new_commandArr[3], "arguments must be process or job IDs");
							fflush(stderr);
						}else{
							killID = charToDecimal(new_commandArr[3], (int)strlen(new_commandArr[3]));
							killSIGUSR2();
						}
					}else{
						fprintf(stderr, "%s\n", "usage: kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... or kill -l [sigspec]");
						fflush(stderr);
					}
				}else {
					fprintf(stderr, "%s\n", "usage: kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... or kill -l [sigspec]");
					fflush(stderr);
				}
			}else if ( (strcmp(new_commandArr[1], "-9")) == 0 ){
				if(!new_commandArr[2]){
					fprintf(stderr, "%s\n", "usage: kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... or kill -l [sigspec]");
					fflush(stderr);
				}else {
					if(new_commandArr[2]){
						int result = isAllNumber(new_commandArr[2], (int)strlen(new_commandArr[2]));
						if(result<=0){
							fprintf(stderr, "kill: %s : %s\n", new_commandArr[2], "arguments must be process or job IDs");
							fflush(stderr);
						}else{
							killID = charToDecimal(new_commandArr[2], (int)strlen(new_commandArr[2]));
							killSIGUSR2();
						}
					}else{
						fprintf(stderr, "%s\n", "usage: kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... or kill -l [sigspec]");
						fflush(stderr);
					}
				}
			}else{
				fprintf(stderr, "kill: %s : %s\n", new_commandArr[1], "invalid signal specification");
				fflush(stderr);
			}
		}
		else if(!hasOperatorResult && !isPipe && (*(new_commandArr[0]) == '/' || *(new_commandArr[0]) == '.')){
			//printf("argumetsArray[0] has slash. argumetsArray[0]: %s, c: %d\n", new_commandArr[0], argumetsArraySize);
			pid_t pid = fork();
			if(pid == 0){
				//currentPID =  getpid();
				char* argumetsCat[argumetsArraySize+1];
				int indexargumetsCat = 0;
				while(argumetsArraySize != 0) {
					argumetsCat[indexargumetsCat] = new_commandArr[indexargumetsCat];
					//printf("argumetsCat: %s\n", argumetsCat[indexargumetsCat]);
					argumetsArraySize--;
					indexargumetsCat++;
					//token = strtok(NULL, space);
				}
				argumetsCat[indexargumetsCat] = NULL;
				struct stat sb;
				if (stat(argumetsCat[0], &sb) == -1) {
					perror("stat");
	          	}else{
	          		execv(argumetsCat[0], argumetsCat);
	          	}
				//_exit(0);
			}else{
				waitpid(pid,NULL,0);
				currentPID = pid;
			}
		}

		else if( (strcmp(new_commandArr[0], "cd")) == 0 ){
			if( (strcmp(new_commandArr[1], "-")) == 0 ){
				char* oldPwd = getenv("OLDPWD");
				setenv("OLDPWD", getCurrPwd(), 1);
				chdir(oldPwd);
				updatePath();
			}else if( (strcmp(new_commandArr[1], "..")) == 0 ){
				setenv("OLDPWD", getCurrPwd(), 1);
				chdir(getParentPath());
				updatePath();
			}else if( (strcmp(new_commandArr[1], ".")) == 0 ){
			}else{
				char* result = NULL;
				if(((result = cdPath(new_commandArr[1]))) == NULL){
					fprintf(stderr, "Warning: cd Path Error, token: %s\n", new_commandArr[1]);
					fflush(stderr);
				}else{
					//fprintf(stdout, "CD Path moved. token: %s\n", new_commandArr[1]);
					//fflush(stderr);
				}
			}
		}

		else if( (strcmp(new_commandArr[0], "ls")) != 0 || (strcmp(new_commandArr[0], "ls")) == 0 || (strcmp(new_commandArr[0], "cat")) == 0 || (strcmp(new_commandArr[0], "echo")) == 0 || (strcmp(new_commandArr[0], "grep")) == 0 ){
			int hasSlashResult = hasSlash(new_commandArr[1]);

			if( (strcmp(new_commandArr[0], "grep")) != 0 &&  (strcmp(new_commandArr[0], "echo")) != 0 && !hasOperatorResult && (hasSlashResult || (!hasSlashResult))){ // ls bin/foo.txt
				char* fullTokenPath = (char*) calloc(maxLength, sizeof(char));
				strcpy(fullTokenPath, new_commandArr[1]);
				struct stat sb;
				if (stat(new_commandArr[1], &sb) == -1) {
					perror("stat");
	          	}else{
	          		forkExecArgv(new_commandArr[0], new_commandArr);
	            }
	            free(fullTokenPath);
			}
			else if( (strcmp(new_commandArr[0], "grep")) == 0 && !hasOperatorResult && (hasSlashResult || (!hasSlashResult))){ // grep "f" bin/foo.txt
				forkExecArgv(new_commandArr[0], new_commandArr);
			}
			else{	// Redirection
				//printf("Redirection - hasOperatorResult (Operator num : %d) (argumetsArraySize : %d)\n", hasOperatorResult, argumetsArraySize);

				// CHECK N> CASES ///////////////////////////////////////////////////////////////////////////////////////
				int hasDupRedirectionResult = -1;
				//int indexOfDupRedirecton = -1;

				for(int i=0; i<argumetsArraySize; i++){
					hasDupRedirectionResult = hasDuplicateRedirection(new_commandArr[i]);
					if(hasDupRedirectionResult>=0){
						break;
					}
				}

				int hasAppendStdoutResult = -1;
				int hasAppendStdInResult = -1;

				for(int i=0; i<argumetsArraySize; i++){
					hasAppendStdoutResult = hasAppendStdout(new_commandArr[i]);
					if(hasAppendStdoutResult>=0){
						break;
					}
				}

				for(int i=0; i<argumetsArraySize; i++){
					hasAppendStdInResult = hasAppendStdIn(new_commandArr[i]);
					if(hasAppendStdInResult>=0){
						break;
					}
				}

				int hasSignArrowStdOutResult = -1;
				for(int i=0; i<argumetsArraySize; i++){
					hasSignArrowStdOutResult = hasSignArrowStdOut(new_commandArr[i]);
					if(hasSignArrowStdOutResult>=0){
						break;
					}
				}

				// printf("CHECK N> CASES: hasDupRedirectionResult: %d, hasDupRedirectionResult: %d\n", hasDupRedirectionResult, indexOfDupRedirecton);
				// printf("CHECK N> CASES: hasAppendStdoutResult: %d, hasAppendStdInResult: %d\n", hasAppendStdoutResult, hasAppendStdInResult);
				// printf("CHECK N> CASES: hasSignArrowStdOutResult: %d\n", hasSignArrowStdOutResult);
				////////////////////////////////////////////////////////////////////////////////////////////////////////

				// NORMAL CASE
				if(hasDupRedirectionResult < 0 && hasAppendStdoutResult < 0 && hasAppendStdInResult < 0 && hasSignArrowStdOutResult < 0) {

					if(argumetsArraySize < 3 && hasOperatorResult){ // ls > bin/foo.txt
						//printf("(0 : %s) (1 : %s) (2 : %s)\n", new_commandArr[0], new_commandArr[1], new_commandArr[2]);
						if(strcmp(new_commandArr[1],">") == 0){
							int outFD  = open(new_commandArr[2], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
							if(outFD == -1){
			            		fprintf(stderr, "%s: file can not make or found\n", new_commandArr[2]);
			            		fflush(stderr);
								exit(EXIT_FAILURE);
								return -1;
			            	}
							pid_t pid = fork();
							if(pid == 0){
								if(dup2(outFD,STDOUT_FILENO) == -1){
									return -1;
								}
								if(outFD != STDOUT_FILENO){
									close(outFD);
								}
	  							char* argvList[] = {new_commandArr[0], NULL};
	  							if(*(new_commandArr[0]) == '/' || *(new_commandArr[0]) == '.'){
	  								struct stat sb;
	  								if (stat(new_commandArr[1], &sb) == -1) {
										perror("stat");
						          	}else{
						          		execv(new_commandArr[0], argvList);
						            }
	  							}else{
	  								forkExecArgv(new_commandArr[0], argvList);
	  							}
								//_exit(0);
							}else{
								waitpid(pid,NULL,0);
								currentPID = pid;
							}
						}
					}
					else if(argumetsArraySize < 3 && !hasOperatorResult){ // echo "hello"   ls bin
						char* argvList[] = {new_commandArr[0], new_commandArr[1], NULL};
						forkExecArgv(new_commandArr[0], argvList);
					}
					else if(argumetsArraySize >= 3){
						int inFD = STDIN_FILENO, outFD = STDOUT_FILENO;

						isPipe = hasPipe(new_commandArr, argumetsArraySize); // return number of |
						//printf("isPipe %d inFD: %d, outFD: %d\n", isPipe, inFD, outFD);

						if(isPipe) {
							int pipefds[2*isPipe];
							for(int i = 0; i < isPipe; i++) {
								if(pipe(pipefds + i*2) < 0) {
									perror("pipe");
									exit(EXIT_FAILURE);
								}
							}

							// for(int i = 0; i < isPipe*2; i++) {
							// 	printf("pipefds[%d] %d \n", i, pipefds[i]);
							// }

							s_commandArrIndex = 0;
							addCMDStructCutByPipe(new_commandArr);

			            	int zz = 1;
							int jj = 0;
							while(jj < isPipe*2) {
								if(s_commandArr[zz]->inputfile != 0x00){
									close(pipefds[jj]);
					            	pipefds[jj] = open(s_commandArr[zz]->inputfile, O_RDONLY);

					            	if(pipefds[jj] == -1){
					            		fprintf(stderr, "%s: file not found\n", s_commandArr[zz]->inputfile);
					            		fflush(stderr);
										exit(EXIT_FAILURE);
										return 0;
					            	}
								}
				            	if(s_commandArr[zz]->outputfile != 0x00){
				            		close(pipefds[jj+1]);
				            		pipefds[jj+1] = open(s_commandArr[zz]->outputfile, O_WRONLY | O_TRUNC | O_CREAT, S_IRGRP | S_IWUSR | S_IRUSR | S_IWGRP);
				            		if(pipefds[jj+1] == -1){
					            		fprintf(stderr, "%s: file can not make or found\n", s_commandArr[zz]->outputfile);
					            		fflush(stderr);
										exit(EXIT_FAILURE);
										return -1;
						            }
				            	}
				            	jj+=2;
				            	zz++;
							}

							// for(int i = 0; i < isPipe*2; i++) {
							// 	printf("pipefds[%d] %d \n", i, pipefds[i]);
							// }

							int i = 0, j = 0, command = isPipe+1, curr_cmd = 0;
							pid_t pid;

						    while(curr_cmd<command) {
					    		inFD = STDIN_FILENO;
								outFD = STDOUT_FILENO;

						        pid = fork();
						        if(pid == 0) {
						        	if(curr_cmd==0){
						        		if(s_commandArr[0]->inputfile != 0x00){
							            	inFD = open(s_commandArr[0]->inputfile, O_RDONLY);
							            	printf("\ninFD : %d \n", inFD);

							            	if(inFD == -1){
							            		fprintf(stderr, "%s: file not found\n", s_commandArr[0]->inputfile);
							            		fflush(stderr);
												exit(EXIT_FAILURE);
												return 0;
							            	}

							            	dup2(inFD,STDIN_FILENO);

							            	if(dup2(inFD,STDIN_FILENO) == -1){
												perror("dup2");
												//printf("\nerror1 inFD : %d curr_cmd: %d, s_commandArr[0]->inputfile: %s\n", inFD, curr_cmd, s_commandArr[0]->inputfile);
											}
											/////////// QUESTION : IT KEEPS inFD!=STDIN_FILENO
							            	if(inFD!=STDIN_FILENO){
							            		//printf("\nerror2 inFD : %d curr_cmd: %d, s_commandArr[0]->inputfile: %s\n", inFD, curr_cmd, s_commandArr[0]->inputfile);
												dup2(inFD,STDIN_FILENO);
									        	close(inFD);
									        }

									        if(inFD!=STDIN_FILENO){
							            		//printf("\nerror3 inFD : %d curr_cmd: %d, s_commandArr[0]->inputfile: %s\n", inFD, curr_cmd, s_commandArr[0]->inputfile);
												dup2(inFD,STDIN_FILENO);
									        	close(inFD);
									        }
										}
						            	if(s_commandArr[0]->outputfile != 0x00){
						            		outFD = open(s_commandArr[0]->outputfile, O_WRONLY | O_TRUNC | O_CREAT, S_IRGRP | S_IWUSR | S_IRUSR | S_IWGRP);
						            		if(outFD == -1){
							            		//fprintf(stderr, "%s: file can not make or found\n", s_commandArr[0]->outputfile);
												exit(EXIT_FAILURE);
												return -1;
								            }
						            		dup2(outFD,STDOUT_FILENO);
						            		close(outFD);
						            	}else{
						            		dup2(pipefds[j + 1],STDOUT_FILENO);
						            	}
									}

						            if(curr_cmd && curr_cmd<(command-1)){ //if not last command
						            	if(dup2(pipefds[j + 1], STDOUT_FILENO) < 0){
						                	// fprintf(stderr,"%s  j+1 %d curr_cmd %d\n", "dup2 1\n", j+1, curr_cmd);
						                	// fflush(stderr);
						                    perror("dup2");
						                    exit(EXIT_FAILURE);
						                }
						            }
						            if( j != 0 ){ //if not first command
						            	if(dup2(pipefds[j-2], STDIN_FILENO) < 0){
						                	// fprintf(stderr,"%s  j+1 %d curr_cmd %d\n", "dup2 2\n", j+1, curr_cmd);
						                	// fflush(stderr);
						                    perror("dup2");
						                    exit(EXIT_FAILURE);
						                }
						            }

							        if(inFD!=STDIN_FILENO){
							        	dup2(inFD,STDIN_FILENO);
							        	close(inFD);
							        }
							        if(outFD!=STDOUT_FILENO){
							        	dup2(outFD,STDOUT_FILENO);
							        	close(outFD);
							        }
						            for(i = 0; i < 2*isPipe; i++){
						                close(pipefds[i]);
						            }

				            		withoutforkExecArgv(s_commandArr[curr_cmd]->newArgList[0], s_commandArr[curr_cmd]->newArgList);
				            		s_commandArr[curr_cmd]->executed = 1;

						        } else if(pid < 0){
						            perror("error");
						            exit(EXIT_FAILURE);
						        }
						        else{
						        	j+=2;
						        	currentPID = pid;
						        }
						        curr_cmd++;
						    }

						    for(int i = 0; i < 2 * isPipe; i++){
								close(pipefds[i]);
							}
							for(int i = 0; i < isPipe+1; i++){
								waitpid(pid,NULL,0);
							}
							currentPID = pid;
							freeCMDStruct();
						} // end if(isPipe)

						else if(!isPipe){
							//printf(" (!isPipe) hasOperatorResult : %d \n", hasOperatorResult);
							if(hasOperatorResult){
								char* argvList[argumetsArraySize];
								int outFD  = 1, inFD = 0;
								int saveCMDPosition = 0;
								for(int i=0; i<argumetsArraySize; i++){
									if(*new_commandArr[i] == '<'){
										inFD  = open(new_commandArr[i+1], O_RDONLY);

										if(inFD == -1){
						            		fprintf(stderr, "%s: file not found\n", new_commandArr[i+1]);
						            		fflush(stderr);
											exit(EXIT_FAILURE);
											return -1;
						            	}

										i++;
										argvList[saveCMDPosition] = new_commandArr[i];
									}else if(*new_commandArr[i] == '>'){

										if(fileExists(new_commandArr[i+1]) == -1){
											perror("stat");
										}else{
											outFD  = open(new_commandArr[i+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRGRP | S_IWUSR | S_IRUSR | S_IWGRP);
											i++;
										}
										//printf("outFD: %d, new_commandArr[i+1] : %s \n", outFD, new_commandArr[i]);
										if(outFD == -1 || outFD==1){
						            		fprintf(stderr, "%s: file can not make or found\n", new_commandArr[i]);
						            		fflush(stderr);
											exit(EXIT_FAILURE);
											return -1;
						            	}
									}else{
										argvList[saveCMDPosition] = new_commandArr[i];
										saveCMDPosition++;
									}
								}
								argvList[saveCMDPosition] = NULL;

								pid_t pid = fork();
								if(pid == 0){
									if(dup2(outFD,STDOUT_FILENO) == -1){
										return -1;
									}
									if(outFD != STDOUT_FILENO){
										dup2(outFD,STDOUT_FILENO);
										close(outFD);
									}
									if(dup2(inFD,STDIN_FILENO) == -1){
										return -1;
									}
									if(inFD != STDIN_FILENO){
										dup2(inFD,STDIN_FILENO);
										close(inFD);
									}
									if(*new_commandArr[0] == '.' || *new_commandArr[0] == '/'){
										//forkExecArgvWithoutEnv(new_commandArr[0], argvList);
										struct stat sb;
										if (stat(new_commandArr[0], &sb) == -1) {
											perror("stat");
								      	}else{
								      		execv(new_commandArr[0], argvList);
								      		perror("exec");
								      		exit(1);
								        }
									}else{
										withoutforkExecArgv(new_commandArr[0], argvList);
									}
								//_exit(0);
								}else{
									waitpid(pid,NULL,0);
									currentPID = pid;
								}
							}
						} // end else if(!isPipe)

					}
				}// END NORMAL CASE
				else if(hasDupRedirectionResult >= 0 && hasAppendStdoutResult < 0 && hasAppendStdInResult < 0 && hasSignArrowStdOutResult < 0){
					//printf("case 1\n");
					if(argumetsArraySize <= 3 && hasOperatorResult) { // ls 2> out.txt
						if(strcmp(new_commandArr[1],"0>") == 0 || strcmp(new_commandArr[1],"1>") == 0 || strcmp(new_commandArr[1],"2>") == 0 || strcmp(new_commandArr[1],"3>") == 0 || strcmp(new_commandArr[1],"4>") == 0 || strcmp(new_commandArr[1],"5>") == 0 || strcmp(new_commandArr[1],"6>") == 0 || strcmp(new_commandArr[1],"7>") == 0 || strcmp(new_commandArr[1],"8>") == 0 || strcmp(new_commandArr[1],"9>") == 0){
							int outFD = 1;
							if( strcmp(new_commandArr[1],"3>") == 0 || strcmp(new_commandArr[1],"4>") == 0 || strcmp(new_commandArr[1],"5>") == 0 || strcmp(new_commandArr[1],"6>") == 0 || strcmp(new_commandArr[1],"7>") == 0 || strcmp(new_commandArr[1],"8>") == 0 || strcmp(new_commandArr[1],"9>") == 0){
								outFD  = open(new_commandArr[2], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);

								if(outFD == -1){
				            		fprintf(stderr, "%s: file can not make or found\n", new_commandArr[2]);
				            		fflush(stderr);
									exit(EXIT_FAILURE);
									return -1;
				            	}
				            	if( (*(new_commandArr[1]) -'0') != outFD){
				            		close(outFD);
				            	}else{
				            		hasDupRedirectionResult = outFD;
				            	}

							}else if(strcmp(new_commandArr[1],"1>") == 0){
								hasDupRedirectionResult = open(new_commandArr[2], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);

								//hasDupRedirectionResult = open(new_commandArr[2], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
							}else if(strcmp(new_commandArr[1],"2>") == 0 ){
								outFD = 2;
							}else if(strcmp(new_commandArr[0],"0>") == 0){
								hasDupRedirectionResult = open(new_commandArr[2], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
							}

							pid_t pid = fork();
							if(pid == 0){
								if(dup2(hasDupRedirectionResult,STDOUT_FILENO) == -1){
									return -1;
								}
								if(hasDupRedirectionResult != STDOUT_FILENO){
									close(hasDupRedirectionResult);
								}
	  							char* argvList[] = {new_commandArr[0], NULL};
	  							if(*(new_commandArr[0]) == '/' || *(new_commandArr[0]) == '.'){
	  								struct stat sb;
	  								if (stat(new_commandArr[1], &sb) == -1) {
										perror("stat");
						          	}else{
						          		execv(new_commandArr[0], argvList);
						            }
	  							}else{
	  								withoutforkExecArgv(new_commandArr[0], argvList);
	  							}

	  							outFD  = open(new_commandArr[2], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
								if(outFD == -1){
				            		fprintf(stderr, "%s: file can not make or found\n", new_commandArr[2]);
				            		fflush(stderr);
									exit(EXIT_FAILURE);
									return -1;
				            	}
				            	// dup2(outFD,STDOUT_FILENO);
				            	// write(STDOUT_FILENO, "redirect! \n", 10);
				            	// close(outFD);
				            	//
							}else{
								waitpid(pid,NULL,0);
								currentPID = pid;
							}
						}
					}else if(argumetsArraySize > 3){
						isPipe = hasPipe(new_commandArr, argumetsArraySize); // return number of |
						if(!isPipe){
							if(hasOperatorResult){
								char* argvList[argumetsArraySize];
								int outFD  = 1, inFD = 0;
								int saveCMDPosition = 0;
								for(int i=0; i<argumetsArraySize; i++){
									if(*new_commandArr[i] == '<'){
										inFD  = open(new_commandArr[i+1], O_RDONLY);
										if(inFD == -1){
						            		fprintf(stderr, "%s: file not found\n", new_commandArr[i+1]);
						            		fflush(stderr);
											exit(EXIT_FAILURE);
											return -1;
						            	}
										i++;
										argvList[saveCMDPosition] = new_commandArr[i];
									}else if( strcmp(new_commandArr[i],"0>") == 0 || strcmp(new_commandArr[i],"1>") == 0 || strcmp(new_commandArr[i],"2>") == 0 || strcmp(new_commandArr[i],"3>") == 0 || strcmp(new_commandArr[i],"4>") == 0 || strcmp(new_commandArr[i],"5>") == 0 || strcmp(new_commandArr[i],"6>") == 0 || strcmp(new_commandArr[i],"7>") == 0 || strcmp(new_commandArr[i],"8>") == 0 || strcmp(new_commandArr[i],"9>") == 0 ){
										if(fileExists(new_commandArr[i+1]) == -1){
											perror("stat");
										}else{
											if(strcmp(new_commandArr[i],"1>") == 0){
												hasDupRedirectionResult = open(new_commandArr[i+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
												i++;
												//fprintf(stderr, "hasDupRedirectionResult %d\n", hasDupRedirectionResult);
											}else if(strcmp(new_commandArr[i],"2>") == 0){
												hasDupRedirectionResult = 2;
												i++;
											}else if(strcmp(new_commandArr[i],"0>") == 0){
												hasDupRedirectionResult = 0;
												i++;
											}else{
												outFD  = open(new_commandArr[i+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
												if(outFD == -1){
								            		fprintf(stderr, "%s: file can not make or found\n", new_commandArr[2]);
								            		fflush(stderr);
													exit(EXIT_FAILURE);
													return -1;
								            	}
								            	if( (*(new_commandArr[i]) -'0') != outFD){
								            		close(outFD);
								            	}else{
								            		hasDupRedirectionResult = outFD;
								            	}
								            	i++;
											}
										}
						            	outFD = hasDupRedirectionResult;
									}else{
										argvList[saveCMDPosition] = new_commandArr[i];
										saveCMDPosition++;
									}
								}
								argvList[saveCMDPosition] = NULL;

								pid_t pid = fork();
								if(pid == 0){
									if(dup2(outFD,STDOUT_FILENO) == -1){
										return -1;
									}
									if(outFD != STDOUT_FILENO){
										dup2(outFD,STDOUT_FILENO);
										close(outFD);
									}
									if(dup2(inFD,STDIN_FILENO) == -1){
										return -1;
									}
									if(inFD != STDIN_FILENO){
										dup2(inFD,STDIN_FILENO);
										close(inFD);
									}
									if(*new_commandArr[0] == '.' || *new_commandArr[0] == '/'){
										struct stat sb;
										if (stat(new_commandArr[0], &sb) == -1) {
											perror("stat");
								      	}else{
								      		execv(new_commandArr[0], argvList);
								      		perror("exec");
								      		exit(1);
								        }
									}else{
										withoutforkExecArgv(new_commandArr[0], argvList);
									}
								}else{
									waitpid(pid,NULL,0);
									currentPID = pid;
								}
							}
						} // end else if(!isPipe)
					} // else if(argumetsArraySize >= 3)

				} //has only DupRedirectionResult
				else if(hasDupRedirectionResult < 1 && hasAppendStdoutResult > 0 && hasAppendStdInResult < 0 && hasSignArrowStdOutResult < 0){
					if(argumetsArraySize <= 3) { // ls >> out.txt
						if(strcmp(new_commandArr[1],">>") == 0 ){
							int outFD  = open(new_commandArr[2], O_APPEND | O_WRONLY | O_CREAT , S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
							if(outFD == -1){
			            		fprintf(stderr, "%s: file can not make or found\n", new_commandArr[2]);
			            		fflush(stderr);
								exit(EXIT_FAILURE);
								return -1;
			            	}
			            	//printf("case 2, outFD: %d\n", outFD);
							pid_t pid = fork();
							if(pid == 0){
								if(dup2(outFD,STDOUT_FILENO) == -1){
									return -1;
								}
								if(outFD != STDOUT_FILENO){
									close(outFD);
								}
	  							char* argvList[] = {new_commandArr[0], NULL};
	  							if(*(new_commandArr[0]) == '/' || *(new_commandArr[0]) == '.'){
	  								struct stat sb;
	  								if (stat(new_commandArr[1], &sb) == -1) {
										perror("stat");
						          	}else{
						          		execv(new_commandArr[0], argvList);
						            }
	  							}else{
	  								withoutforkExecArgv(new_commandArr[0], argvList);
	  							}
	  							close(STDOUT_FILENO);
	  							dup2(1, 1);
							}else{
								waitpid(pid,NULL,0);
								currentPID = pid;
							}
						}
					}else if(argumetsArraySize > 3){
						isPipe = hasPipe(new_commandArr, argumetsArraySize); // return number of |
						if(!isPipe){
							if(hasOperatorResult){
								char* argvList[argumetsArraySize];
								int outFD  = 1, inFD = 0;
								int saveCMDPosition = 0;
								for(int i=0; i<argumetsArraySize; i++){
									if(*new_commandArr[i] == '<'){
										inFD  = open(new_commandArr[i+1], O_RDONLY);
										if(inFD == -1){
						            		fprintf(stderr, "%s: file not found\n", new_commandArr[i+1]);
						            		fflush(stderr);
											exit(EXIT_FAILURE);
											return -1;
						            	}
										i++;
										argvList[saveCMDPosition] = new_commandArr[i];
									}else if( strcmp(new_commandArr[i],">>") == 0 ){
										if(fileExists(new_commandArr[i+1]) == -1){
											perror("stat");
										}else{
											outFD  = open(new_commandArr[i+1], O_WRONLY | O_APPEND | O_TRUNC | O_CREAT, S_IRGRP | S_IWUSR | S_IRUSR | S_IWGRP);
											i++;
										}
										if(outFD == -1 || outFD==1){
						            		fprintf(stderr, "%s: file can not make or found\n", new_commandArr[i]);
						            		fflush(stderr);
											exit(EXIT_FAILURE);
											return -1;
						            	}
									}else{
										argvList[saveCMDPosition] = new_commandArr[i];
										saveCMDPosition++;
									}
								}
								argvList[saveCMDPosition] = NULL;

								pid_t pid = fork();
								if(pid == 0){
									if(dup2(outFD,STDOUT_FILENO) == -1){
										return -1;
									}
									if(outFD != STDOUT_FILENO){
										dup2(outFD,STDOUT_FILENO);
										close(outFD);
									}
									if(dup2(inFD,STDIN_FILENO) == -1){
										return -1;
									}
									if(inFD != STDIN_FILENO){
										dup2(inFD,STDIN_FILENO);
										close(inFD);
									}
									if(*new_commandArr[0] == '.' || *new_commandArr[0] == '/'){
										struct stat sb;
										if (stat(new_commandArr[0], &sb) == -1) {
											perror("stat");
								      	}else{
								      		execv(new_commandArr[0], argvList);
								      		perror("exec");
								      		exit(1);
								        }
									}else{
										withoutforkExecArgv(new_commandArr[0], argvList);
									}
								}else{
									waitpid(pid,NULL,0);
									currentPID = pid;
								}
							}
						} // end if(!isPipe)

						else if(isPipe) {

							int inFD = STDIN_FILENO, outFD = STDOUT_FILENO;

							int pipefds[2*isPipe];
							for(int i = 0; i < isPipe; i++) {
								if(pipe(pipefds + i*2) < 0) {
									perror("pipe");
									exit(EXIT_FAILURE);
								}
							}

							s_commandArrIndex = 0;
							addCMDStructCutByPipeExtra(new_commandArr);

			            	int zz = 1;
							int jj = 0;
							while(jj < isPipe*2) {
								if(s_commandArr[zz]->inputfile != 0x00){
									close(pipefds[jj]);
					            	pipefds[jj] = open(s_commandArr[zz]->inputfile, O_RDONLY);

					            	if(pipefds[jj] == -1){
					            		fprintf(stderr, "%s: file not found\n", s_commandArr[zz]->inputfile);
					            		fflush(stderr);
										exit(EXIT_FAILURE);
										return 0;
					            	}
								}
				            	if(s_commandArr[zz]->outputfile != 0x00){
				            		close(pipefds[jj+1]);
				            		pipefds[jj+1] = open(s_commandArr[zz]->outputfile, O_WRONLY | O_APPEND | O_TRUNC | O_CREAT, S_IRGRP | S_IWUSR | S_IRUSR | S_IWGRP);
				            		if(pipefds[jj+1] == -1){
					            		fprintf(stderr, "%s: file can not make or found\n", s_commandArr[zz]->outputfile);
					            		fflush(stderr);
										exit(EXIT_FAILURE);
										return -1;
						            }
				            	}
				            	jj+=2;
				            	zz++;
							}

							int i = 0, j = 0, command = isPipe+1, curr_cmd = 0;
							pid_t pid;

						    while(curr_cmd<command) {
					    		inFD = STDIN_FILENO;
								outFD = STDOUT_FILENO;

						        pid = fork();
						        if(pid == 0) {
						        	if(curr_cmd==0){
						        		if(s_commandArr[0]->inputfile != 0x00){
							            	inFD = open(s_commandArr[0]->inputfile, O_RDONLY);
							            	//printf("\ninFD : %d \n", inFD);

							            	if(inFD == -1){
							            		fprintf(stderr, "%s: file not found\n", s_commandArr[0]->inputfile);
							            		fflush(stderr);
												exit(EXIT_FAILURE);
												return 0;
							            	}

							            	dup2(inFD,STDIN_FILENO);

							            	if(dup2(inFD,STDIN_FILENO) == -1){
												perror("dup2");
											}
							            	if(inFD!=STDIN_FILENO){
							            		dup2(inFD,STDIN_FILENO);
									        	close(inFD);
									        }
									        if(inFD!=STDIN_FILENO){
							            		dup2(inFD,STDIN_FILENO);
									        	close(inFD);
									        }
										}
						            	if(s_commandArr[0]->outputfile != 0x00){
						            		outFD = open(s_commandArr[0]->outputfile, O_WRONLY | O_APPEND | O_TRUNC | O_CREAT, S_IRGRP | S_IWUSR | S_IRUSR | S_IWGRP);
						            		if(outFD == -1){
							            		exit(EXIT_FAILURE);
												return -1;
								            }
						            		dup2(outFD,STDOUT_FILENO);
						            		close(outFD);
						            	}else{
						            		dup2(pipefds[j + 1],STDOUT_FILENO);
						            	}
									}

						            if(curr_cmd && curr_cmd<(command-1)){ //if not last command
						            	if(dup2(pipefds[j + 1], STDOUT_FILENO) < 0){
						                	perror("dup2");
						                    exit(EXIT_FAILURE);
						                }
						            }
						            if( j != 0 ){ //if not first command
						            	if(dup2(pipefds[j-2], STDIN_FILENO) < 0){
						                	perror("dup2");
						                    exit(EXIT_FAILURE);
						                }
						            }

							        if(inFD!=STDIN_FILENO){
							        	dup2(inFD,STDIN_FILENO);
							        	close(inFD);
							        }
							        if(outFD!=STDOUT_FILENO){
							        	dup2(outFD,STDOUT_FILENO);
							        	close(outFD);
							        }
						            for(i = 0; i < 2*isPipe; i++){
						                close(pipefds[i]);
						            }

				            		withoutforkExecArgv(s_commandArr[curr_cmd]->newArgList[0], s_commandArr[curr_cmd]->newArgList);
				            		s_commandArr[curr_cmd]->executed = 1;

						        } else if(pid < 0){
						            perror("error");
						            exit(EXIT_FAILURE);
						        }
						        else{
						        	j+=2;
						        	currentPID = pid;
						        }
						        curr_cmd++;
						    }

						    for(int i = 0; i < 2 * isPipe; i++){
								close(pipefds[i]);
							}
							for(int i = 0; i < isPipe+1; i++){
								waitpid(pid,NULL,0);
							}
							currentPID = pid;
							freeCMDStruct();
						} // end else if(isPipe)

					} // else if(argumetsArraySize >= 3)

				} //has only hasAppendStdoutResult

				else if(hasDupRedirectionResult <= 1 && hasAppendStdInResult > 0 && hasSignArrowStdOutResult < 0){
					//printf("case 3 AppendStdIn \n");

					if(argumetsArraySize <= 3) { // cat << EOF
						if(strcmp(new_commandArr[1],"<") == 0 || strcmp(new_commandArr[1],"<<") == 0){
							int outFD = 1;
							if(strcmp(new_commandArr[1],"<<") == 0){
								outFD  = open(new_commandArr[2], O_RDWR | O_APPEND);
							}else{
								outFD  = open(new_commandArr[2],  O_RDONLY);
							}
							if(outFD == -1){
			            		fprintf(stderr, "%s: file can not make or found\n", new_commandArr[2]);
			            		fflush(stderr);
								exit(EXIT_FAILURE);
								return -1;
			            	}

							pid_t pid = fork();
							if(pid == 0){
								if(dup2(outFD,STDOUT_FILENO) == -1){
									return -1;
								}
								if(outFD != STDOUT_FILENO){
									close(outFD);
								}
	  							char* argvList[] = {new_commandArr[0], NULL};
	  							if(*(new_commandArr[0]) == '/' || *(new_commandArr[0]) == '.'){
	  								struct stat sb;
	  								if (stat(new_commandArr[1], &sb) == -1) {
										perror("stat");
						          	}else{
						          		execv(new_commandArr[0], argvList);
						            }
	  							}else{
	  								withoutforkExecArgv(new_commandArr[0], argvList);
	  							}
	  							close(STDOUT_FILENO);
	  							dup2(1, 1);
							}else{
								waitpid(pid,NULL,0);
								currentPID = pid;
							}
						}
						if(strcmp(new_commandArr[1],">") == 0 || strcmp(new_commandArr[1],">>") == 0){
							int outFD = 1;
							if(strcmp(new_commandArr[1],">") == 0){
								outFD  = open(new_commandArr[2],  O_WRONLY | O_CREAT , S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
							}else{
								outFD  = open(new_commandArr[2],  O_WRONLY | O_APPEND | O_CREAT , S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
							}
							if(outFD == -1){
			            		fprintf(stderr, "%s: file can not make or found\n", new_commandArr[2]);
			            		fflush(stderr);
								exit(EXIT_FAILURE);
								return -1;
			            	}

							pid_t pid = fork();
							if(pid == 0){
								if(dup2(outFD,STDOUT_FILENO) == -1){
									return -1;
								}
								if(outFD != STDOUT_FILENO){
									close(outFD);
								}
	  							char* argvList[] = {new_commandArr[0], NULL};
	  							if(*(new_commandArr[0]) == '/' || *(new_commandArr[0]) == '.'){
	  								struct stat sb;
	  								if (stat(new_commandArr[1], &sb) == -1) {
										perror("stat");
						          	}else{
						          		execv(new_commandArr[0], argvList);
						            }
	  							}else{
	  								withoutforkExecArgv(new_commandArr[0], argvList);
	  							}
	  							close(STDOUT_FILENO);
	  							dup2(1, 1);
							}else{
								waitpid(pid,NULL,0);
								currentPID = pid;
							}
						}
					}else if(argumetsArraySize > 3){
						isPipe = hasPipe(new_commandArr, argumetsArraySize); // return number of |
						if(!isPipe){
							if(hasOperatorResult){
								char* argvList[argumetsArraySize];
								int outFD  = 1, inFD = 0;
								int saveCMDPosition = 0;
								for(int i=0; i<argumetsArraySize; i++){
									if(strcmp(new_commandArr[i],"<<") == 0){
										inFD  = open(new_commandArr[i+1], O_RDWR | O_APPEND);
										if(inFD == -1){
						            		fprintf(stderr, "%s: file not found\n", new_commandArr[i+1]);
						            		fflush(stderr);
											exit(EXIT_FAILURE);
											return -1;
						            	}
										i++;
										argvList[saveCMDPosition] = new_commandArr[i];
									}else if(strcmp(new_commandArr[i],"<") == 0){
										inFD  = open(new_commandArr[i+1], O_RDONLY);
										if(inFD == -1){
						            		fprintf(stderr, "%s: file not found\n", new_commandArr[i+1]);
						            		fflush(stderr);
											exit(EXIT_FAILURE);
											return -1;
						            	}
										i++;
										argvList[saveCMDPosition] = new_commandArr[i];
									}else if( strcmp(new_commandArr[i],">") == 0 ){
										if(fileExists(new_commandArr[i+1]) == -1){
											perror("stat");
										}else{
											outFD  = open(new_commandArr[i+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRGRP | S_IWUSR | S_IRUSR | S_IWGRP);
											i++;
										}
										if(outFD == -1 || outFD==1){
						            		fprintf(stderr, "%s: file can not make or found\n", new_commandArr[i]);
						            		fflush(stderr);
											exit(EXIT_FAILURE);
											return -1;
						            	}
						            }else if( strcmp(new_commandArr[i],">>") == 0 ){
										if(fileExists(new_commandArr[i+1]) == -1){
											perror("stat");
										}else{
											outFD  = open(new_commandArr[i+1], O_WRONLY | O_APPEND | O_TRUNC | O_CREAT, S_IRGRP | S_IWUSR | S_IRUSR | S_IWGRP);
											i++;
										}
										if(outFD == -1 || outFD==1){
						            		fprintf(stderr, "%s: file can not make or found\n", new_commandArr[i]);
						            		fflush(stderr);
											exit(EXIT_FAILURE);
											return -1;
						            	}
									}else{
										argvList[saveCMDPosition] = new_commandArr[i];
										saveCMDPosition++;
									}
								}
								argvList[saveCMDPosition] = NULL;

								pid_t pid = fork();
								if(pid == 0){
									if(dup2(outFD,STDOUT_FILENO) == -1){
										return -1;
									}
									if(outFD != STDOUT_FILENO){
										dup2(outFD,STDOUT_FILENO);
										close(outFD);
									}
									if(dup2(inFD,STDIN_FILENO) == -1){
										return -1;
									}
									if(inFD != STDIN_FILENO){
										dup2(inFD,STDIN_FILENO);
										close(inFD);
									}
									if(*new_commandArr[0] == '.' || *new_commandArr[0] == '/'){
										struct stat sb;
										if (stat(new_commandArr[0], &sb) == -1) {
											perror("stat");
								      	}else{
								      		execv(new_commandArr[0], argvList);
								      		perror("exec");
								      		exit(1);
								        }
									}else{
										withoutforkExecArgv(new_commandArr[0], argvList);
									}
								}else{
									waitpid(pid,NULL,0);
									currentPID = pid;
								}
							}
						} // end if(!isPipe)

						else if(isPipe) {

							int inFD = STDIN_FILENO, outFD = STDOUT_FILENO;

							int pipefds[2*isPipe];
							for(int i = 0; i < isPipe; i++) {
								if(pipe(pipefds + i*2) < 0) {
									perror("pipe");
									exit(EXIT_FAILURE);
								}
							}

							s_commandArrIndex = 0;
							addCMDStructCutByPipeExtra(new_commandArr);

			            	int zz = 1;
							int jj = 0;
							while(jj < isPipe*2) {
								if(s_commandArr[zz]->inputfile != 0x00){
									close(pipefds[jj]);
									if(s_commandArr[zz]->doubleLeft == 0){
					            		pipefds[jj] = open(s_commandArr[zz]->inputfile, O_RDONLY);
									}
					            	else{
					            		pipefds[jj] = open(s_commandArr[zz]->inputfile, O_RDWR | O_APPEND);
					            	}

					            	if(pipefds[jj] == -1){
					            		fprintf(stderr, "%s: file not found\n", s_commandArr[zz]->inputfile);
					            		fflush(stderr);
										exit(EXIT_FAILURE);
										return 0;
					            	}
								}
				            	if(s_commandArr[zz]->outputfile != 0x00){
				            		close(pipefds[jj+1]);
				            		if(s_commandArr[zz]->doubleRight == 0){
					            		pipefds[jj+1] = open(s_commandArr[zz]->outputfile, O_WRONLY | O_TRUNC | O_CREAT, S_IRGRP | S_IWUSR | S_IRUSR | S_IWGRP);
									}
					            	else{
					            		pipefds[jj+1] = open(s_commandArr[zz]->outputfile, O_WRONLY | O_APPEND | O_TRUNC | O_CREAT, S_IRGRP | S_IWUSR | S_IRUSR | S_IWGRP);
					            	}
				            		if(pipefds[jj+1] == -1){
					            		fprintf(stderr, "%s: file can not make or found\n", s_commandArr[zz]->outputfile);
					            		fflush(stderr);
										exit(EXIT_FAILURE);
										return -1;
						            }
				            	}
				            	jj+=2;
				            	zz++;
							}

							int i = 0, j = 0, command = isPipe+1, curr_cmd = 0;
							pid_t pid;

						    while(curr_cmd<command) {
					    		inFD = STDIN_FILENO;
								outFD = STDOUT_FILENO;

						        pid = fork();
						        if(pid == 0) {
						        	if(curr_cmd==0){
						        		if(s_commandArr[0]->inputfile != 0x00){

						        			if(s_commandArr[0]->doubleLeft == 0){
							            		inFD = open(s_commandArr[0]->inputfile, O_RDONLY);
											}
							            	else{
							            		inFD = open(s_commandArr[0]->inputfile, O_RDWR | O_APPEND);
							            	}

							            	if(inFD == -1){
							            		fprintf(stderr, "%s: file not found\n", s_commandArr[0]->inputfile);
							            		fflush(stderr);
												exit(EXIT_FAILURE);
												return 0;
							            	}

							            	dup2(inFD,STDIN_FILENO);

							            	if(dup2(inFD,STDIN_FILENO) == -1){
												perror("dup2");
												//printf("\nerror1 inFD : %d curr_cmd: %d, s_commandArr[0]->inputfile: %s\n", inFD, curr_cmd, s_commandArr[0]->inputfile);
											}
											/////////// QUESTION : IT KEEPS inFD!=STDIN_FILENO
							            	if(inFD!=STDIN_FILENO){
							            		//printf("\nerror2 inFD : %d curr_cmd: %d, s_commandArr[0]->inputfile: %s\n", inFD, curr_cmd, s_commandArr[0]->inputfile);
												dup2(inFD,STDIN_FILENO);
									        	close(inFD);
									        }

									        if(inFD!=STDIN_FILENO){
							            		//printf("\nerror3 inFD : %d curr_cmd: %d, s_commandArr[0]->inputfile: %s\n", inFD, curr_cmd, s_commandArr[0]->inputfile);
												dup2(inFD,STDIN_FILENO);
									        	close(inFD);
									        }
										}
						            	if(s_commandArr[0]->outputfile != 0x00){

						            		if(s_commandArr[0]->doubleRight == 0){
							            		outFD = open(s_commandArr[0]->outputfile, O_WRONLY | O_TRUNC | O_CREAT, S_IRGRP | S_IWUSR | S_IRUSR | S_IWGRP);
											}
							            	else{
							            		outFD = open(s_commandArr[0]->outputfile, O_WRONLY | O_APPEND | O_TRUNC | O_CREAT, S_IRGRP | S_IWUSR | S_IRUSR | S_IWGRP);
							            	}

						            		if(outFD == -1){
							            		//fprintf(stderr, "%s: file can not make or found\n", s_commandArr[0]->outputfile);
												exit(EXIT_FAILURE);
												return -1;
								            }
						            		dup2(outFD,STDOUT_FILENO);
						            		close(outFD);
						            	}else{
						            		dup2(pipefds[j + 1],STDOUT_FILENO);
						            	}
									}

						            if(curr_cmd && curr_cmd<(command-1)){ //if not last command
						            	if(dup2(pipefds[j + 1], STDOUT_FILENO) < 0){
						                	// fprintf(stderr,"%s  j+1 %d curr_cmd %d\n", "dup2 1\n", j+1, curr_cmd);
						                	// fflush(stderr);
						                    perror("dup2");
						                    exit(EXIT_FAILURE);
						                }
						            }
						            if( j != 0 ){ //if not first command
						            	if(dup2(pipefds[j-2], STDIN_FILENO) < 0){
						                	// fprintf(stderr,"%s  j+1 %d curr_cmd %d\n", "dup2 2\n", j+1, curr_cmd);
						                	// fflush(stderr);
						                    perror("dup2");
						                    exit(EXIT_FAILURE);
						                }
						            }

							        if(inFD!=STDIN_FILENO){
							        	dup2(inFD,STDIN_FILENO);
							        	close(inFD);
							        }
							        if(outFD!=STDOUT_FILENO){
							        	dup2(outFD,STDOUT_FILENO);
							        	close(outFD);
							        }
						            for(i = 0; i < 2*isPipe; i++){
						                close(pipefds[i]);
						            }

				            		withoutforkExecArgv(s_commandArr[curr_cmd]->newArgList[0], s_commandArr[curr_cmd]->newArgList);
				            		s_commandArr[curr_cmd]->executed = 1;

						        } else if(pid < 0){
						            perror("error");
						            exit(EXIT_FAILURE);
						        }
						        else{
						        	j+=2;
						        	currentPID = pid;
						        }
						        curr_cmd++;
						    }

						    for(int i = 0; i < 2 * isPipe; i++){
								close(pipefds[i]);
							}
							for(int i = 0; i < isPipe+1; i++){
								waitpid(pid,NULL,0);
							}
							currentPID = pid;
							freeCMDStruct();
						} // end else if(isPipe)
					}
				} //has only hasAppendStdoutResult
				else if(hasSignArrowStdOutResult > -1){ // &>
					//printf("hasSignArrowStdOutResult case\n");
					if(argumetsArraySize <= 3 && hasOperatorResult) { // ls 2> out.txt
						if(strcmp(new_commandArr[1],"&>") == 0 ){
							int outFD  = open(new_commandArr[2], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
							if(outFD == -1){
			            		fprintf(stderr, "%s: file can not make or found\n", new_commandArr[2]);
			            		fflush(stderr);
								exit(EXIT_FAILURE);
								return -1;
			            	}

							pid_t pid = fork();
							if(pid == 0){
								dup2(outFD,2);
								if(dup2(outFD,STDOUT_FILENO) == -1){
									return -1;
								}
								if(outFD != STDOUT_FILENO){
									close(outFD);
								}
	  							char* argvList[] = {new_commandArr[0], NULL};
	  							if(*(new_commandArr[0]) == '/' || *(new_commandArr[0]) == '.'){
	  								struct stat sb;
	  								if (stat(new_commandArr[1], &sb) == -1) {
										perror("stat");
						          	}else{
						          		execv(new_commandArr[0], argvList);
						            }
	  							}else{
	  								withoutforkExecArgv(new_commandArr[0], argvList);
	  							}
	  							//fprintf(stderr, "%s\n", "dddd");
							}else{
								waitpid(pid,NULL,0);
								currentPID = pid;
							}
						}
					}else if(argumetsArraySize > 3){
						isPipe = hasPipe(new_commandArr, argumetsArraySize); // return number of |
						if(!isPipe){
							if(hasOperatorResult){
								char* argvList[argumetsArraySize];
								int outFD  = 1, inFD = 0;
								int saveCMDPosition = 0;
								for(int i=0; i<argumetsArraySize; i++){
									if(*new_commandArr[i] == '<'){
										inFD  = open(new_commandArr[i+1], O_RDONLY);
										if(inFD == -1){
						            		fprintf(stderr, "%s: file not found\n", new_commandArr[i+1]);
						            		fflush(stderr);
											exit(EXIT_FAILURE);
											return -1;
						            	}
										i++;
										argvList[saveCMDPosition] = new_commandArr[i];
									}else if( strcmp(new_commandArr[i],"&>") == 0 ){
										if(fileExists(new_commandArr[i+1]) == -1){
											perror("stat");
										}else{
											outFD  = open(new_commandArr[i+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRGRP | S_IWUSR | S_IRUSR | S_IWGRP);
											i++;
										}
										if(outFD == -1 || outFD==1){
						            		fprintf(stderr, "%s: file can not make or found\n", new_commandArr[i]);
						            		fflush(stderr);
											exit(EXIT_FAILURE);
											return -1;
						            	}
						            	close(outFD);
						            	outFD = hasDupRedirectionResult;
									}else{
										argvList[saveCMDPosition] = new_commandArr[i];
										saveCMDPosition++;
									}
								}
								argvList[saveCMDPosition] = NULL;

								pid_t pid = fork();
								if(pid == 0){
									dup2(outFD,2);

									if(dup2(outFD,STDOUT_FILENO) == -1){
										return -1;
									}
									if(outFD != STDOUT_FILENO){
										dup2(outFD,STDOUT_FILENO);
										close(outFD);
									}
									if(dup2(inFD,STDIN_FILENO) == -1){
										return -1;
									}
									if(inFD != STDIN_FILENO){
										dup2(inFD,STDIN_FILENO);
										close(inFD);
									}
									if(*new_commandArr[0] == '.' || *new_commandArr[0] == '/'){
										struct stat sb;
										if (stat(new_commandArr[0], &sb) == -1) {
											perror("stat");
								      	}else{
								      		execv(new_commandArr[0], argvList);
								      		perror("exec");
								      		exit(1);
								        }
									}else{
										withoutforkExecArgv(new_commandArr[0], argvList);
									}
								}else{
									waitpid(pid,NULL,0);
									currentPID = pid;
								}
							}
						} // end else if(!isPipe)
					} // else if(argumetsArraySize >= 3)

				}	// (hasSignArrowStdOutResult > -1)  &>
			} // END REDIRECTION
		}
		else{
			fprintf(stderr, "Invaild Case\n");
			//forkExecArgv(new_commandArr[0], new_commandArr);
		}
	}
	free(removedSpaceCMD);   //saveReceivedCMD
	freeCommandArrSpace(new_commandArr, c);
	return 0;
}

void builtin_help(){
	fprintf(stdout, "\thelp: Print a list of all builtins and their basic usage in a single column.\n\n\texit: Exits the shell by using the exit(3) function.\n\n\tcd: Changes the current working directory of the shell by using the chdir(2) system call.\n\tcd - should change the working directory to the last directory the user was in.\n\tcd with no arguments should go to the user's home directory which is stored in the HOME environment variable.\n\n\tpwd: prints the absolute path of the current working directory, which can be obtained by using the getcwd(3) function.\n");
	fflush(stdout);
}

void builtin_exit(){
	exit(3);
}

void builtin_pwd(){
	pid_t pid;
	//int status = 0;
	pid = fork();
	if(pid == 0){
		//printf("child getpid() %d, getppid() %d\n", getpid(), getppid());
		char* result = NULL;
		if((result = getCurrPwd()) != NULL){
			fprintf(stdout, "%s\n", result);
			fflush(stdout);
			_exit(0);
		}
	}else{
		waitpid(pid,NULL,0);
		currentPID = pid;
		//printf("my child pid: %d, getpid() %d \n", pid, getpid());
	}
}

char* getCurrPwd(){
	char* currPwd = NULL;
	if((currPwd = getcwd(currPath, maxLength)) == NULL){
		fprintf(stderr, "%s\n", "No obtained current cwd");
		fflush(stderr);
		exit(EXIT_FAILURE);
		return NULL;
	}
	return currPwd;
}

void updatePath(){
	char* sId = "dachoe : ";
    char* dollorSign = " $ ";

	char* temp = getCurrPwd();
    strcpy(currPathAdd, temp); //getenv("HOME");

    strcpy(sline, sId);
    strcat(sline, currPathAdd);
    strcat(sline, dollorSign);
}

char* getParentPath(){
	char* temp = getCurrPwd();
    strcpy(currPathAdd, temp);

    int c = 0;
    char* tempPtr = ((char*)temp) + strlen(temp) - 1;
    while(*(tempPtr) != '/'){
    	c++;
    	tempPtr--;
    }
    c++;
    int copyTargetNum = strlen(temp) - c;

    char* tempBuff = (char*)calloc(maxLength, sizeof(char));
    strncpy(tempBuff, currPathAdd, copyTargetNum);
    memset(currPathAdd, 0, maxLength);
    strcpy(currPathAdd, tempBuff);

    free(tempBuff);
	return currPathAdd;
}

char* cdPath(char* cmd){
	char* tempBuff = (char*)calloc(maxLength, sizeof(char));
	strcpy(tempBuff, cmd);
	//printf("tempBuff: %s\n", tempBuff);
	if(chdir(tempBuff) == -1){
		fprintf(stderr, "%s\n", "No such file or directory");
		fflush(stderr);
	}
	updatePath();
	free(tempBuff);
	return tempBuff;
}

void builtin_cd(){
	char* currHome = getenv("HOME");
	if(chdir(currHome) == -1){
		fprintf(stderr, "%s\n", "No such file or directory");
		fflush(stderr);
	}
	updatePath();
	return;
}

int hasSlash(char* path){
	char* tempBuff = (char*)calloc(maxLength, sizeof(char));
	strcpy(tempBuff, path);
	char* tempBuffPtr = tempBuff;

	while(*(tempBuffPtr)){
		if(*(tempBuffPtr) == '/'){
			free(tempBuff);
			return 1;
		}
    	tempBuffPtr++;
    }
    free(tempBuff);
	return 0;
}

int hasOperator(char* path){
	char* tempBuff = (char*)calloc(maxLength, sizeof(char));
	strcpy(tempBuff, path);
	char* tempBuffPtr = tempBuff;
	int numOperators = 0;
	while(*(tempBuffPtr)){
		if( *(tempBuffPtr) == '>' ){
			if( *(tempBuffPtr+1) == '>' ){ // check the operator is >>  or  >
				tempBuffPtr++;
			}
			numOperators++;
		}else if( *(tempBuffPtr) == '<' ){ // check the operator is <<  or  <
			if( *(tempBuffPtr+1) == '<' ){
				tempBuffPtr++;
			}
			numOperators++;
		}else if( *(tempBuffPtr) == '|' ){
			numOperators++;
		}
    	tempBuffPtr++;
    }
    free(tempBuff);
	return numOperators;
}

int checkArguments(char* path[], int size){
	//printf("path: %p \n", (char*) path);  // &(*tempBuffPtr)[1]
	char* tempBuffPtr[size];
	int i = (size-1);
	int j = 1; // since [0] has cat / ls / ...
	while(i){
		tempBuffPtr[j] = path[j];
		//printf("tempBuffPtr: %s \n", tempBuffPtr[j]);
		if( (strcmp(*(tempBuffPtr),">>") == 0 ) || *(tempBuffPtr[j]) == '>' || *(tempBuffPtr[j]) == '<' || *(tempBuffPtr[j]) == '|' ){
			if( !(j % 2)  ){ // must be at odd number index
				return 0;
			}
		}
		j++;
    	i--;
    }
	return 1;
}

int hasPipe(char* path[], int size){
	//printf("path: %p \n", (char*) path);  // &(*tempBuffPtr)[1]
	char* tempBuffPtr[size];
	int i = size;
	int j = 0; // since [0] has cat / ls / ...
	int numPipe = 0;
	while(i){
		tempBuffPtr[j] = path[j];
		if( *(tempBuffPtr[j]) == 124 ){
			numPipe++;
		}
		j++;
    	i--;
    }
	return numPipe;
}

int forkExec(char* keyword, char* path){
	pid_t pid;
	int status = 0;
	pid = fork();
	if(pid == 0){
		//printf("forkExec() child getpid() %d, getppid() %d\n", getpid(), getppid());
		char* envPath = getenv("PATH");
		char *array[255];
		char * concatBuff = calloc(maxLength, sizeof(char));

		int numPaths = 0;
		char* token = strtok(envPath, ":");
		while(token != NULL) {
			array[numPaths++] = token;
			token = strtok(NULL, ":");
		}

		for (int j = 0; j < numPaths; j++){
			if(array[j]){
				//strcpy(concatBuff, result);
				//strcat(concatBuff, array[j]);
				strcpy(concatBuff, array[j]);
				strcat(concatBuff, "/");
				strcat(concatBuff, keyword);
				//printf("concatBuff: %s\n", concatBuff);
				struct stat * sb = malloc(sizeof(struct stat));
				if (stat(concatBuff, sb) == 0) {
					char* argvList[] = {keyword, path, NULL};
		      		execv(concatBuff, argvList);
		      		perror("exec");
		      		break;
				}
				free(sb);
			}
		}
		free(concatBuff);
		//_exit(0);
	}else{
		waitpid(pid,&status,0);
		currentPID = pid;
	}
	return 0;
}

int forkExecArgv(char* keyword, char* argvList[]){
	//printf("forkExecArgv: %s \n", keyword);
	pid_t pid;
	int status = 0;
	pid = fork();

	//int childPid = 0;
	if(pid == 0){
		//printf("forkExec() child getpid() %d, getppid() %d\n", getpid(), getppid());
		//childPid = getpid();
		char* envPath = getenv("PATH");
		char *array[255];
		char * concatBuff = calloc(maxLength, sizeof(char));

		int numPaths = 0;
		char* token = strtok(envPath, ":");
		while(token != NULL) {
			array[numPaths++] = token;
			token = strtok(NULL, ":");
		}

		for (int j = 0; j < numPaths; j++){
			if(array[j]){
				strcpy(concatBuff, array[j]);
				strcat(concatBuff, "/");
				strcat(concatBuff, keyword);
				struct stat * sb = malloc(sizeof(struct stat));
				if (stat(concatBuff, sb) == 0) {
					if(execv(concatBuff, argvList) == -1){
						fprintf(stderr,"CAN NOT EXECUTE");
						fflush(stderr);
					}
		      		perror("exec");
		      		//childPid = 0;
		      		break;
				}
				free(sb);
			}
		}
		free(concatBuff);
		//_exit(0);
	}else{
		waitpid(pid,&status,0);
		currentPID = pid;
	}
	return 0;
}

int forkExecArgvWithoutEnv(char* keyword, char* argvList[]){
	pid_t pid;
	int status = 0;
	pid = fork();
	if(pid == 0){
		struct stat sb;
		if (stat(keyword, &sb) == -1) {
			perror("stat");
      	}else{
      		execv(keyword, argvList);
      		perror("exec");
      		exit(1);
        }

		// if(execv(keyword, argvList) == -1){
		// 	fprintf(stderr,"forkExecArgvWithoutEnv: CAN NOT EXECUTE");
		// 	fflush(stderr);
		// }
  		//perror("exec");
	}else{
		waitpid(pid,&status,0);
	}
	return 0;
}



int withoutforkExecArgv(char* keyword, char* argvList[]){
	char* envPath = getenv("PATH");
	char *array[255];
	char * concatBuff = calloc(maxLength, sizeof(char));

	int numPaths = 0;
	char* token = strtok(envPath, ":");
	while(token != NULL) {
		array[numPaths++] = token;
		token = strtok(NULL, ":");
	}

	for (int j = 0; j < numPaths; j++){
		if(array[j]){
			strcpy(concatBuff, array[j]);
			strcat(concatBuff, "/");
			strcat(concatBuff, keyword);
			struct stat * sb = malloc(sizeof(struct stat));
			if (stat(concatBuff, sb) == 0) {
				if(execv(concatBuff, argvList) == -1){
					fprintf(stderr,"CAN NOT EXECUTE");
					fflush(stderr);
				}
	      		perror("exec");
	      		break;
			}
			free(sb);
		}
	}
	free(concatBuff);
	return 0;
}

int fileExists(const char* file) {
	struct stat buf;
	return (stat(file, &buf) == 0);
}

// if | operator is founded,  save new array to save until it encounters |, and set null
// check number of element before encountering | operator
// ls bin | cat | grep "foo"

int getNewArrayCutByPipe(char* sourceArr[], char* newArr[]){
	int newSize = countElementByPipe(sourceArr);
	int i = newSize;
	int j = 0;
	while(i){
		if( *(newArr[j]) == 124 ){
			newArr[newSize] = NULL;
			return 0;
		}else{
			newArr[j] = sourceArr[j];
		}
		j++;
    	i--;
    }
    newArr[newSize] = NULL;
	return 0;
}

int countElementByPipe(char* path[]){
	// printf("path: %p \n", (char*) path);  // &(*tempBuffPtr)[1]
	int size = 0; /////////////////
	char* tempBuffPtr[size];
	int i = size;
	int j = 0; // since [0] has cat / ls / ...
	int numPipe = 0;
	while(i){
		//printf("countElementByPipe tempBuffPtr[%d]): %s \n",j, tempBuffPtr[j]);

		if( *(tempBuffPtr[j]) == 124 ){
			return numPipe;
		}
		tempBuffPtr[j] = path[j];
		j++;
    	i--;
    	numPipe++;
    }
	return numPipe;
}

// count pipe > make struct * counted number of pipe > save into commandArrs
void addCMDStructCutByPipe(char* sourceArr[]){
	//printf("argumetsArraySize : %d \n", argumetsArraySize);
	int numCmmd = isPipe + 1;
	int indexSoucrArr = 0;

	for(int i=0; i<numCmmd; i++){
		s_command* cmd = (struct s_command*) calloc(maxLength, sizeof(s_command));
		cmd->executed = 0;
		cmd->numArg=0;
		cmd->doubleLeft = 0;
		cmd->doubleRight = 0;
		int j = 0;
		int newArgListIndex = 0;
		int readFileName = 0; // prevent to save output-filename and operator into newArgList
		while( indexSoucrArr < argumetsArraySize && *sourceArr[indexSoucrArr] != 124 ){
			//printf("s_commandArr[%d], cmd->argList[%d], sourceArr[%d] : %s \n", s_commandArrIndex, j, indexSoucrArr, sourceArr[indexSoucrArr]);
			cmd->argList[j++] = sourceArr[indexSoucrArr];
			cmd->numArg++;

			if(*sourceArr[indexSoucrArr] == '<'){
				readFileName = 1;
				cmd->newArgList[newArgListIndex++] = sourceArr[indexSoucrArr+1];
				cmd->inputfile = cmd->newArgList[newArgListIndex-1];
				//printf("cmd->newArgList[%d] : %s \n", newArgListIndex-1, cmd->newArgList[newArgListIndex-1]);
				//printf("cmd->inputfile : %s \n", cmd->inputfile);
			}else if(*sourceArr[indexSoucrArr] == '>'){
				readFileName = 1;
				cmd->outputfile = sourceArr[indexSoucrArr+1];
				//printf("cmd->outputfile : %s \n", cmd->outputfile);
			}else{
				if(!readFileName){
					cmd->newArgList[newArgListIndex++] = sourceArr[indexSoucrArr];
					//printf("cmd->newArgList[%d] : %s \n", newArgListIndex-1, cmd->newArgList[newArgListIndex-1]);
				}else
					readFileName = 0;
			}
			indexSoucrArr++;
		}
		indexSoucrArr++;
		cmd->argList[j] = NULL;
		cmd->newArgList[newArgListIndex] = NULL;
		cmd->numArg++;
		//printf("s_commandArr[%d], cmd->argList[%d] = %s, cmd->newArgList[%d] = %s\n", s_commandArrIndex, j, cmd->argList[j], newArgListIndex, cmd->newArgList[newArgListIndex]);

		s_commandArr[s_commandArrIndex] = cmd;
		s_commandArrIndex++;
	}
}
// extra << >>
void addCMDStructCutByPipeExtra(char* sourceArr[]){
	//printf("argumetsArraySize : %d \n", argumetsArraySize);
	int numCmmd = isPipe + 1;
	int indexSoucrArr = 0;

	for(int i=0; i<numCmmd; i++){
		s_command* cmd = (struct s_command*) calloc(maxLength, sizeof(s_command));
		cmd->executed = 0;
		cmd->numArg=0;
		cmd->doubleLeft = 0;
		cmd->doubleRight = 0;
		int j = 0;
		int newArgListIndex = 0;
		int readFileName = 0; // prevent to save output-filename and operator into newArgList
		while( indexSoucrArr < argumetsArraySize && *sourceArr[indexSoucrArr] != 124 ){
			//printf("s_commandArr[%d], cmd->argList[%d], sourceArr[%d] : %s \n", s_commandArrIndex, j, indexSoucrArr, sourceArr[indexSoucrArr]);
			cmd->argList[j++] = sourceArr[indexSoucrArr];
			cmd->numArg++;

			if(*sourceArr[indexSoucrArr] == '<'){
				if(*(sourceArr[indexSoucrArr]+1) == '<'){
					cmd->doubleLeft = 1;
				}
				readFileName = 1;
				cmd->newArgList[newArgListIndex++] = sourceArr[indexSoucrArr+1];
				cmd->inputfile = cmd->newArgList[newArgListIndex-1];
				//printf("cmd->newArgList[%d] : %s \n", newArgListIndex-1, cmd->newArgList[newArgListIndex-1]);
				//printf("cmd->inputfile : %s \n", cmd->inputfile);
			}else if(*sourceArr[indexSoucrArr] == '>'){
				if(*(sourceArr[indexSoucrArr]+1) == '>'){
					cmd->doubleRight = 1;
				}
				readFileName = 1;
				cmd->outputfile = sourceArr[indexSoucrArr+1];
				//printf("cmd->outputfile : %s \n", cmd->outputfile);
			}else{
				if(!readFileName){
					cmd->newArgList[newArgListIndex++] = sourceArr[indexSoucrArr];
					//printf("cmd->newArgList[%d] : %s \n", newArgListIndex-1, cmd->newArgList[newArgListIndex-1]);
				}else
					readFileName = 0;
			}
			indexSoucrArr++;
		}
		indexSoucrArr++;
		cmd->argList[j] = NULL;
		cmd->newArgList[newArgListIndex] = NULL;
		cmd->numArg++;
		//printf("s_commandArr[%d], cmd->argList[%d] = %s, cmd->newArgList[%d] = %s\n", s_commandArrIndex, j, cmd->argList[j], newArgListIndex, cmd->newArgList[newArgListIndex]);

		s_commandArr[s_commandArrIndex] = cmd;
		s_commandArrIndex++;
	}
}

void takeOffQuote(char* argumetsArray[]){
	int index = 1;
	while(index != (argumetsArraySize) ){
		char* tokenKeyword = (char*) calloc(maxLength, sizeof(char));
		strcpy(tokenKeyword, argumetsArray[index]);
		char* tokenNewKeyword = (char*) calloc(maxLength, sizeof(char));

		char* keywordPtr = tokenKeyword;
		char* newkeywordPtr = tokenNewKeyword;
		while(*keywordPtr){
			if(*keywordPtr == 92){ // 92 = '\'  case1: finding " or ' symbol
				if(*(keywordPtr+1) == 34 || *(keywordPtr+1) == 39){
					*(newkeywordPtr++) = *(keywordPtr+1);
					keywordPtr++;
				}
			}else if(*(keywordPtr) == 34 || *(keywordPtr) == 39){ // case 2: take off symbol
				// skipp symbol to take off. Do not add into tokenNewKeyword
			}else{
				*(newkeywordPtr++) = *keywordPtr;
			}
			keywordPtr++;
		}
		strcpy(argumetsArray[index], tokenNewKeyword);
		free(tokenKeyword);
		free(tokenNewKeyword);
		index++;
	}
}

void freeCommandArrSpace(char ** new_commandArr, int size){
  for(int i=0; i<size; i++){
    free(new_commandArr[i]);
  }
}

void removeleadingSpace(char* cmd){
	char* saveOriginalCMD = (char*) calloc(maxLength, sizeof(char));
	strcpy(saveOriginalCMD, cmd);
	char* saveOriginalCMDPtr = saveOriginalCMD;
	char* saveCMD = (char*) calloc((int)strlen(cmd), sizeof(char));

	int removedSpace = 0;
	while(*saveOriginalCMDPtr == ' '){
		saveOriginalCMDPtr++;
		removedSpace++;
	}

	if(removedSpace){
		strcpy(saveCMD, saveOriginalCMDPtr);
		strcpy(cmd, saveCMD);
	}

	free(saveOriginalCMD);
	free(saveCMD);
}

void removeleadingEndingSpace(char* cmd){
	char* endSpaceCMD = (char*) calloc((int)strlen(cmd), sizeof(char));
	strcpy(endSpaceCMD, cmd);
	char* endSpaceCMDPtr = endSpaceCMD;

	int lengthCMD = (int)strlen(endSpaceCMD);

	endSpaceCMDPtr += (lengthCMD-1);

	if(*endSpaceCMDPtr != ' ' && *endSpaceCMD != ' '){	// check start/end points have space, if not, just return;
		strcpy(removedSpaceCMD, endSpaceCMD);
		strcat(removedSpaceCMD, "+");
		return;
	}

	if(*endSpaceCMDPtr == ' '){
		int numEndSpace = 0;	// if the string ends with space, take off the space
		while(*endSpaceCMDPtr == ' '){
			numEndSpace++;
			endSpaceCMDPtr--;
		}
		lengthCMD = lengthCMD - numEndSpace;

		memset(endSpaceCMD,'\0',strlen(endSpaceCMD));
		strncpy(endSpaceCMD, cmd, lengthCMD);
		endSpaceCMDPtr = endSpaceCMD;
		strcat(endSpaceCMD, "+");
	}else{
		strcat(endSpaceCMD, "+");
	}

	if(*endSpaceCMD == ' '){
		endSpaceCMDPtr = endSpaceCMD;
		int removedLeadingSpace = 0;
		while(*endSpaceCMDPtr == ' '){
			endSpaceCMDPtr++;
			removedLeadingSpace++;
		}
		lengthCMD = lengthCMD - removedLeadingSpace;
		strcpy(removedSpaceCMD, endSpaceCMDPtr);
	}else{
		endSpaceCMDPtr = endSpaceCMD;
		strcpy(removedSpaceCMD, endSpaceCMDPtr);
	}

	free(endSpaceCMD);
}

void alarmHandler(int sig) {

	char* alarm_msg = "\nYour ";
	char* alarm_msg_end = " second timer has finished!\n";

	char intToString[alarmTime];
	decimalToChar(alarmTime, intToString);

	write(STDOUT_FILENO, alarm_msg, strlen(alarm_msg));
	write(STDOUT_FILENO, intToString, strlen(intToString));
	write(STDOUT_FILENO, alarm_msg_end, strlen(alarm_msg_end));
	write(STDOUT_FILENO, sline, strlen(sline));
	fflush(stdout);
	//rl_on_new_line();
}

void reverseString(int strlength, char str[]){
	char* buf = calloc(strlen(str)+1, sizeof(char));
	char* bufPtr = buf;
    int index = 0;
    int end = strlength -1;
    while (index <= (strlength -1))
    {
    	*(bufPtr++) = *(str+end);
        index++;
        end--;
    }
    *(bufPtr) = '\0';
    memset(str, 0, strlen(str));
    strcpy(str, buf);
    free(buf);
}

char* decimalToChar(int num, char* str){
    int i = 0;

    if (num == 0){
        str[i] = '0';
        str[i+1] = '\0';
        return str;
    }

	while (num != 0){
		int rem = num % 10;
		if(rem > 9){
			str[i++] = (rem-10) + 'a';
		}else{
			str[i++] = rem + '0';
		}
		num = num/10;
	}
	str[i] = '\0';

    reverseString(i, str);
    return str;
}

int charToDecimal(char* str, int nLength){
	char * pointer = str;
	int decimalNum = 0;
	int tempNum = 0;
	int stop = 0;
	int powerStop = nLength;

	while(stop != nLength){
		tempNum = *pointer - '0';

		int power = 1;
		int i = 0;
		while(++i <powerStop){
			power *= 10;
		}

		tempNum = tempNum * power;
		decimalNum = decimalNum + tempNum;
		pointer++;
		stop++;
		powerStop--;

		if(*pointer== 0x0){
			break;
		}

	}
	return decimalNum;
}

int isAllNumber(char* str, int nLength){
	char *ptr = str;
	int stop = 0;
	if(*ptr== 0x0)
		return -1;

	while(stop < nLength){
		if(*ptr < '0' || *ptr > '9'){
			return -1;
		}
		ptr++;

		if(*ptr== 0x0)
			break;
	}
	return 1;
}

void childSignalHandler (int sig, siginfo_t *sip, void *notused)
{
  int status;
  if(sig == SIGCHLD){
    fprintf(stdout, "Child with PID %i has died. It spent %.0f milliseconds \n", sip->si_pid, (((double) (sip->si_stime + sip->si_utime) / (double)sysconf(_SC_CLK_TCK)) * 1000) );
    fflush (stdout);

    status = 0;
    if (sip->si_pid == waitpid (sip->si_pid, &status, WNOHANG))
    {
        if (WIFEXITED(status) || WIFSTOPPED(status) || (WTERMSIG(status) <= 12) || (WTERMSIG(status) == 15))
        {
         	goto done;
        }
	done:;
	    }
	    else{

	    }
	}
}
//SIGTSTP
void ctrlZHandler(int sig){
	// printf ("ctrlZHandler, currentPID: %d\n", currentPID);
	// fprintf(stderr,"%d\n", currentPID);
	// //kill(currentPID,SIGTSTP);
	// fprintf(stderr,"%d\n", currentPID);
	// fflush (stdout);
	return;
	//_exit(0);
	//rl_on_new_line();
}

// kill -s SIGUSR2
void killSIGUSR2(){
	if( kill(killID,SIGUSR2) < 0 ){
		fprintf (stderr, "kill: (%d) - No such process\n", killID);
		fflush (stderr);
	}else{
		write(STDOUT_FILENO, "Well that was easy.\n", strlen("Well that was easy.\n"));
		fflush(stdout);
  		exit (0);
		return;
	}
}

void freeCMDStruct(){
	int numCmmd = isPipe + 1;
	for(int i=0; i<numCmmd; i++){
		s_command* ptr = s_commandArr[i];
		free(ptr);
	}
}


//extra
int hasDuplicateRedirection(char* path){	// return file descripter number
	char* tempBuff = (char*)calloc(maxLength, sizeof(char));
	strcpy(tempBuff, path);
	char* tempBuffPtr = tempBuff;

	int hasDupRedirection = -1;
	while(*(tempBuffPtr)){	// 0 for stdin  1 for stdout 2 for stderr  3 ... for custom descripter
		if( *(tempBuffPtr) >= '0' && *(tempBuffPtr) <= '9' ){
			if( *(tempBuffPtr+1) == '>' ){
				hasDupRedirection = *tempBuffPtr - '0';
			}
		}
    	tempBuffPtr++;
    }
    free(tempBuff);
    return hasDupRedirection;
}

int hasAppendStdout(char* path){	// >>  return 1 if >> found, otherwise -1
	char* tempBuff = (char*)calloc(maxLength, sizeof(char));
	strcpy(tempBuff, path);
	char* tempBuffPtr = tempBuff;

	while(*(tempBuffPtr)){	// 0 for stdin  1 for stdout 2 for stderr  3 ... for custom descripter
		if( *(tempBuffPtr) == '>' ){
			if( *(tempBuffPtr+1) == '>' ){
				free(tempBuff);
    			return 1;
			}
		}
    	tempBuffPtr++;
    }
    free(tempBuff);
    return -1;
}

int hasAppendStdIn(char* path){	// >>  return 1 if << found, otherwise -1
	char* tempBuff = (char*)calloc(maxLength, sizeof(char));
	strcpy(tempBuff, path);
	char* tempBuffPtr = tempBuff;

	while(*(tempBuffPtr)){	// 0 for stdin  1 for stdout 2 for stderr  3 ... for custom descripter
		if( *(tempBuffPtr) == '<' ){
			if( *(tempBuffPtr+1) == '<' ){
				free(tempBuff);
    			return 1;
			}
		}
    	tempBuffPtr++;
    }
    free(tempBuff);
    return -1;
}

int hasSignArrowStdOut(char* path){	// >>  return 1 if &> found, otherwise -1
	char* tempBuff = (char*)calloc(maxLength, sizeof(char));
	strcpy(tempBuff, path);
	char* tempBuffPtr = tempBuff;

	while(*(tempBuffPtr)){	// 0 for stdin  1 for stdout 2 for stderr  3 ... for custom descripter
		if( *(tempBuffPtr) == '&' ){
			if( *(tempBuffPtr+1) == '>' ){
				free(tempBuff);
    			return 1;
			}
		}
    	tempBuffPtr++;
    }
    free(tempBuff);
    return -1;
}
