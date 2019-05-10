#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
	FILE *f;
	//int buffersize = 1024;
	//char buffer[buffersize];
	
	char *line = NULL;

	size_t len = 0;
	ssize_t nread;

	if (argc < 2) {
		exit(0);
	}
	
	for (int i = 1; i < argc; i++){
		f = fopen(argv[i], "r");
		if (f == NULL) {
			printf("my-cat: cannot open file\n");
			exit(1);
		}	
	
	
		while(1){
			if((nread = getline(&line, &len, f)) != -1) {
			//	printf("%s", buffer);
				printf("%s", line);
			}
			else {
				break;
			}
		}
		free(line);
		fclose(f);
	}
}
