#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include "config.h"

// ANSI color codes
#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define RESET "\x1b[0m"

char lastExecutedCommand[1024] = "none";


// Tokenize a string into an array of strings
char ** tokenize(char * string) {
    int i, j, lastStart = 0, flag = 0, counter = 0;
    // allocating memory
    char ** tokens = malloc(256 * sizeof(char *));
    if(strlen(string) == 0){
        tokens[0] = malloc((2) * sizeof(char));
        tokens[0] = " \0";
        return tokens;
    }
    for (i = 0; i < strlen(string); i++) {
        char ch = string[i];
        // printf("i:%d ch:%c\n", i,ch);
        // delimiters
        if ( ch == '&' ) {
            // if we were previously building up a token, add it to the array
            if (flag) {
                flag = 0;
                tokens[counter] = malloc((i - lastStart + 1) * sizeof(char));
                for (j = lastStart; j < i; j++) {
                    tokens[counter][j - lastStart] = string[j];
                }
                tokens[counter][i - lastStart] = '\0';
                counter++;
            }
            // add the delimiter character as a separate token
            tokens[counter] = malloc((2) * sizeof(char));
            tokens[counter][0] = ch;
            tokens[counter][1] = '\0';
            counter++;
        }
        else if(ch == '\n' || ch == ' '|| ch == '\"'){
            // if we were previously building up a token, add it to the array
            if (flag) {
                flag = 0;
                tokens[counter] = malloc((i - lastStart + 1) * sizeof(char));
                for (j = lastStart; j < i; j++) {
                    tokens[counter][(j - lastStart)] = string[j];
                }
                tokens[counter][i - lastStart] = '\0';
                counter++;
            }
        }
        else {
            if (!flag) {
                lastStart = i;
            }
            flag = 1;
        }
    }
    // if we were previously building up a token, add it to the array
    if (flag) {
        tokens[counter] = malloc((i - lastStart) * sizeof(char));
        for (j = lastStart; j < i; j++) {
            tokens[counter][j - lastStart] = string[j];
        }
        tokens[counter][i - lastStart] = '\0';
    }
    tokens[counter+1] = NULL;
    /*
    for(int i = 0; i<256;i++){
        if(tokens[i]!=NULL)
            printf("i: %d : %s \n",i, tokens[i]);
    }
    */
    return tokens;
}

// Checks if a file exists
int file_exists(const char *filename) {
    FILE *file;
    if ((file = fopen(filename, "r"))) {
        fclose(file);
        return 1;
    }
    return 0;
}

// Checks if a file exists in the given PATH
char * check_file_in_path(char *filename, char *path) {
    char *token;
    char *path_copy = strdup(path);

    if (path_copy == NULL) {
        printf("Memory allocation error.\n");
        free(path_copy);
        return NULL;
    }

    char delimeter[1] = ":";
    token = strtok(path_copy, delimeter);

    while (token != NULL) {
        char full_path[256]; 
        snprintf(full_path, sizeof(full_path), "%s/%s", token, filename);

        if (file_exists(full_path)) {
            //printf("File found: %s\n", full_path);
            return token;
        }

        token = strtok(NULL, ":");
    }

    //printf("File not found in the PATH.\n");
    free(path_copy);
    return NULL;
}


// Reverses the content of a file and appends it to another file
void reverseAndAppend(const char *inputFileName, const char *outputFileName) {
    FILE *inputFile, *outputFile;
    char *buffer;
    long fileSize;
    size_t bytesRead;

    // Open input file for reading
    inputFile = fopen(inputFileName, "rb");
    if (inputFile == NULL) {
        printf("Error opening input file.\n");
        return;
    }

    // Open output file for appending
    outputFile = fopen(outputFileName, "ab");
    if (outputFile == NULL) {
        printf("Error opening output file.\n");
        fclose(inputFile);
        return;
    }

    // Get input file size
    fseek(inputFile, 0, SEEK_END);
    fileSize = ftell(inputFile);
    fseek(inputFile, 0, SEEK_SET);

    // Allocate memory for buffer to hold file content
    buffer = (char *)malloc(fileSize * sizeof(char));
    if (buffer == NULL) {
        printf("Memory allocation failed.\n");
        fclose(inputFile);
        fclose(outputFile);
        return;
    }

    // Read content of input file into buffer
    bytesRead = fread(buffer, sizeof(char), fileSize, inputFile);
    if (bytesRead != fileSize) {
        printf("Error reading input file.\n");
        free(buffer);
        fclose(inputFile);
        fclose(outputFile);
        return;
    }

    // Reverse the content in the buffer
    for (long i = 0, j = fileSize - 1; i < j; i++, j--) {
        char temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
    }

    // Write reversed content to output file
    fwrite(buffer, sizeof(char), fileSize, outputFile);

    // Clean up and close files
    free(buffer);
    fclose(inputFile);
    fclose(outputFile);
}

