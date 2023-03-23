/* 这是聊天室程序的服务器程序 */
#define _GUN_SOURCE 1
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<assert.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<poll.h>

#define USER_LIMIT 5	/* 最大用户数量 */
#define BUFFER_SIZE 64	/* 读缓冲区的大小 */
#define FD_LIMIT 65535	/* 文件描述符数量限制 */

/* 客户数据结构体：客户端 socket 地址，待写到客户端的数据的位置，从客户端读入的数据 */
struct client_data
{
	sockaddr_in address;	/* 该客户端的 socket 地址 */
	char* write_buf;	/* 指针，指向服务器要发送给该客户端的数据的起始位置 */
	char buf[BUFFER_SIZE];  /* 存储该客户端发送给服务器的数据 */
};

int setnonblocking(int fd)
{
	//获取fd的标志
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	//设置fd的标志,在之前的标志上加上非阻塞
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

int main(int argc, char* argv[])
{
	if (argc <= 2)
	{
		return 1;
	}
	const char* ip = argv[1];
	int port = atoi(argv[2]);

	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	inet_pton(AF_INET, ip, &address.sin_addr);
	
	int listenfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(listenfd >= 0);

	int ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
	assert(ret != -1);

	ret = listen(listenfd, 5);
	assert(ret != -1);

	/* 创建 user 数组，分配 FD_LIMIT 个 client_data 对象。
	 * 可以预期，每个可能的 socket 连接都可以获得一个这样的对象，并且 socket 的值可以直接作为数组的下标来索引 socket 连接对应的 client_data 对象，
	 * 这是将 socket 和客户数据关联的简单而高效的方式
	 */
	client_data* users = new client_data[FD_LIMIT];
	/* 尽管我们分佩了足够多的 client_data 对象，但为了提高 poll 的性能，仍然有必要限制用户的数量 */
	pollfd fds[USER_LIMIT + 1];			/* 监听 socket 也要放入 fds 里，所以要 + 1 */
	int user_counter = 0;				/* user_counter 是已连接的客户数量，同时也是 fds 数组中 最后一个客户的下标 */
	for (int i = 1; i <= USER_LIMIT; i++)		/* 初始化 fds */
	{
		fds[i].fd = -1;
		fds[i].events = 0;
	}
	//设值用于监听的socket
	fds[0].fd = listenfd;
	fds[0].events = POLLIN | POLLERR; //监听的事件是输入或者错误
	fds[0].revents = 0;

	while (1)
	{
		ret = poll(fds, user_counter + 1, -1);		/* 阻塞的调用 poll */
		if (ret < 0)
		{/* poll 调用失败 */
			printf("poll failure\n");
			break;
		}
		
		for (int i = 0; i < user_counter + 1; i++)
		{
			if (fds[i].fd == listenfd && fds[i].revents & POLLIN)		/* 如果当前是是监听 socket，且读事件被触发 */
			{
				struct sockaddr_in client_address;
				socklen_t client_addrlength = sizeof(client_address);
				int connfd = accept(fds[i].fd, (struct sockaddr*)&client_address, &client_addrlength);
				if (connfd < 0)		/* connfd 调用失败 */
				{
					printf("errno is: %d\n", errno);
					continue;
				}
				/* 当前用户数量已达上限，关闭新到的连接 */
				if (user_counter >= USER_LIMIT)
				{
					const char* info = "too many users\n";
					printf("%s", info);
					send(connfd, info, strlen(info), 0);	/* 通知刚来的客户：客户满了，一会再连接把 */
					close(connfd);				/* 别忘记 close ！！！ */
					continue;
				}
				/* 对于新的连接，同时修改 fds 和 users 数组。前文已经提到，users[connfd] 对应于新连接文件描述符 connfd 的客户数据 */
				user_counter++;
				users[connfd].address = client_address;
				setnonblocking(connfd);				/* 所有客户端都设置为非阻塞，这样就能同时监听多个客户端了，而不是在一个客户端上死等 */
				fds[user_counter].fd = connfd;
				fds[user_counter].events = POLLIN | POLLRDHUP | POLLERR;	/* 第一次把客户端添加进来时，三种事件都要监听 */
				fds[user_counter].revents = 0;
				printf("comes a new user, now have %d users\n", user_counter);
			}
			else if (fds[i].revents & POLLERR)	/* 如果当前 socket 出现错误时，获取并清除错误状态，然后进入下一次循环 */
			{
				printf("get an error from %d\n", fds[i].fd);
				char errors[100];
				memset(errors, '\0', 100);
				socklen_t length = sizeof(errors);
				if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &length) < 0)
				{
					printf("get socket option failed\n");
				}
				continue;
			}
			else if (fds[i].revents & POLLRDHUP)	/* 如果当前 socket 关闭连接，则服务器也关闭对应的连接，并将用户总数减1 */
			{
				users[fds[i].fd] = users[fds[user_counter].fd];		/* 覆盖掉当前 socket 对应的客户信息 */
				close(fds[i].fd);		/* 关闭当前客户对应的 socket */
				fds[i] = fds[user_counter];	/* 离开的客户位置存储最后面客户的 fd，这样才能让 user_counter 减 1 */
				i--;
				user_counter--;
				printf("a client left\n");
			}
			else if (fds[i].revents & POLLIN)	/* 当前的 socket 不是监听 socket，且可读事件被触发 */
			{
				int connfd = fds[i].fd;
				memset(users[connfd].buf, '\0', BUFFER_SIZE);
				ret = recv(connfd, users[connfd].buf, BUFFER_SIZE - 1, 0);
				if (ret < 0)			/* 读取失败 */
				{
					if (errno != EAGAIN)	/* 错误码不是 EAGAIN，妥妥的出现了错误，关闭连接 */
					{
						close(connfd);
						users[connfd] = users[fds[user_counter].fd];
						fds[i] = fds[user_counter];
						i--;
						user_counter--;
					}
				}
				else if (ret == 0)	/* 客户端关闭连接了，啥都不干，交给下一次循环里的 POLLRDHUP 判断处理，当然喽，也可以在这处理，但是没必要，代码要简洁 */
				{
				}
				else	/* 顺利接收到客户数据，则通知其他 socket 连接准备写数据 */
				{
					printf("from %d, get %dbytes of client data: "%s"\n", connfd, ret, users[connfd].buf);
					for (int j = 1; j <= user_counter; j++)	/* 既然服务器接收到了数据，那么就要向除发送数据客户端外的所有客户端转发该数据*/
					{					/* 这个循环就是在干这件事 */
						if (fds[j].fd == connfd)	/* 跳过发来数据的客户端 */
						{
							continue;
						}
						/* 对于其他客户端，就不再监视他们的可读事件了，优先监视他们的可写事件，所以要注销 POLLIN 事件 */
						fds[j].events |= ~POLLIN;
						fds[j].events |= POLLOUT;
						users[fds[j].fd].write_buf = users[connfd].buf;
					}
				}
			}
			else if (fds[i].revents & POLLOUT)	/* 如果当前 socket 的可写事件被触发 */
			{
				int connfd = fds[i].fd;
				if (!users[connfd].write_buf)	/* 验证该 socket 确实有数据可写，也就是 write_buf 不为空 */
				{
					continue;
				}
				ret = send(connfd, users[connfd].write_buf, strlen(users[connfd].write_buf), 0);
				users[connfd].write_buf = NULL;		/* 写完了，那么当前 socket 的写入数据需要置为空 */
				/* 写完数据后需要重新注册 fds[i] 上的可读事件 */
				fds[i].events |= ~POLLOUT;
				fds[i].events |= POLLIN;
			}
		}
	}

	delete [] users;
	close(listenfd);
	return 0;
}

