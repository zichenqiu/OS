#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h> 
#include<fcntl.h>
#include<string.h>
#include<ctype.h>
#define DELIM " \t\r\n\a"
#define READ 0
#define WRITE 1
char* path[100]; // paths
char error_message[30] = "An error has occurred\n";
char prompt_message[10] = "wish> ";
int isBatch = 0;
char *historyStr[1024]; 
int counter = 0;

//method declarations
int getCommand(char *input[], FILE *f);
int executeCommand(char *input[], char *red[]);	

void printError() {
	write(STDERR_FILENO, error_message, strlen(error_message));
}

void printPrompt() {
	write(STDOUT_FILENO, prompt_message, strlen(prompt_message));
   	fflush(stdout);
}

int tokenization(char *line, char *input[], char *deliminator) {
	char *saveptr;

	input[0] = strtok_r(line, deliminator, &saveptr);
	if (input[0] == NULL) {
		return 0;
	}

	int i = 1;
	while (1) {
		input[i] = strtok_r(NULL, deliminator, &saveptr);
		
		if(input[i] == NULL) {
			break;
		}
		i++;
	}
	return i; //return the size of words[]
}

void redirect_prompt(char *redirect, char *line) {
	char *input_l[512];
	char *output_r[512];

	redirect[0] = '\0'; 
	redirect = redirect + 1;

	int n1 = tokenization(line, input_l, DELIM);
	if (n1 == 0) {
		printError();
	}
	int n2 = tokenization(redirect, output_r, DELIM);
	if (n2 != 1) {
		printError();
	}
	executeCommand(input_l, output_r);
}
	
char *find_path(char *input[]){
	int isFound = 0;
	//char *filePath = malloc(1024);
        //execute left_command	 
	if (path[0] == NULL) {
		printError(); 
		return NULL;
	}

	if ((input[0] == NULL)||(input == NULL)) {
		return NULL;
	}
	
	int i = 0;
	char *executable_path = malloc(1024);
	while(1) {
		if (path[i] == NULL) {
			break;
		}

		char *temp = malloc(1024);
		strcpy(temp, path[i]);
		temp[strlen(temp)] = '/';

		strcat(temp, input[0]);
	        if (access(temp, X_OK) == 0) {
			strcpy(executable_path, temp);
		        isFound = 1;	
			free(temp);
			//break;
			return executable_path;
		}
		i++;
	}
	if (isFound == 0) {
		printError();
		return NULL;
	}
	return NULL;
}

int spawn_process(int in, int out, int argc, char **argv) {
	pid_t pid = fork();
	
	if (pid == 0) {
		if (in != 0) {
			dup2(in, 0); //stdin is now in
			close(in);
		}
		if (out != 1) {
			dup2(out, 1);//stdout is now out
			close(out);
		}
		return execvp(argv[0], argv);
	}
	return pid;

}
	
int pipe_prompt(char *pipeline, char* line) {
	//allocate memory
	char *left_command = malloc(strlen(line) + 1);
	char *right_command = malloc(strlen(line) + 1);
	
	int left_command_size = pipeline - line;
        int right_command_size = strlen(line) - left_command_size -1;
	left_command = strncpy(left_command, line, left_command_size);
	left_command[left_command_size] = '\0';
	right_command = strcpy(right_command, pipeline + 1);
	right_command[right_command_size] = '\0';

	pid_t pid = fork(); //fork a process to execuate all commands
	if(pid == 0) {
		int p[2];
		char *argv[1024];
		int argc = tokenization(left_command, argv, DELIM);
		char *left_executable_path = find_path(argv);
		argv[0] = left_executable_path;
		pipe(p);
		spawn_process(0, p[WRITE], argc, argv);
		close(p[WRITE]);

		//execute the last commnad
		argc = tokenization(right_command, argv, DELIM);
		char *right_executable_path = find_path(argv);
		argv[0] = right_executable_path;	
		dup2(p[READ], 0);
		execvp(argv[0], argv);
	} else {
		int status;
		wait(&status);
	}	
	free(left_command);
	free(right_command);
	return 0;
}