// Counts the number of children of a process recursively
int childrenCount(pid_t pid){
    char path[256];
    int count = 0;
    sprintf(path, "/proc/%u/task/%u/children", pid, pid);
    FILE *file_pointer;
    file_pointer = fopen(path,"r");
    char processes[256] = "\0";
    fgets(processes, sizeof(processes), file_pointer);
    fclose(file_pointer);
    if(processes[0] == '\0'){
        return 0;
    }
    char* token = strtok(processes, " ");
    while(token!=NULL){
        count++;
        count += childrenCount(atoi(token));
        token = strtok(NULL, " ");
    }
    return count;
}

// Kills zombie children
void handleZombies(){
    pid_t pid;
    int status;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        printf("[%d]\t\tdone \n", pid);
    }
}

// Returns the name of the process with the given pid
char* processName(pid_t pid){
    char path[256];
    sprintf(path, "/proc/%u/status", pid);
    FILE *file_pointer;
    file_pointer = fopen(path,"r");
    char status[256] = "\0";
    while(fgets(status, sizeof(status), file_pointer)!=NULL){
        if(strstr(status, "Name:")){
            char* token;
            token = strtok(status, "\t");
            token = strtok(NULL, "\n");
            fclose(file_pointer);
            char * name = malloc(strlen(token)+1);
            strcpy(name,token);
            return name;
        }
    }
    fclose(file_pointer);
    return NULL;
}

void bello(){
    char hostname[256];
    gethostname(hostname,256);
    printf("Username: %s\n", getenv("USER"));
    printf("Hostname: %s\n", hostname);
    printf("Last Executed Command: %s\n", lastExecutedCommand);
    printf("tty: %s\n", ttyname(STDIN_FILENO));
    char* shell = processName(getppid());
    printf("Shell: %s\n", shell);
    free(shell);
    printf("Home Location: %s\n", getenv("HOME"));
    time_t current_time;
    time(&current_time);
    printf("Current Time and Date: %s", ctime(&current_time));
    printf("Number of processes executing: %d\n", childrenCount(getpid())+1);
}


/**
 * @brief Redirects the standard output to a file with the specified type and filename.
 * 
 * @param type The type of redirection: 1 for truncation, 2 for appending.
 * @param filename The name of the file to redirect the output to.
 * @return The file descriptor of the original standard output, or -1 if an error occurs.
 */
int redirect(int type, char* filename){
    int param;
    if(type == 1)
        param = O_TRUNC;
    else if(type == 2)
        param = O_APPEND;
    int file = open(filename, O_WRONLY | O_CREAT | param , 0644);
    if (file == -1) {
        perror("Error opening file");
        return -1;
    }

    int stdoutnew = dup(STDOUT_FILENO);

    // Duplicate the file descriptor onto stdout (1)
    if (dup2(file, STDOUT_FILENO) == -1) {
        perror("Error duplicating file descriptor");
        return -1;
    }
    
    // Close the original file descriptor
    close(file);

    return stdoutnew;
}

void updateLastExecutedCommand(char** array){
    int index = 0;
    for(int i = 0; i<256;i++){
        if(array[i]== NULL){
            break;
        }
        for(int j = 0; j < strlen(array[i]); j++) {
            lastExecutedCommand[index] = array[i][j];
            index++;
        }
        lastExecutedCommand[index] = ' ';
        index++;
    }
    lastExecutedCommand[index] = '\0';
}

/**
 * Executes the command specified by the given array of arguments.
 * 
 * @param array The array of arguments representing the command.
 * @param redirectionType The type of redirection (0 for no redirection, 1 for '>', 2 for '>>').
 * @param redirectionFile The name of the file to redirect the output.
 * @param background Flag indicating whether the command should be executed in the background.
 * @return 0 if the command was executed successfully, -1 otherwise.
 */
