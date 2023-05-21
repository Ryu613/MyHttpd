#include<stdio.h>
//����ͨ����Ҫ������ͷ�ļ���windows��
#include<WinSock2.h>
#pragma comment(lib, "WS2_32.lib")

void error_die(const char* str) {
    perror(str);
    exit(1);
}


int startup(unsigned short *port) {
    //windows��Ҫ��ʼ������
    WSADATA data;
    int ret = WSAStartup(
        MAKEWORD(1,1), // 1.1�汾��Э��
        &data
    );
    if (ret) { // ret != 0 ��ʼ��ʧ��
        error_die("WSAStartup init failed");
    }
    //�����׽���,������
    int server_socket = socket(PF_INET, // �׽��ֵ�����
        SOCK_STREAM, // ������
        IPPROTO_TCP  // Э��TCP
    );
    if (server_socket == -1) {
        //��ӡ������ʾ������������
        error_die("socket init fail");
    }
    //�����׽��ֵĶ˿ڿɸ����ԣ�����رպ��޷�ֱ�Ӹ��ô˶˿�
    int opt = 1;
    ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    if (ret == -1) {
        error_die("socket setup failed");
    }

    //���÷������������ַ
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = PF_INET; //�����ַ����
    server_addr.sin_port = htons(*port); //���紫����ֽ�����������п��ܲ�ͬ����Ҫת��
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // �����IP��ַ��0�Ƕ�����
    //���׽���
    ret = bind(server_socket, (struct sockaddr*)& server_addr, sizeof(server_addr));
    if (ret < 0) {
        error_die("bind port failed");
    }
    //��̬����һ���˿�
    int nameLen = sizeof(server_addr);
    if (*port == 0) {
        ret = getsockname(server_socket, (struct sockaddr*)&server_addr, &nameLen); //��ȡʵ�ʿ��õĶ˿ںţ��ŵ�server_addr��
        if (ret < 0) {
            error_die("getsockname error");
        }
        *port = server_addr.sin_port;
    }

    //������������(ͬʱ����ʱ����Ҫ�ŶӴ���)
    ret = listen(server_socket, 5);
    if (ret < 0) {
        error_die("listen queue init failed");
    }

    return server_socket;
}

//�����û�������̺߳���
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

    //��while������ʽ�ȴ��ͻ��˷���
    while (1) {
        //�µĿͻ����׽��ָ���ר�ŽӴ��ÿ�
        int client_sock = accept(server_socket, 
            (struct sockaddr*) & client_addr, //�ͻ��˵ĵ�ַ
            &client_addr_len
        );
        if (client_sock == -1) {
            error_die("client_sock accept failed");
        }
        //ʹ���µ��׽��ֶԷÿ�������д���
        //�����µ��̣߳�����ÿ�����(ʹ��windows��ʽ�����߳�)
        DWORD threadId = 0;
        CreateThread(0, 0, 
            accept_request,
            (void*)client_sock,
            0, &threadId);

    }
    closesocket(server_socket);
    return 0;
}