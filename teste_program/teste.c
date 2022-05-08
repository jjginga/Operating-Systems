/*
** The Following program was developed by me, Joel José Ginga, while studying at Universidade Aberta (www.uab.pt)
** for the subject Operating Systems.
** This program allows us to test another c program -  in linux operating system.
** It needs two arguments, the program to test ant the test cases.
**          teste <test_program.c> <test_cases.txt>
** This program then compiles the test_program.c, and executes it - in a child process created using fork -
** using what is in the odd lines of test_cases.txt, and compares
** the result to what is on the next even line, recording the output of each test in the out<test_id>.txt file, where test_id
** is the index of the test, starting at zero. 
** at the end of the program, the failed tests are printed.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>


#define MAX_STR 255

/**Function prototypes**/
/*Auxiliary functions*/
FILE* openFile(char* fileName);
char** getReallocedArray(char*** array, int j);
void addToArray(char*** array, char* str, int j);
void nullTerminateArray(char*** array, int j);
char* getProgName(char* fileName);
char* getOutputFilename(int i);
char* getDottedProgName(char* progName);
void getValuesFromFile(FILE* fp, char*** testCase, char*** expectedResult,int* n);
char** getArgs(char* progName, char* test);

/* Run the child process */
/* Used for compiling test_program.c and running tests, depending on the arguments - if outputFileName is NULL we are compiling
if it has a value we are running a test*/
void runChildProcess(char* progName, char* argument,char* outputFileName, char* message, char* error){
  
    //we start by creating arg0 for execvp
    char arg0[strlen(progName)+2];
    strcpy(arg0, progName);

    /* we then create a child process */
    pid_t pid = fork();
    
    /* if pid == - 1 then an error has happened and the child process wasn't created, so we terminate execution */
    if(pid == -1){
        perror("Error creating a new process.\n");
        exit(1);
    }

    /* if pid=0 we are in the child process */
    if (pid == 0){
        printf("Processo Child:  PID=%5d  PPID=%5d %s.\n", (int) getpid(), (int) getppid(), message);  

        /*if we are running a test - i.e. outputFileName has a value - we get the get the string with the test_progam name preceded by ./
        and direct stdout to the file*/
        if(outputFileName){
            strcpy(arg0, getDottedProgName(progName));
            stdout = freopen(outputFileName, "wt", stdout);
        } else {
            /*otherwise we direct the stderr to /dev/null because we don't wan't compilation errors for the test_program only for ours*/
            freopen("/dev/null", "w", stderr);
        }

        /* we use execvp to overlay the process image created by fork  */ 
        execvp(arg0,getArgs(progName, argument));
        /* if the function returns it is because an error has happened, so we terminate execution*/
        perror(error);
        exit(1);

    } else {
        /* We are in the parent process */           
        int status;
        /* we wait for the child process to terminate */ 
        waitpid(pid,&status,0);

        /* if the child process terminates with and error, we print it and terminate execution */
        if(status!=0){
           errno = EPERM;
           perror(error);
           exit(1);
        }

    }

}