int commandHandler(char** array, int redirectionType, char* redirectionFile, int background){
    // built in commands
    if(! strcmp(array[0],"bello")){
        if(redirectionType==0)
            bello();
        else{
            // redirect to file
            int newfd = redirect(redirectionType, redirectionFile);
            bello();
            dup(1);
            dup2(newfd,1);
        }
        lastExecutedCommand[0] = 'b'; lastExecutedCommand[1] = 'e'; lastExecutedCommand[2] = 'l';
        lastExecutedCommand[3] = 'l'; lastExecutedCommand[4] = 'o'; lastExecutedCommand[5] = '\0';    
    }
    else if(! strcmp(array[0],"exit")){
        exit(0);
    }
    else if(! strcmp(array[0],"wrong")){
        printf("wrong format");
    }
    else if(! strcmp(array[0]," ")){
        
    }
    // external commands
    else{
        char * path = getenv("PATH");
        path = check_file_in_path(array[0], path);
        // if the command is in the paths
        if(path != NULL){

            updateLastExecutedCommand(array);
            int newfd = 1;
            if(redirectionType>0)
                newfd = redirect(redirectionType, redirectionFile);
            pid_t pid = fork();
            if(pid < 0 ){
                //error
            }
            else if(pid == 0 ){ // child
                strcat(path,"/\0");
                execv(strcat(path,array[0]),array);
                //error
            }
            else{ // parent
                if(!background)
                    waitpid(pid,NULL,0);
                else{
                    printf("[%d]\n",pid);
                }
                if(redirectionType>0){
                    dup(1);
                    dup2(newfd,1);
                }
            }
        }
        // if not in the paths
        else{
            printf("Command not found\n");
            return(-1);
        }
        return 0;
    }
}

int main(void) {
    char * username = getenv("USER");
    char * pwd = getenv("PWD");
    char hostname[256];
    gethostname(hostname,256);
    char input[1024];
    char** array;

    KeyValuePair* head = load_config_from_file(CONFIG_FILE);

    while(1){

        pwd = getenv("PWD");
        printf(BLUE "%s@%s" GREEN " ~%s"RESET" --- ",username, hostname, pwd);

        // input taking
        if(fgets(input, 1024, stdin)!=NULL){
            input[strcspn(input, "\n")] = '\0'; // Remove trailing newline
            //printf("%s\n",input);
            array = tokenize(input);
            
            // killing zombie processes
            handleZombies();

            // if the command is alias
            if(! strcmp(array[0],"alias")){
                if( (array[1]!=NULL) && (! strcmp(array[2],"=")) && (array[3]!=NULL)){
                    strtok(input,"=");
                    head = set_config_value(head,array[1],strtok(NULL, "="));

                    for (int i = 0; i < strlen(input); i++) {
                        lastExecutedCommand[i] = input[i];
                    }
                }
                continue;
            }
            // checks if the command stands for an alias
            else{
                if(get_config_value(head,array[0])!=NULL){
                    char * a = get_config_value(head,array[0]);
                    strtok(input," ");
                    char* temp = strtok(NULL,"\n");
                    if(temp!=NULL){
                        strcat(a," \0");
                        strcat(a,temp);
                    }
                    array = tokenize(a);
                }
            }  

            int redirectionType = 0;
            char* redirectionFile; 
            int wrong = 0;
            int background = 0;

            // checks for redirection and background
            for(int i = 0; i<256;i++){
                if(array[i]== NULL){
                    break;
                }
                if(wrong){
                    array[0] = "wrong";
                }
                //printf( "%s \n", array[i]);
                if(redirectionType!=0){
                    redirectionFile = array[i];
                    wrong = 1;
                    if(array[i+1]!= NULL){
                        if(! strcmp(array[i+1],"&")){
                            background = 1;
                            i++;
                        }
                    }
                    else{
                        break;
                    }
                }
                else if(! strcmp(array[i],">")){
                    redirectionType = 1;
                    array[i] = NULL;
                }
                else if(! strcmp(array[i],">>")){
                    redirectionType = 2;
                    array[i] = NULL;
                }
                else if(! strcmp(array[i],">>>")){
                    redirectionType = 3;
                    array[i] = NULL;
                }
                else if(! strcmp(array[i],"&")){
                    background = 1;
                    wrong = 1;
                    array[i] = NULL;
                }
            }

            // if reverse redirection is required
            if(redirectionType ==3){
                int newfd = 1;
                pid_t pid = fork();
                if(pid < 0 ){
                    //error
                }
                else if(pid == 0 ){ // middle child
                    char filename[256];
                    sprintf(filename,".tempredirection%d",getpid());
                    if(commandHandler(array,1,filename,0) == 0){
                        // read .tempredirection & reverse them & write them into redirectionfile
                        reverseAndAppend(filename,redirectionFile);
                    } 
                    remove(filename);
                    exit(0);
                }
                else{
                    if(!background)
                        waitpid(pid,NULL,0);                   
                    //dup(1);
                    //dup2(newfd,1);
                }
            }
            else
                commandHandler(array,redirectionType,redirectionFile,background);
            free(array); 
        }
    }
    return 0;
}

