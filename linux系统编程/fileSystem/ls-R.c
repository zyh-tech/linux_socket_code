#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>

void read_dir(char*);

void isFile(char *name){
	int ret = 0;
	struct stat sb;
	ret = stat(name,&sb);
	if(ret == -1){
		perror("stat error");
		return;
	}

	if(S_ISDIR(sb.st_mode)){
		read_dir(name);
	}

	printf("%s\t%ld\n",name,sb.st_size);

	return;
}


void read_dir(char* dir){
	DIR * dp;
	char path[256];
	struct dirent *sdp;
	dp = opendir(dir);

	if(dp == NULL){
		perror("opendir error");
		return;
	}
	while((sdp = readdir(dp)) != NULL){
		if(strcmp(sdp->d_name,".") == 0 || strcmp(sdp->d_name,"..") == 0){
			continue;
		}
		sprintf(path,"%s/%s",dir,sdp->d_name);
		isFile(path);
	}
	closedir(dp);
	return;
}


int main(int argc, char *argv[]){

	if(argc == 1){
		isFile(".");
	}else{
		isFile(argv[1]);
	}
	return 0;
}
