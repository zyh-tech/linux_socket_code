/* 这是一个聊天室程序的客户端程序 */
#define _GUN_SOURCE
#define BUFFER_SIZE 64

#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<poll.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include <features.h>
#include <sys/poll.h>
#include <signal.h>




int main(int argc, char* argv[])
{
	if (argc <= 0)
	{
		return 1;
	}
	const char* ip = argv[1];
	int port = atoi(argv[2]);

	struct sockaddr_in server_address;
	bzero(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	//inet_pton是一个IP地址转换函数，可以在将点分文本的IP地址转换为二进制网络字节序”的IP地址
	inet_pton(AF_INET, ip, &server_address.sin_addr);

	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(sockfd >= 0);
	if (connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0)
	{
		printf("connection failed\n");
		close(sockfd);
		return 1;
	}
	
	/* 前期准备工作 */
	struct pollfd fds[2];
	/* 注册文件描述符0(标准输入)和文件描述符 sockfd 上的可读事件 */
	fds[0].fd = 0;
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	fds[1].fd = sockfd;
	fds[1].events = POLLIN | POLLRDHUP;	/* 这里使用了 POLLRDHUP，所以本程序开头要进行 _GUN_SOURCE 的宏定义 */
	fds[1].revents = 0;
	char read_buf[BUFFER_SIZE];
	int pipefd[2];
	//pipefd[0]用于读，pipefd[1]表示写
	int ret = pipe(pipefd);
	assert(ret != -1);

	while (1)
	{
		ret = poll(fds, 2, -1);		/* 最后一个参数为-1，意味着 poll 调用将永远阻塞，这对 epoll 也同样适用 */
		if (ret < 0)
		{
			printf("poll failure\n");
			break;
		}

		if (fds[1].revents & POLLRDHUP)
		{
			printf("server close the connection\n");
			break;
		}
		else if (fds[1].revents & POLLIN)
		{
			memset(read_buf, '\0', BUFFER_SIZE);
			recv(fds[1].fd, read_buf, BUFFER_SIZE - 1, 0);
			printf("%s\n", read_buf);
		}

		if (fds[0].revents & POLLIN)
		{
			/* 使用 splice 将用户输入的数据直接写到 sockfd 上(零拷贝) */
			ret = splice(0, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
			ret = splice(pipefd[0], NULL, sockfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
			/* 这里使用 SPLICE_F_MORE 也要在程序开头进行 _GUN_SOURCE 的宏定义 */
		}
	}
	close(pipefd[1]);
	close(pipefd[0]);
	close(sockfd);
	return 0;
}

