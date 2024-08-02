#include "server.h"
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>

int initListenFd(unsigned short port){
	/* ����������fd */
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1) {
		perror("socket");
		return -1;
	}

	/* ���ö˿ڸ��� */
	int opt = 1;
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
	if (ret == -1) {
		perror("setsockopt");
		return -1;
	}

	/* ��ip�Ͷ˿� */
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1) {
		perror("bind");
		return -1;
	}

	/* ���ü��� */
	ret = listen(lfd, 128);
	if (ret == -1) {
		perror("listen");
		return -1;
	}

	/* ����fd */
	return lfd;
}

int epollRun(int lfd){
	/* ����epollʵ�� */
	int epfd = epoll_create(1);
	if (epfd == -1) {
		perror("epoll_create");
		return -1;
	}

	/* �Ѽ����ļ�����������epoll���� */
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = lfd;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
	if (ret == -1) {
		perror("epoll_ctl");
		return -1;
	}

	/* ��� */
	struct epoll_event evs[1024];
	int size = sizeof(evs) / sizeof(struct epoll_event);
	while (1) {
		int num = epoll_wait(epfd, evs, size, -1);
		for (int i = 0; i < num; i++) {
			int fd = evs[i].data.fd;
			if (fd == lfd) {
				/* ���ļ�������Ϊ��������Ҫ���������� */
				acceptClient(lfd, epfd);
			}
			else {
				/* ���ļ�������Ϊ��д����Ҫ�������� */
			}
		}
	}

	return 0;
}

int acceptClient(int lfd, int epfd){
	/* �Ϳͻ��˽������� */
	struct sockaddr_in caddr;
	int cfd = accept(lfd, (struct sockaddr*)&caddr, sizeof(caddr));
	if (cfd == -1) {
		perror("accept");
		return -1;
	}
	char ip[32];
	printf("�ͻ��˽������ӣ�IP��%s, �˿ڣ�%d\n",
		inet_ntop(AF_INET, &caddr.sin_addr.s_addr, ip, sizeof(ip)),
		ntohs(caddr.sin_port));

	/* ���÷����� */
	int flag = fcntl(cfd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(cfd, F_SETFL);

	/* cfd��ӵ�epoll���� */
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
