#include "shell.h"
#include <stdio.h>

const int CWD_BUF_SIZE = 256;
const int SCAN_BUF_SIZE = 256;
const int MAX_TOKENS = 50;
const int HISTORY_SIZE = 100;
const char* HISTORY_FILE = ".shell_history";

char* cwdBuf;  // chose to use global buffers to store the lines with configurable size parameters
char* scanBuf;
char** tokens;
int historyIndex;

pid_t pid;
int status;

int main(){
    historyIndex = getHistIndex();
    signal(SIGCHLD,proc_exit);
    while(1){
        cwdBuf = malloc(sizeof(char)*CWD_BUF_SIZE);
        scanBuf = malloc(sizeof(char)*SCAN_BUF_SIZE);
        tokens = malloc(sizeof(char*)*MAX_TOKENS);
        
        printCWD();
        scanInput();
        if(scanBuf[0]!='!') storeHistory(); // need to ensure that the command isn't calling history to prevent recursive history calls
        getTokens();
        executeTokens(); // using global vars for buffers/tokens is pretty convenient in this simple case

        free(cwdBuf);
        free(scanBuf);
        free(tokens);
    }
    return EXIT_SUCCESS;
}

void printCWD(){
    getcwd(cwdBuf, CWD_BUF_SIZE);
    printf("%s>",cwdBuf);
}

void scanInput(){
    if(fgets(scanBuf,SCAN_BUF_SIZE,stdin)==NULL){ // == NULL == uh oh!
        printf("Error scanning user input, shell must terminate...\n");
        exit(EXIT_FAILURE);
    }
    scanBuf[strcspn(scanBuf,"\n")] = '\0'; //remove the newline at the end
}

void getTokens(){
    int i=0;
    strtok(scanBuf," "); // buf holds the first token
    while(scanBuf != NULL){ // while there are more tokens to parse...
        tokens[i] = scanBuf; // record the char* of each token in an array
        scanBuf = strtok(NULL," ");
        i++;
    }
}

void executeTokens(){
    if(!strcmp(tokens[0],"exit")){  //if we should exit, exit
        exit(EXIT_SUCCESS);
    }else if(!strcmp(tokens[0],"cd")){
        if(chdir(tokens[1])){ // if we return nonzero, print an error message
            perror("cd");
        }
    }else if(!strcmp(tokens[0],"history")){
        FILE* histFile = fopen(HISTORY_FILE,"r"); // store shell history in a hidden file (like bash does)
        if(histFile == NULL){
            histFile = fopen(HISTORY_FILE,"w"); // if theres no history file, make one
            fclose(histFile);
        }else{
            char c = fgetc(histFile);
            for(int i=0;c!=EOF;i++){
                printf("   %d  ",i+1);
                while(c != '\n'){
                    printf("%c",c);
                    c = fgetc(histFile);
                }
                printf("\n");
                c = fgetc(histFile);
            }
            fclose(histFile);
        }
    }else if(tokens[0][0] == '!'){ // if the very first character is '!'
        int recallIndex = parseInt(&tokens[0][1]);
        if(0<recallIndex && recallIndex<HISTORY_SIZE+1 && recallIndex<=historyIndex){
            free(scanBuf); 
            free(tokens);
            scanBuf = malloc(sizeof(char)*SCAN_BUF_SIZE);
            tokens = malloc(sizeof(char*)*MAX_TOKENS);
            getHistory(recallIndex); // essentially just rerun the loop but with history as the fgets input
            printf("%s\n",scanBuf); // calling history prints the command you just recalled
            storeHistory();
            getTokens();
            executeTokens();
        }else printf("shell: %s: event not found\n",tokens[0]);
    }else if(strcmp(tokens[0],"")){  // so long as there is some command to run (i.e., tokens is not just a single "")
        char* inputFile = NULL;
        char* outputFile = NULL;
        stripRedirs(&inputFile,&outputFile);
        int bg = backgroundProc();
        
        pid = fork();
        if(pid < 0){
            printf("Error forking process, shell must terminate...\n");
            exit(EXIT_FAILURE);
        }else if(!pid){  // we're in the child
            if(inputFile!=NULL) freopen(inputFile,"r",stdin);
            if(outputFile!=NULL) freopen(outputFile,"w",stdout);
            execvp(tokens[0],tokens); // finishing execution correctly will send a 'done' flag to the parent's waitpid
            printf("%s: command not found\n",tokens[0]); // otherwise we just say we can't find the command
            exit(EXIT_SUCCESS); // since we don't execute anything, we need to manually kill the fork
        }else{ // if we're in the parent
            if(!bg){
                do{ // inspired by the man page for wait (and much trial and error)
                    waitpid(pid,&status,WUNTRACED | WCONTINUED); // WNOHANG is used in the signal handler, these options work here
                } while(!WIFEXITED(status) && !WIFSIGNALED(status));
            }
        }
    }
}

