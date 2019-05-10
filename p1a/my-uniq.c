#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
	FILE *f;
	char *line = NULL;
	char *lineprev = NULL;
	size_t len = 0;
	ssize_t nread;


	if (argc < 2) {
		f = stdin;	
		nread = getline(&lineprev, &len, f);
		if (nread != - 1) {
			printf("%s", lineprev);	
		}	
		while((nread = getline(&line, &len, f)) != -1 ) {
			if (strcmp(lineprev, line) != 0) {
				printf("%s", line);
			}
			free(lineprev);
			lineprev = line;
			line = NULL;
		}
		line = NULL;
		lineprev = NULL;
		fclose(f);
		exit(0);
	}

	
	for (int i = 1; i < argc; i++){
		f = fopen(argv[i], "r");
		if (f == NULL) {
			printf("my-uniq: cannot open file\n");
			exit(1);
		}	
		
		nread = getline(&lineprev, &len, f);
		if (nread != -1) {
		     	printf("%s", lineprev);
		}

		while ((nread = getline(&line, &len, f)) != -1 ) {
			//compare previous line
			if (strcmp(lineprev, line) != 0) { // not matched
				printf("%s", line);
			}
			free(lineprev);
			lineprev = line; //update prev line
			line = NULL;
		}
		line = NULL;
		lineprev = NULL;
		fclose(f);
	}
	

}
