#ifndef SFISH_H
#define SFISH_H
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <wait.h>
#include <ucontext.h>

#endif

#define maxLength 1024
#define minLength 255

char currPath[maxLength-1];
char* sline;
char* currPathAdd;
int currentPID;

char* removedSpaceCMD;

struct sigaction sact;
volatile int alarmTime;  // alarm signal
volatile double count;
time_t t;

void childSignalHandler (int sig, siginfo_t *sip, void *notused);

struct s_command {
	int executed;
	int numArg;
	int fd;
	int doubleLeft;
	int doubleRight;
	char* argList[minLength];
	char* newArgList[minLength];
	char* inputfile;
	char* outputfile;
};
typedef struct s_command s_command;

struct s_command* s_commandArr[10 * sizeof(s_command)];


void takeOffQuote(char* argumetsArray[]);

char* getCurrPwd();

/*
 *	Update path of current working directory $
 */
void updatePath();

/*
 *	Get parent directory from current path
 */
char* getParentPath();

char* cdPath(char* cmd);

// If the given path has '/', return 1, otherwise 0
int hasSlash(char* path);

// If the given path has '<', '>', '|', return NUMBER OF OPERATORS, otherwise 0
int hasOperator(char* path);

// Check the arguments array contains data in order correctly.
int checkArguments(char* path[], int size);
int hasPipe(char* path[], int size);

// handle user command line
int builtins(char* cmd); // old
int parseCMD(char* cmd);
void removeleadingSpace(char* cmd);
void removeleadingEndingSpace(char* cmd);

int forkExec(char* keyword, char* path);
int forkExecArgv(char* keyword, char* argvList[]);
int forkExecArgvWithoutEnv(char* keyword, char* argvList[]);

int withoutforkExecArgv(char* keyword, char* argvList[]); // for pipe
//char** getNewArrayCutByPipe(char* path[]);
int getNewArrayCutByPipe(char* sourceArr[], char* newArr[]);
int countElementByPipe(char* path[]);

void addCMDStructCutByPipe(char* sourceArr[]);
void addCMDStructCutByPipeAndDF(char* sourceArr[]);

void freeCMDStruct();
void freeCommandArrSpace(char ** new_commandArr, int size);

int fileExists(const char* file);

void builtin_help();
void builtin_exit();
void builtin_pwd();
void builtin_cd();

void alarmHandler(int sig);
char* decimalToChar(int num, char* buf);
void reverseString(int strlength, char str[]);
int charToDecimal(char* str, int nLength);
int isAllNumber(char* str, int nLength);

//void ctrlZHandler();
void ctrlZHandler(int sig);
void killSIGUSR2();

//extra
int hasDuplicateRedirection(char* path);
int hasAppendStdout(char* path);
int hasAppendStdIn(char* path);
void addCMDStructCutByPipeExtra(char* sourceArr[]);
int hasSignArrowStdOut(char* path);