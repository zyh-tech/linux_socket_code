#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/stat.h>
#include<fcntl.h>


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
    
    if(connect(sock,(struct sockaddr*)&server_address,sizeof(server_address)) < 0){
        printf("connection failed\n");
    }
    else{
    	//这里输入发送的内容
    	char *data = "hello , i am client.";
		write(sock,data,strlen(data));

		char recvBuf[1024] = {0}; 
		while(read(sock,recvBuf,sizeof(recvBuf)) > 0){
        	printf("recv data :%s\n",recvBuf);
    	}
    }
    close(sock);
    return 0;
}
