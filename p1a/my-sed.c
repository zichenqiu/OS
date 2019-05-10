#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {

        if (argc < 3) {
		printf("my-sed: find_term replace_term [file ...]\n");
		exit(1);
	}
	

	FILE *f;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;

	
	char *searchWord = malloc(strlen(argv[1]) + 1);
	char *replaceWord = malloc(strlen(argv[2]) + 1);
	strcpy(searchWord, argv[1]);
	strcpy(replaceWord, argv[2]);

        if (argc == 3) {	
		f = stdin;
		while ((nread = getline(&line, &len, f)) != -1 ) {
			char *pos = strstr(line, searchWord);
			if (pos != NULL) {
				char *temp = malloc(nread + 1);
				strcpy(temp, line);
				int index = pos - line;
				line[index] = '\0';
				printf("%s", line);
				printf("%s", replaceWord);
				printf("%s", line + index + strlen(argv[1])); 
			} else {
				printf("%s", line);
			}
		}
			free(line);
			line = NULL;
			exit(0);
		
	}


	
	for (int i = 3; i < argc; i++) {
		f = fopen(argv[i], "r");
		
		if (f == NULL) {
			printf("my-sed: cannot open file\n");
			exit(1);
		}
	
		while ((nread = getline(&line, &len, f)) != -1) {
			char *pos = strstr(line, searchWord);
			
			if (pos != NULL) {
				char *temp = malloc(nread + 1);
				strcpy(temp, line);
				int index = pos - line;
				line[index] = '\0';
				strcat(line, replaceWord);
				strcat(line, temp + index + strlen(argv[1]));
				printf("%s", line);
			} else {
				printf("%s", line);
			}
		}

		free(line);
		fclose(f);
	}

}
