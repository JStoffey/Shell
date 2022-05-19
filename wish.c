#include <stdio.h>
#include<fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

char path[200] = "/bin";

char *buildinCmd[] = {"exit", "cd", "path"};

char **paths;

int main(int argc, const char * argv[]) {
    void execCmd(char **cmds, char *filename, bool redir);
    char **SplitArgs(char *line, char *delim);
    void errMsg();
    void processCmd(char *line);
    
    //batch mode
    if(argv[1] != NULL){
        if(argv[2] != NULL){
            errMsg();
            exit(1);
        }

        char const* const fpname = argv[1];
        FILE *fp = fopen(fpname, "r");
        char line[256];
        
        if(fp == NULL){
            errMsg();
            exit(1);
        }
        
        while(fgets(line, sizeof(line), fp)){
            processCmd(line);
        }
 
        fclose(fp);
        exit(0);
    }
    
    //interactive mode
    while(1){
        printf("wish> ");
        
        char *line = NULL;
        size_t len = 0;
        getline(&line, &len, stdin);
        
        processCmd(line);
        
    }
    
    return 0;
}

//Split arguments
char **SplitArgs(char *line,char *delim){
    int bufferSize = 100;
    
    char **cmds = malloc(bufferSize*sizeof(char *));
    char *cmd = NULL;
    
    int i = 0;
    cmd = strtok(line, delim);
    
    while(cmd != NULL){
        cmds[i] = cmd;
        i++;
        cmd = strtok(NULL, delim);
    }
    cmds[i] = NULL;
    return cmds;
}

//Write error.
void errMsg(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

//Execute given command
void execCmd(char **cmds, char *filename, bool redir){
    int cmdLength = sizeof(buildinCmd)/sizeof(buildinCmd[0]);
    int code = -1;
    int i;
    for(i=0; i < cmdLength; i++){
        if(strcasecmp(buildinCmd[i], cmds[0]) == 0){
            code = i;
            break;
        }
    }
    
    if(code != -1){
        switch (code) {
            case 0:
                //Check exit
                if(cmds[1] != NULL){
                    errMsg();
                }
                else{
                    exit(0);
                }
                break;
            case 1:
                //Check cd
                if(cmds[1] == NULL){
                    errMsg();
                }
                else if(cmds[2] != NULL){
                    errMsg();
                }
                else {
                    if(chdir(cmds[1]) != 0){
                        errMsg();
                    }
                }
                break;
            case 2:
                //Check path
                strcpy(path, "");
                int j = 1;
                while(cmds[j] != NULL){
                    strcat(path, cmds[j]);
                    strcat(path," ");
                    j++;
                }
                break;
            default:
                break;
        }
    }
    
    //Fork for command
    else{ 
        //int status;
        //pid_t pid = fork();
        //if (pid == 0) { //child process  
        char **cmdPaths = SplitArgs(path, " \t\r\n\a");
        bool searchPath = false;
        
        int i = 0;
        char *cmdPath = NULL;
        //Add command to end of path for execv
        while(cmdPaths[i] != NULL){
            cmdPath = (char *)malloc(strlen(cmdPaths[i]) + strlen(cmds[0]) + 1);
            strcpy(cmdPath, cmdPaths[i]);
            strcat(cmdPath, "/");
            strcat(cmdPath, cmds[0]);
            printf("%s\n", cmdPath);

            //Check if path is viable
            if(access(cmdPath, X_OK)==0){
                searchPath = true;
                if(!redir){
                    int status;
                    pid_t pid = fork();

                    if (pid == 0) { //child process  
                        if(execv(cmdPath, cmds) == -1){
                            errMsg();
                            exit(EXIT_FAILURE);
                        }
                    }

                    else if(pid < 0){
                        errMsg();
                    }

                    //Wait for child
                    else {
                        do {
                            waitpid(pid, &status, WUNTRACED);
                        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
                    }
                }
                //break;
            }else{
                errMsg();
                i++;
            }
            free(cmdPath);
            i++;
        }

        //Check Redirection
        if(redir){
            int fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            dup2(fd, 1);
            dup2(fd, 2);
            close(fd);
            if(searchPath){
                int status;
                pid_t pid = fork();

                if (pid == 0) { //child process  
                    execv(cmdPath, cmds);
                }

                //Fork Error.
                else if (pid < 0) {
                    errMsg();
                }
    
                //Wait for child
                else {
                    do {
                        waitpid(pid, &status, WUNTRACED);
                    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
                }
            }
        }
            

        //For Error.
        /*else if (pid < 0) {
            errMsg();
        }
        
        //Wait for child
        else {
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }*/
    }
}


//Process batch command
void processCmd(char *line){
    char **redirCmds = NULL;
    char **par = NULL;
    char **cmds = NULL;
    
    //Parallel check
    par = SplitArgs(line, "&");
    
    int i = 0;
    //Check redirection errors.
    while(par[i] != NULL){
        cmds = NULL;

        char *redirError = strstr(par[i], ">>");
        if(redirError){
            errMsg();
            break;
        }

        redirCmds = SplitArgs(par[i], ">");
        if(redirCmds[1] != NULL){ 
            errMsg();
            break;
        }

        if(redirCmds[1] != NULL && redirCmds[2] != NULL){
            errMsg();
            break;
        }

        else if(redirCmds[1] != NULL){
            char **filename = SplitArgs(redirCmds[1], " \t\r\n\a");
            
            if(filename[1] != NULL){
                errMsg();
                break;
            }
            cmds = NULL;
            cmds = SplitArgs(redirCmds[0]," \t\r\n\a");
            if(cmds[0]!=NULL){
                execCmd(cmds, filename[0], true);
            }
        }
        
        else{
            cmds = NULL;
            cmds = SplitArgs(redirCmds[0]," \t\r\n\a");
            if(cmds[0] != NULL){
                execCmd(cmds, NULL, false);
            }
        }
        i++;
    }
}