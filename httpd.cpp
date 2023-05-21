#include<stdio.h>
//网络通信需要包含的头文件（windows）
#include<WinSock2.h>
#pragma comment(lib, "WS2_32.lib")

void error_die(const char* str) {
    perror(str);
    exit(1);
}


int startup(unsigned short *port) {
    //windows需要初始化网络
    WSADATA data;
    int ret = WSAStartup(
        MAKEWORD(1,1), // 1.1版本的协议
        &data
    );
    if (ret) { // ret != 0 初始化失败
        error_die("WSAStartup init failed");
    }
    //创建套接字,并返回
    int server_socket = socket(PF_INET, // 套接字的类型
        SOCK_STREAM, // 数据流
        IPPROTO_TCP  // 协议TCP
    );
    if (server_socket == -1) {
        //打印错误提示，并结束程序
        error_die("socket init fail");
    }
    //设置套接字的端口可复用性，避免关闭后无法直接复用此端口
    int opt = 1;
    ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    if (ret == -1) {
        error_die("socket setup failed");
    }

    //配置服务器的网络地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = PF_INET; //网络地址类型
    server_addr.sin_port = htons(*port); //网络传输的字节序跟跟本地有可能不同，需要转换
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 允许的IP地址，0是都允许
    //绑定套接字
    ret = bind(server_socket, (struct sockaddr*)& server_addr, sizeof(server_addr));
    if (ret < 0) {
        error_die("bind port failed");
    }
    //动态分配一个端口
    int nameLen = sizeof(server_addr);
    if (*port == 0) {
        ret = getsockname(server_socket, (struct sockaddr*)&server_addr, &nameLen); //获取实际可用的端口号，放到server_addr中
        if (ret < 0) {
            error_die("getsockname error");
        }
        *port = server_addr.sin_port;
    }

    //创建监听队列(同时访问时，需要排队处理)
    ret = listen(server_socket, 5);
    if (ret < 0) {
        error_die("listen queue init failed");
    }

    return server_socket;
}

//处理用户请求的线程函数
DWORD WINAPI accept_request(LPVOID arg) {
    return 0;
}

int main(void) {
    //0-65535
    unsigned short port = 0;
    int server_socket = startup(&port);
    printf("httpd server is started, listening port: %d...", port);
    
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    //用while，阻塞式等待客户端访问
    while (1) {
        //新的客户端套接字负责专门接待访客
        int client_sock = accept(server_socket, 
            (struct sockaddr*) & client_addr, //客户端的地址
            &client_addr_len
        );
        if (client_sock == -1) {
            error_die("client_sock accept failed");
        }
        //使用新的套接字对访客请求进行处理
        //创建新的线程，处理访客请求(使用windows方式创建线程)
        DWORD threadId = 0;
        CreateThread(0, 0, 
            accept_request,
            (void*)client_sock,
            0, &threadId);

    }
    closesocket(server_socket);
    return 0;
}