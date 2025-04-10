
/* 
chatgpt : https://chatgpt.com/share/6762f406-0f98-8002-abd3-0b25ac53a7d1
video about piping that helped me : https://www.youtube.com/watch?v=8Q9CPWuRC6o
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>


int prepare (void) ;
int process_arglist(int count, char **arglist) ;
void signal_handler () ;
int exec_command (int count , char **arglist);
int run_background(int count , char **arglist);
int run_InputRedirect(int count , char **arglist) ;
int run_OutputRedirect(int count , char **arglist);
int find_PipeIndex(char **arglist,int count);
int check_pipe_sym (int count, char **arglist);
int check_InputRedirecting_sym (int count, char **arglist);
int check_OutputRedirecting_sym (int count, char **arglist);
int run_pipe(int count , char**arglist);
int exec_NormalCommand (int count , char**arglist);
int get_inputredirectIndex(int count , char **arglist);
int finalize();








int prepare (void) 
{
    struct sigaction s1,s2;
    s1.sa_handler = SIG_IGN;
    s2.sa_handler = SIG_IGN;
    s1.sa_flags = SA_RESTART;
    s2.sa_flags = SA_RESTART;
    if (sigaction(SIGINT,&s1,NULL)==-1 || sigaction(SIGCHLD,&s2,NULL)==-1)
    {
        perror("sigaction error");
        return 1; /* 1 means error occured*/
    }

    return 0 ; 
}

int process_arglist(int count, char **arglist) /* size is count +1 */
{
    /*return 1 on success or 0 on failure or exists as mentiones in the assignment pdf*/
    if (strcmp(arglist[count-1],"&")==0) return run_background(count,arglist);
    else if(check_pipe_sym(count,arglist)) return run_pipe(count,arglist);
    else if(check_InputRedirecting_sym(count,arglist)) return run_InputRedirect(count,arglist);
    else if(check_OutputRedirecting_sym(count,arglist)) return run_OutputRedirect(count,arglist);
    return exec_NormalCommand (count,arglist) ; /*normal command */
}

void signal_handler() {
    struct sigaction s;
    s.sa_handler = SIG_DFL;
    s.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &s, NULL) == -1) {
        perror("sigaction failed");
        exit(1);  // Terminates child process as required
    }
}

/*commands without {&,|,<,>} */
int exec_NormalCommand (int count , char **arglist)
{
    pid_t pid = fork() ; 
    if(pid < 0 )
    {
        perror("Error executing Command - failed forking") ; 
        return 0 ; 
    }
    if(pid == 0) {
        signal_handler(); /*handles signal*/
        if (execvp(arglist[0] , arglist)==-1){
        perror("Error executing Command - execvp failed") ; 
        exit(1) ; 
        } 
    }
    else 
    {
        int stat ;
        if(waitpid(pid,&stat,0)==-1 && errno!=ECHILD && errno!= EINTR) 
        {
        perror("Error executing Command - wating failed"); 
        exit(1);
        }
    }
    return 1;
}
/*background commands */
int run_background(int count , char **arglist) {
    pid_t pid = fork() ;
    if (pid == -1){
        perror("An Error Occured");
        return 0 ;
    }
    arglist[count-1] = NULL ; 
    if (pid== 0) /* this is child*/
    {
        if(execvp(arglist[0] , arglist)==-1){
            perror("error") ; 
            exit(1);
        } 
    }
    return 1 ; /* parent does not wait for the child*/
}

int get_InputOutputredirectIndex(int count , char **arglist)
{
    for(int i=0 ; i<count ; i++ )
    {
         if((strcmp(arglist[i],">")==0) || (strcmp(arglist[i],"<")==0 )) return i;
    }
    return -1 ; /* any value in order to compile*/
}
int run_InputRedirect(int count , char **arglist)
{
    int index = get_InputOutputredirectIndex(count, arglist);
    char *FileName = arglist[index + 1];/*file name appers after '<' as mentions in the pdf*/
    arglist[index] = NULL;  // Remove "<"

    pid_t pid = fork(); 
    if (pid == -1) {
        perror("Error Forking") ; 
        return 0 ; 
    }
    if(pid ==0)
    {
    if (index == -1 || index == 0 || index + 1 >= count) { /* if index is 0 , then its just '<' which is not allowed*/
        fprintf(stderr, "Error: Invalid input redirection syntax.\n");
        return 0;
    }
    signal_handler(); /*handles signal*/
    int fd = open(FileName , O_RDONLY) ;
    if (fd == -1 ) {
        perror("Error Opening the FIle") ; 
        exit(1); 
    }
    else { /* now we redirect*/
    if (dup2(fd,STDIN_FILENO)==-1)
    {
        perror("Error redirecting") ; 
        close(fd);
        exit(1); 
    }
    close(fd);
    if(execvp(arglist[0] , arglist)==-1)
    {
        perror("error") ; 
        exit(1);
    } 

    }

    }
    else { /*parent should wait*/
    int stat ; 
    if(waitpid(pid,&stat,0)==-1 && errno!=ECHILD && errno!= EINTR) 
    {
        perror("Error , I am the parent"); 
        return 0;
    }
    }
    return 1 ;
}

