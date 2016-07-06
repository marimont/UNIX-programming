#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char** argv){
	int fd, n, i, x;

	
	if(argc != 3){
			fprintf(stderr, "Invalid number of arguments\n");
			exit(1);
	}
	
	n = atoi(argv[1]);
	fd = creat(argv[2], S_IRUSR | S_IWUSR );
	
	if (fd < 0){
			fprintf(stdout, "Cannot open %s\n", argv[2]);
			exit(1);
	}

	srand(time(NULL));
	
	for(i = 0; i < n; i++){
		x = rand()%100+1;
		write(fd, &x, sizeof(int));
	}
	
	close(fd);
	
	return 0;
}