int main(int argc, char *argv[]){

    /*if we have the wrong number of arguments we print an usefull error and terminate*/
    if(argc!=3){
        errno = EPERM;
        perror("Wrong number of arguments: teste <test_program.c> <test_cases.txt>.\n");
        exit(1);
    }

    char* fileName = argv[1];
    char* progName = getProgName(fileName);

    if(strlen(progName)<=0){
        errno=EINVAL;
        printf("Incorrect test program name <test_program.c>.\n");
        exit(1);
    }

    /*arguments for compiling test_program.c*/
    char* compileArguments = malloc(255*sizeof(char));
    sprintf(compileArguments, "%s -o %s", fileName, progName); 

    printf("Process:  PID=%5d  PPID=%5d [main].\n", (int) getpid(), (int) getppid());

    /* we use runChildProcess to compile it */
    runChildProcess("gcc", compileArguments,NULL,"[compilacao]","Erro a compilar.");
    free(compileArguments);

    /* we get the test_cases and expected results from the  test_cases.txt*/
    int n=0;
    char** testCase = malloc(sizeof(char*));
    char** expectedResult = malloc(sizeof(char*));

    FILE *fp=openFile(argv[2]);
    getValuesFromFile(openFile(argv[2]), &testCase, &expectedResult, &n);
    fclose(fp);

    

    /* we iterate through the testCase array, and for each test we use the runChildProcess to execute it, passing along an error message */
    for(int i = 0 ; i<n ; i++){   
        char* message = malloc(255*sizeof(char));
        sprintf(message, "[executing case %d]", i);
        runChildProcess(progName, testCase[i], getOutputFilename(i),message,"Error executing the test");
    }

    /* we iterate again and read the value stored in out<test_id>.txt and check if is the expected */
    for(int i = 0 ; i<n ; i++){
        char* str=malloc(MAX_STR*sizeof(char));
        fp=openFile(getOutputFilename(i));
        fgets(str,MAX_STR,fp);
        fclose(fp);
        
        if(strcmp(str,expectedResult[i])){
            printf("Error in case %d: expecting '%s' got '%s'.\n", i, expectedResult[i], str);
        }

        free(str);
    }

    printf("Number of executed test cases: %d.\n", n);

    free(testCase);
    free(expectedResult);

    return 0;
}

/* Open the file stream */
FILE* openFile(char* fileName){
    FILE *fp;
    if ((fp = fopen(fileName,"r")) == NULL){
        perror("Error! opening file\n");
        exit(1);
    }
    return fp;
}

/* Realloc array to increase it's size */
char** getReallocedArray(char*** array, int j){
    char** tmp = realloc(*array, (j+1)*sizeof(char*));
    
    if(tmp==NULL){
        free(tmp);
        perror("Error! Realloc failed!\n");
        exit(1);
    }

    return tmp;
}

/* add and element to the array */
void addToArray(char*** array, char* str, int j){
  
    char** tmp = getReallocedArray(array,j);  

    *array = tmp;
    (*array)[j] = strdup(str);    
}


/* NULL terminate the array for the exec function */
void nullTerminateArray(char*** array, int j){
  
    char** tmp = getReallocedArray(array,j);  
    *array = tmp;   
    (*array)[j] = NULL;    
}

/* Get the name of the compiled test_program based on the original name */
char* getProgName(char* fileName){
    size_t position = strcspn(fileName,".");
    char *progName=malloc(sizeof(char)*(position+1));
    
    if(position<strlen(fileName))
        strncpy(progName, fileName, position);

    return progName;
}

/* Get the name of the output text file*/
char* getOutputFilename(int i){
    char* outputFileName = malloc(20*sizeof(char));
    sprintf(outputFileName, "out%d.txt", i);
    return outputFileName;
}

/* get the string with the name of the executable preceded by ./ */
char* getDottedProgName(char* progName){
    char* dottedProgName = malloc((strlen(progName)+3)*sizeof(progName));
    sprintf(dottedProgName, "./%s", progName);
    
    return dottedProgName;
}

/* Get the values from the test_cases file */
void getValuesFromFile(FILE* fp, char*** testCase, char*** expectedResult,int* n){
        
    char* str = malloc(MAX_STR*sizeof(char)); 

    while(fgets(str,MAX_STR,fp)!=NULL){

        str[strcspn(str, "\r\n")]='\0';
        addToArray(testCase, str, (*n));
       
        fgets(str,MAX_STR,fp);
        str[strcspn(str, "\r\n")]='\0';
        addToArray(expectedResult, str, (*n)++);
    }

    free(str);
}


/* get the arguments for the exec function */
char** getArgs(char* progName, char* test){
    char **args = malloc(sizeof(char*));
    int i=0;

    addToArray(&args, progName, i++);

    char* arg;
    arg = strtok (test," ");
    while (arg != NULL){
        addToArray(&args, arg, i++);
        arg = strtok (NULL, " ");
    }

    nullTerminateArray(&args, i++);

    return args;
}