int run_OutputRedirect(int count , char **arglist)
{
    int index = get_InputOutputredirectIndex(count,arglist);
    pid_t pid = fork(); 
    if (pid == -1) {
        perror("Error Forking") ; 
        return 0 ; 
    }
    if(pid ==0)
    {
    if (index == -1 || index==0 || index + 1 >= count) {
        fprintf(stderr, "Error: Invalid output redirection syntax.\n");
        return 0;
    }
    signal_handler();
    arglist[count-2] = NULL ;
    char * FileName = arglist[index+1] ; 
    int fd = open(FileName , O_WRONLY | O_CREAT | O_TRUNC ,0600) ; 
    if (fd == -1 ) {
        perror("Error Opening the FIle") ; 
        exit(1); 
    }
    else { /* now we redirect*/
    if (dup2(fd,STDOUT_FILENO)==-1)
    {
        perror("Error redirecting") ; 
        close(fd); /*close fd before exiting*/
        exit(1); 
    }
    close(fd);
    if(execvp(arglist[0] , arglist)==-1)
    {
        perror("error") ; 
        exit(1);
    } 

    }
    }
    else { /*parent should wait*/
    int stat ; 
    if(waitpid(pid,&stat,0)==-1 && errno!=ECHILD && errno!= EINTR) 
    {
        perror("Error , I am the parent"); 
        return 0;
    }
    }
    return 1 ;
}

int find_PipeIndex(char **arglist,int count) 
{
    for(int i=0 ; i<count ; i++ )
    {
         if(strcmp(arglist[i],"|")==0) return i;
    }
    return -1 ; /* any value in order to compile*/
}
int run_pipe(int count,char **arglist)
{
    int fd[2];
    int pipe_index = find_PipeIndex(arglist,count);
    arglist[pipe_index] = NULL ; 
    pid_t pid ;
    if(pipe(fd)==-1) /*pipe failed*/
    {
        perror("pipe failed");
        return 0 ; 
    }
    pid = fork();
    if(pid == -1)
    {
        perror("fork failed");
        return 0; 
    }
    if (pid ==0) /*first child process*/
    {
        signal_handler(); /*handles signal*/
        close(fd[0]); /*close read pipe*/
        if (dup2(fd[1],STDOUT_FILENO)==-1) /*dup2 failed*/
        {
            perror("dup2 failed");
            close(fd[1]);
            exit(1);  
        }
        close(fd[1]);
        if(execvp(arglist[0],arglist)==-1)
        {
             perror("exec failed ");
             exit(1); 
        }
    }
    pid_t pid2 = fork(); /*second child */
        if(pid2 == -1)
    {
        perror("fork failed");
        return 0 ; 
    }
    if (pid2 ==0) /*2nd child process*/
    {
        signal_handler(); /*handles signal*/
        close(fd[1]); /*close read pipe*/
        if (dup2(fd[0],STDIN_FILENO)==-1) /*dup2 fails*/
        {
            perror("dup2 failed");
            close(fd[0]);
            exit(1);  
        }
        close(fd[1]);
        if(execvp(arglist[pipe_index+1],arglist+1+pipe_index)==-1) /*file name appers after index*/
        {
             perror("exec failed ");
             exit(1); 
        }
    }
    close (fd[1]);
    close (fd[0]);
    int status1,status2;
    if(waitpid(pid,&status1,0)==-1 && errno!=ECHILD && errno!= EINTR) 
    {
        perror("Error , I am the parent"); 
        return 0;
    }
    if(waitpid(pid2,&status2,0)==-1 && errno!=ECHILD && errno!= EINTR) 
    {
        perror("Error , I am the parent"); 
        return 0;
    }
    return 1;
}


int check_pipe_sym (int count, char **arglist)
{
    int i ;
    for(i =0 ; i<count ; i++)
    {
        if(strcmp(arglist[i],"|")==0) return 1;
    
    }
    return 0;
}
int check_InputRedirecting_sym (int count, char **arglist)
{
    int i ;
    for(i =0 ; i<count ; i++)
    {
        if(strcmp(arglist[i],"<")==0) return 1;
        
    }
    return 0 ;
}
int check_OutputRedirecting_sym (int count, char **arglist)
{
    int i ;
    for(i =0 ; i<count ; i++)
    {
        if(strcmp(arglist[i],">")==0) return 1;
    
    }
    return 0 ;
}


int finalize() {
    return 0 ;/* didnt  change anything */
}