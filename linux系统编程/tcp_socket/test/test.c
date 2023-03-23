#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<ctype.h>
#include<sys/wait.h>
#include<sys/socket.h>

#define SRV_PORT 9999

void catch_child(int signum){
    while( (waitpid(0,NULL,WNOHANG)) > 0);
    
    return;
}

void perr_exit(const char *str){
	perror(str);
	exit(-1);
}

int main(int argc,char *argv[]){
    
    int lfd, cfd;
    pid_t pid;
    int ret;
    char buf[BUFSIZ];
    struct sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len;

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SRV_PORT);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    lfd = socket(AF_INET, SOCK_STREAM, 0);

    bind(lfd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));

    listen(lfd,128);

    clt_addr_len = sizeof(clt_addr);
    while(1){
        cfd = accept(lfd, (struct sockaddr*)&clt_addr, &clt_addr_len);
        pid = fork();
        if(pid < 0){
            perr_exit("fork error");
        }else if(pid == 0){
            //说明是子进程
            close(lfd);
            break;
        }else{
            //说明是父进程
            struct sigaction act;
            act.sa_handler = catch_child;
            sigemptyset(&act.sa_mask);
            act.sa_flags = 0;
            ret = sigaction(SIGCHLD, &act, NULL);
            if(ret != 0){
                perr_exit("sigaction error");
            }
            close(cfd);
            continue;
        }
    }


    if( pid == 0){
        while(1){
        ret = read(cfd, buf, sizeof(buf));
        if(ret == 0){
            close(cfd);
            exit(1);
        }
        for(int i=0; i<ret; i++)
            buf[i] = toupper(buf[i]);
        write(cfd, buf, ret);
        write(STDOUT_FILENO, buf, ret);
        }
    }
    
    return 0;
}