int indexOfToken(char* tok){ // check every token to see if any are the string tok
    for(int i=0;tokens[i]!=NULL;i++){
        if(!strcmp(tokens[i],tok)) return i; // return index of the token
    }
    return -1;
}

void stripRedirs(char** inputFile,char** outputFile){ // strip "<", ">", and the associated filenames from the user input
    int inputIndex = indexOfToken("<"); // turns out redirs can be anywhere in the command so long as filename follows them immediately, so ">output ls -a" works (in bash at least)
    if(inputIndex!=-1){
        *inputFile = tokens[inputIndex+1]; // filename is the token following "<"
        for(int i=inputIndex;tokens[i]!=NULL;i++){
            tokens[i] = tokens[i+2]; // we need to remove the "<" token and filenames from tokens; to do this we shift each token down by 2 until we shift the final NULL token
        }
    }
    int outputIndex = indexOfToken(">");
    if(outputIndex!=-1){
        *outputFile = tokens[outputIndex+1]; // filename is the token following "<"
        for(int i=outputIndex;tokens[i]!=NULL;i++){
            tokens[i] = tokens[i+2]; // we need to remove the "<" token and filenames from tokens; to do this we shift each token down by 2 until we shift the final NULL token
        }
    }
}

void printTokens(){
    for(int i=0;tokens[i]!=NULL;i++){
        printf("Token %d: %s\n",i,tokens[i]);
    }
}

int getHistIndex(){ // gets the current number of entries in .shell_history
    FILE* histFile = fopen(HISTORY_FILE,"r"); // store shell history in a hidden file (like bash does)
        if(histFile == NULL){
            return 0;
        }else{
            char c = fgetc(histFile);
            int i = 0;
            while(c!=EOF){
                if(c == '\n') i++; // inc i for each line in the file (each history entry is a line)
                c = fgetc(histFile);
            }
            fclose(histFile);
            return i;
        }
}

void storeHistory(){ // store what is currently in scanBuf to history file
    if(historyIndex < HISTORY_SIZE){
        FILE* histFile = fopen(HISTORY_FILE,"a"); // store shell history in a hidden file (like bash does)
        if(histFile == NULL){
            perror("history");
        }else{
            fprintf(histFile,"%s\n",scanBuf);
            fclose(histFile);
            historyIndex++;
        }
    }
}

int backgroundProc(){ // return true if final token ends with '&'
    char* finalTok = tokens[0];
    for(int i=0;tokens[i]!=NULL;i++){
        finalTok = tokens[i];
    }
    if(!strcmp(finalTok,"&")){
        finalTok = NULL;
        return 1;
    }else if(finalTok[strlen(finalTok)-1] == '&'){
        finalTok[strlen(finalTok)-1] = '\0'; // get rid of the '&' at the end of the token (in the case user inputs something like 'ls&')
        return 1;
    }
    return 0;
}

void proc_exit(){ // loop through any (non executing) children and see if they have finished, to clear out the process table
    while((waitpid(-1,&status,WNOHANG))>0); // we don't actually need to do anything other than call wait to reap all the finished processes
    // still need to do a while loop in case multiple children are finished at once (second one finishing interrupts the first one exiting)
}

int parseInt(char* str){ // theres definitely a library that does this, but honestly this is so trivial to do it might be less work than importing something
    int output = 0;
    for(int i=strlen(str)-1;i>=0;i--){
        int place = 1;
        for(int j=strlen(str)-1;j>i;j--){
            place *= 10;
        }
        int d = (int)str[i];
        if(47<d && d<58) output += place * (d-48);
        else return -1; // return -1 in the case that we can't parse an int
    }
    return output;
}

void getHistory(int index){
    FILE* histFile = fopen(HISTORY_FILE,"r"); // store shell history in a hidden file (like bash does)
    if(histFile == NULL){
        perror("history");
    }else{
        char c = fgetc(histFile);
        int i = 0;
        int currentIndex = 0;
        while(i<index){ // exit the loop asap
            if(i==index-1){
                scanBuf[currentIndex] = c;
                currentIndex++;
            }
            if(c == '\n') i++; // inc i for each line in the file (each history entry is a line)
            c = fgetc(histFile);
        }
        fclose(histFile);
    }
    scanBuf[strcspn(scanBuf,"\n")] = '\0'; //remove the newline at the end
}