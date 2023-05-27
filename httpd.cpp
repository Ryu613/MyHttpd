#include<stdio.h>
//����ͨ����Ҫ������ͷ�ļ���windows��
#include<WinSock2.h>
#pragma comment(lib, "WS2_32.lib")

#define PRINTF(str)  printf("[%s - %d]"#str"=%s", __func__, __LINE__, str);

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

//��ȡ��ָ���Ŀͻ����׽��֣���ȡһ�����ݣ����浽buff��
//����ʵ�ʶ�ȡ�����ֽ���
int get_line(int sock, char* buff, int size) {
    char c = 0; //'\0'
    int i = 0;

    // \r\n
    while (i<size-1 && c != '\n') {
        int n = recv(sock, &c, 1, 0);
        if (n > 0) {
            //������\n�����
            if (c == '\r') {
                n = recv(sock, &c, 1, MSG_PEEK); // ���һ���ַ������������������ȡ����
                if (n > 0 && c == '\n') {
                    recv(sock, &c, 1, 0);
                }
                else {
                    c = '\n';
                }
            }
            buff[i++] = c;
        }
        else {
            c = '\n';
        }
    }
    buff[i] = 0; //'\0'
    return 0;
}

void unimplement(int client) {
    //��ָ�����׽��֣�����һ����ʾ����û��ʵ�ֵĴ���ҳ��
}

//�����û�������̺߳���
DWORD WINAPI accept_request(LPVOID arg) {
    char buff[1024]; //1K
    int client = (SOCKET)arg; //�ͻ����׽���
    
    //��ȡһ������
    // 0x015ffad8 "GET / HTTP/1.1\n"
    int numchars = get_line(client, buff, sizeof(buff));
    PRINTF(buff); // [accept_request-53]buff="GET ...."

    char method[255];
    int j = 0, i = 0;
    while (!isspace(buff[j]) && i < sizeof(method) - 1) {
        method[i++] = buff[j++];
    }
    method[i] = 0; // '\0'
    PRINTF(method);

    //�������ķ��������������Ƿ�֧��
    if (stricmp(method, "GET") && stricmp(method, "POST")) {
        //����������ش�����ʾҳ��
        unimplement(client);
        return 0; // �߳�ֹͣ����
    }

    //������Դ�ļ�·��
    //www.ryuom.com/abc/text.html
    // GET /abc/test.html HTTP/1.1\n
    char url[255]; //����������Դ������·��
    i = 0; 
    while (isspace(buff[j]) && j < sizeof(buff)) {
        j++;
    }

    while (!isspace(buff[j]) && i < sizeof(url) - 1 && j < sizeof(buff)) {
        url[i++] = buff[j++];
    }
    url[i] = 0;
    PRINTF(url)


    return 0;
}

int main(void) {
    //0-65535
    unsigned short port = 8095;
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

    // "/"��վ��������Դ�µ�index.html

    closesocket(server_socket);
    return 0;
}