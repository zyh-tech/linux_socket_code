#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>


#define BUFFER_SIZE 1024


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
    
    int recvbuf = atoi(argv[3]);
    int len = sizeof(recvbuf);
    //int len = 0;
    //设置TCP缓冲区大小，同时检查是否成功并且打印提示
    setsockopt(sock,SOL_SOCKET,SO_RCVBUF,&recvbuf,sizeof(recvbuf));
    getsockopt(sock,SOL_SOCKET,SO_RCVBUF,&recvbuf,(socklen_t*)&len);
    printf("tcp recv buffer size is setted %d\n",recvbuf);
    //printf("tcp recv buffer size is actual setted %d\n",len);
    
    int ret = bind(sock,(struct sockaddr*)&server_address, sizeof(server_address));
    assert(ret!=-1);
    
    ret = listen(sock,5);
    assert(ret != -1);
    
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock,(struct sockaddr*)&client, &client_addrlength);
    if(connfd < 0){
    	printf("errno is : %d\n" , errno);
    }else{
    	char buffer[BUFFER_SIZE];
    	memset(buffer,'\0',BUFFER_SIZE);
    	while(recv(connfd,buffer,BUFFER_SIZE-1,0) > 0){}
    	close(connfd);
    }
    
    close(sock);
    return 0;
}
