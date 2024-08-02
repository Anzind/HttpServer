#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#include "server.h"

int main(){
    /* 初始化用于监听的套接字 */
    int lfd = initListenFd(9999);

    /* 启动服务器程序 */


    return 0;
}