#include "server.h"
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>

int initListenFd(unsigned short port){
	/* 创建监听的fd */
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1) {
		perror("socket");
		return -1;
	}

	/* 设置端口复用 */
	int opt = 1;
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
	if (ret == -1) {
		perror("setsockopt");
		return -1;
	}

	/* 绑定ip和端口 */
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1) {
		perror("bind");
		return -1;
	}

	/* 设置监听 */
	ret = listen(lfd, 128);
	if (ret == -1) {
		perror("listen");
		return -1;
	}

	/* 返回fd */
	return lfd;
}

int epollRun(int lfd){
	/* 创建epoll实例 */
	int epfd = epoll_create(1);
	if (epfd == -1) {
		perror("epoll_create");
		return -1;
	}

	/* 把监听文件描述符挂载epoll处理 */
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = lfd;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
	if (ret == -1) {
		perror("epoll_ctl");
		return -1;
	}

	/* 检测 */
	struct epoll_event evs[1024];
	int size = sizeof(evs) / sizeof(struct epoll_event);
	while (1) {
		int num = epoll_wait(epfd, evs, size, -1);
		for (int i = 0; i < num; i++) {
			int fd = evs[i].data.fd;
			if (fd == lfd) {
				/* 该文件描述符为监听，需要建立新连接 */
				acceptClient(lfd, epfd);
			}
			else {
				/* 该文件描述符为读写，需要接收数据 */
			}
		}
	}

	return 0;
}

int acceptClient(int lfd, int epfd){
	/* 和客户端建立连接 */
	struct sockaddr_in caddr;
	int cfd = accept(lfd, (struct sockaddr*)&caddr, sizeof(caddr));
	if (cfd == -1) {
		perror("accept");
		return -1;
	}
	char ip[32];
	printf("客户端建立连接！IP：%s, 端口：%d\n",
		inet_ntop(AF_INET, &caddr.sin_addr.s_addr, ip, sizeof(ip)),
		ntohs(caddr.sin_port));

	/* 设置非阻塞 */
	int flag = fcntl(cfd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(cfd, F_SETFL);

	/* cfd添加到epoll管理 */
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = cfd;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
	if (ret == -1) {
		perror("epoll_ctl");
		return -1;
	}
	return 0;
}
