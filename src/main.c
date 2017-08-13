#include "../include/sfish.h"
#include "../include/debug.h"
#include <unistd.h>
#include <signal.h>
/*
 * As in previous hws the main function must be in its own file!
 */

int main(int argc, char const *argv[], char* envp[]){
    /* DO NOT MODIFY THIS. If you do you will get a ZERO. */
    rl_catch_signals = 0;
    /* This is disable readline's default signal handlers, since you are going to install your own.*/
    char *cmd;

    ////////////////////////////////////////////////////////////////////////////

    currPathAdd = (char*) calloc(maxLength, sizeof(char));
    sline = (char*) calloc(maxLength, sizeof(char));

    updatePath();

    struct sigaction action;

    action.sa_sigaction = childSignalHandler; /* Note use of sigaction, not handler */
    sigfillset (&action.sa_mask);
    action.sa_flags = SA_SIGINFO; /* Note flag - otherwise NULL in function */

    sigaction (SIGCHLD, &action, NULL);

    sigset_t mask;
    sigemptyset (&mask);
    sigaddset (&mask, SIGTSTP);

    if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
        perror("sigprocmask");
        return 1;
    }

    if( signal( SIGALRM, alarmHandler) == SIG_ERR  )
    {
        printf("Unable to create Alarm handler for SIGALRM\n");
    }

    if( signal( SIGTSTP, ctrlZHandler) == SIG_ERR  )
    {
        printf("Unable to create Alarm handler for SIGALRM\n");
    }

    if( signal( SIGUSR2, killSIGUSR2) == SIG_ERR  )
    {
        printf("Unable to create SIGUSR2 handler for SIGUSR2\n");
    }

    ////////////////////////////////////////////////////////////////////////////

    while((cmd = readline(sline)) != NULL) { //"sfish> "
        if (strcmp(cmd, "exit")==0)
            break;

        if(strlen(cmd)==0){
            //printf("cmd empty\n");
            currentPID = getpid();
            //printf("currentPID : %d\n", currentPID);
        }else{
            parseCMD(cmd);
        }

        /* All your debug print statements should use the macros found in debu.h */
        /* Use the `make debug` target in the makefile to run with these enabled. */
        info("Length of command entered: %ld\n", strlen(cmd));
        /* You WILL lose points if your shell prints out garbage values. */
    }

    /* Don't forget to free allocated memory, and close file descriptors. */
     free(cmd);
     free(sline);
     free(currPathAdd);

    return EXIT_SUCCESS;
}