int getCommand(char *input[], FILE *f) {
        char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	
	if ((nread = getline(&line, &len, f)) == -1) {
		return 1; 
	}	
	
	// if hit EOF marker (end of file), exit gracefully
	if (line[0] == EOF) {
		//return 1;
		exit(0); 
	}
	

	if (strcmp(line, "\n") == 0) {
		//continue
		return -1;
	}
	
        if (strcmp(line, "") == 0) {
		// cotinue
		return -1;
	}
	//white space
	int ws_count = 0;
	while (isspace(line[ws_count])) {
		ws_count++;
	}	
	if (ws_count == strlen(line)) {
		return -1;
	}
	
	// get rid of the '\n' in the end
        if (line[strlen(line) - 1] == '\n') {
		line[strlen(line) - 1] = '\0';
	}
	
	//history entries        
	
	historyStr[counter] = malloc(strlen(line) + 1);
	strcpy(historyStr[counter], line);	
	counter++;	

	//redirect
	char *redirect;	
	if ((redirect = strchr(line, '>'))) {
		redirect_prompt(redirect, line);
		return -1;
	}
	
	//pipe
	char *pipeline;
	if ((pipeline = strchr(line, '|'))) {
		pipe_prompt(pipeline, line);
		return -1;
	}

        //tokenization
	//int sizewords = tokenization(line, input, DELIM);
	tokenization(line, input, DELIM);

	//history if requested
	if (strcmp(input[0], "history") == 0) {
		int start = 0;
		int n = counter; 
		
		// error: > 2 arguments
		if (input[2] != NULL) {
			printError();
			return -1;
		}

		//history w/ arguments: update n
		if (input[1] != NULL) {
			n = atoi(input[1]);
			
		}

		//update starting point based on n
		if (n < counter) {
			start = counter - n;
		}
                	
		//execute commnad	
		for (int i = start; i < counter; i++) {
			printf("%s\n", historyStr[i]);
		}			
		fflush(stdout);
		return -1;
	}

      
	//exit if requested
	if (strcmp(input[0], "exit") == 0) {
		if (input[1] != NULL) {
			printError();  
		} else {
			exit(0);
		}
	}	

	//cd: always take one argument (O or >1 error)
	if (strcmp(input[0], "cd") == 0) {
		if ((input[1] == NULL) || (input[2] != NULL)) {
			printError();
		} else {
			int cd_status = chdir(input[1]);
			if (cd_status == -1) {
				printError();
			}
		}
		return -1;
	}

	//path: 0 or more
	if (strcmp(input[0], "path") == 0) {
		int i = 0;
		while(1) {
		        if ((path[i] = input[i + 1]) == NULL) 
				break;
			i++;
		}	
		return -1;	
	}
		
	return 0;
}
int executeCommand(char *input[], char *redirect[]) {
	//char *finalPath = malloc(1024);
	int isFound = 0;
	int pid;
	int status;

        if (path[0] == NULL) {
		printError();
		return 1; 
	}

	if ((input[0] == NULL)||(input == NULL)) {
		return 1;
	}

	int i = 0;
	char *filePath = malloc(1024);

	while(1) {
		if (path[i] == NULL) {
			break;
		}

		char *temp = malloc(1024);
		strcpy(temp, path[i]);
		temp[strlen(temp)] = '/';

		strcat(temp, input[0]);
	        if (access(temp, X_OK) == 0) {

			strcpy(filePath, temp); 
			isFound = 1;
			free(temp);
			break;
		}
		i++;
	}

	if (isFound == 0) {
		printError();
		return 1;
	}
	
	//Upon finding, now we shall execute
	pid = fork();
		
	if (pid == 0) {		
		if (redirect) {
			int fout = open(redirect[0], O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
			if (fout > 0) {
				//redirect STDOUT
				dup2(fout, STDOUT_FILENO);
				//redirect STDERR
				dup2(fout, STDERR_FILENO);
				
				fflush(stdout);
				fflush(stderr);
			}
		}
		execv(filePath, input);
	}
	wait(&status);
	return -1;
}

int main(int argc, char*argv[]) {
	FILE *f;
	char* input[1024];
	path[0] = "/bin";
	int isBatch = 0;

	if (argc == 1) {
		f = stdin;
	}

	else if (argc == 2) {
		if (!(f = fopen(argv[1], "r"))) {
			printError();
			exit(1);
		}
		isBatch = 1;
		
	} else if (argc < 1 || argc > 2) {
		//anything other than 1 argument or 2 argument (0 or 1) is not right
		printError();
		exit(1);
	}

	while(1) {
		if (isBatch == 1) {
			// printPrompt();		
			int readInStatus  =  getCommand(input, f); 
			if (readInStatus == -1) {
				continue;
			} 
			if (readInStatus == 1 ) {
				break; 
			}
			executeCommand(input, NULL);
		
		} else if (isBatch == 0) {
		        printPrompt();		
			int readInStatus  =  getCommand(input, f); 
			if (readInStatus == -1) {
				continue;
			} 
			if (readInStatus == 1 ) {
				break; 
			}
			//execute the command 
			executeCommand(input, NULL);
			
		}
	}
	return 0;
}








