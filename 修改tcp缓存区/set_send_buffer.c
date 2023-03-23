#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>


#define BUFFER_SIZE 512


int main(int argc, char* argv[]){
    if(argc <= 2){
    	printf("参数输入错误");
        return 1;
    }
    
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    
    struct sockaddr_in server_address;
    bzero(&server_address,sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&server_address.sin_addr);
    server_address.sin_port = htons(port);

    int sock = socket(PF_INET,SOCK_STREAM,0);
    assert(sock >= 0);
    
    int sendbuf = atoi(argv[3]);
    int len = 0;
    //设置TCP缓冲区大小，同时检查是否成功并且打印提示
    setsockopt(sock,SOL_SOCKET,SO_SNDBUF,&sendbuf,sizeof(sendbuf));
    getsockopt(sock,SOL_SOCKET,SO_SNDBUF,&sendbuf,(socklen_t*)&len);
    printf("tcp send buffer size is setted %d\n",sendbuf);
    printf("tcp send buffer size is actual setted %d\n",len);
    
    if(connect(sock,(struct sockaddr*)&server_address,sizeof(server_address)) < 0){
        printf("connection failed\n");
    }
    else{
    	char buffer[BUFFER_SIZE];
    	memset(buffer,'a',BUFFER_SIZE);
    	
    	send(sock,buffer,BUFFER_SIZE,0);
    }
    close(sock);
    return 0;
}
