/*
 * @Description: In User Settings Edit
 * @Author: your name
 * @Date: 2019-08-01 09:23:53
 * @LastEditTime: 2019-08-12 18:04:53
 * @LastEditors: Please set LastEditors
 */
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "can/socket_can.h"
//服务器端

void *fun_thrReceiveHandler(void *socketInfo);
void *fun_thrAcceptHandler(void *socketListen);
//1:是 0：否
int checkThrIsKill(pthread_t thr);
void businessHandle(unsigned char *cmd);

typedef struct MySocketInfo
{
    int socketCon;
    char *ipaddr;
    uint16_t port;
} _MySocketInfo;

// 客户端数组
struct MySocketInfo arrConSocket[10];
int conClientCount = 0;

// 接受客户端线程列表
pthread_t arrThrReceiveClient[10];
int thrReceiveClientCount = 0;
SocketCan Can;

int main()
{
    //初始化全局变量
    //memset(arrConSocket,0,sizeof(struct MySocketInfo)*10);

    /* 初始化CAN */
    if (socket_can_init(&Can, "can0") < 0)
    {
        printf("Can init fail\n");
        return -1;
    }

    printf("开始socket\n");
    /* 创建TCP连接的Socket套接字 */
    int socketListen = socket(AF_INET, SOCK_STREAM, 0);
    if (socketListen < 0)
    {
        printf("创建TCP套接字失败\n");
        exit(-1);
    }
    else
    {
        printf("创建套接字成功\n");
    }
    /* 设置套接字选项避免地址使用错误 */
    int on = 1;
    if ((setsockopt(socketListen, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0)
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    /* 填充服务器端口地址信息，以便下面使用此地址和端口监听 */
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* 这里地址使用全0，即所有 */
    server_addr.sin_port = htons(2000);
    if (bind(socketListen, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) != 0)
    {
        perror("绑定ip地址、端口号失败\n");
        exit(-1);
    }
    else
    {
        printf("绑定ip地址,端口号\n");
    }
    /* 开始监听相应的端口 */
    if (listen(socketListen, 10) != 0)
    {
        printf("开启监听失败\n");
        exit(-1);
    }
    else
    {
        printf("开启监听成功\n");
    }
    /* 接受连接套接字 */
    pthread_t thrAccept;
    pthread_create(&thrAccept, NULL, fun_thrAcceptHandler, &socketListen);

    /* 实时发送数据 */
    while (1)
    {
        //判断线程存活多少
        int i;
        for (i = 0; i < thrReceiveClientCount; i++)
        {
            if (checkThrIsKill(arrThrReceiveClient[i]) == 1)
            {
                printf("有个线程被杀了\n");
                thrReceiveClientCount--;
            }
        }
        printf("当前有接受数据线程多少个：%d\n", thrReceiveClientCount);

        // 可以录入用户操作选项，并进行相应操作
        char userStr[30] = {'0'};
        scanf("%s", userStr);
        if (strcmp(userStr, "q") == 0)
        {
            printf("用户选择退出！\n");
            break;
        }
        // 发送消息
        if (conClientCount <= 0)
        {
            printf("没有客户端连接\n");
        }
        else
        {
            int i;
            for (i = 0; i < conClientCount; i++)
            {
                //int sendMsg_len = send(arrConSocket[i].socketCon, userStr, 30, 0);
                int sendMsg_len = write(arrConSocket[i].socketCon, userStr, 30);
                if (sendMsg_len > 0)
                {
                    printf("向%s:%d发送成功\n", arrConSocket[i].ipaddr, arrConSocket[i].port);
                }
                else
                {
                    printf("向%s:%d发送失败\n", arrConSocket[i].ipaddr, arrConSocket[i].port);
                }
            }
        }

        sleep(0.5);
    }

    socket_can_close(&Can);
    // 等待子进程退出
    printf("等待子线程退出，即将退出！\n");
    char *message;
    pthread_join(thrAccept, (void *)&message);
    printf("%s\n", message);

    return 0;
}

void *fun_thrAcceptHandler(void *socketListen)
{
    while (1)
    {
        int sockaddr_in_size = sizeof(struct sockaddr_in);
        struct sockaddr_in client_addr;
        int _socketListen = *((int *)socketListen);
        int socketCon = accept(_socketListen, (struct sockaddr *)(&client_addr), (socklen_t *)(&sockaddr_in_size));
        if (socketCon < 0)
        {
            printf("连接失败\n");
        }
        else
        {
            printf("连接成功 ip: %s:%d\r\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
        }
        printf("连接套接字为：%d\n", socketCon);
        //开启新的通讯线程，负责同连接上来的客户端进行通讯
        _MySocketInfo socketInfo;
        socketInfo.socketCon = socketCon;
        socketInfo.ipaddr = inet_ntoa(client_addr.sin_addr);
        socketInfo.port = client_addr.sin_port;
        arrConSocket[conClientCount] = socketInfo;
        conClientCount++;
        printf("连接了%d个用户\n", conClientCount);

        pthread_t thrReceive = 0;
        pthread_create(&thrReceive, NULL, fun_thrReceiveHandler, &socketInfo);
        arrThrReceiveClient[thrReceiveClientCount] = thrReceive;
        thrReceiveClientCount++;

        //让进程休息1秒
        sleep(0.5);
    }

    char *s = "安全退出接受进程";
    pthread_exit(s);
}

/*
    线程处理 - 数据接受
    测试阶段注释打印语句
 */
void *fun_thrReceiveHandler(void *socketInfo)
{
    char buffer[30];
    int buffer_length;
    _MySocketInfo _socketInfo = *((_MySocketInfo *)socketInfo);
    unsigned int _count = 0;
    while (1)
    {
        //添加对buffer清零
        bzero(&buffer, sizeof(buffer));

        buffer_length = read(_socketInfo.socketCon, buffer, 30);
        if (buffer_length == 0)
        {
            printf("%s:%d 客户端关闭\n", _socketInfo.ipaddr, _socketInfo.port);
            conClientCount--;
            break;
        }
        else if (buffer_length < 0)
        {
            printf("接受客户端数据失败\n");
            break;
        }
        buffer[buffer_length] = '\0';
        printf("%s:%d [%d]说：%s\n", _socketInfo.ipaddr, _socketInfo.port, _count++, buffer);
        businessHandle((unsigned char *)buffer);
        sleep(0.2);
    }
    printf("接受数据线程结束了\n");
    return NULL;
}

int checkThrIsKill(pthread_t thr)
{
    int res = 1;
    int res_kill = pthread_kill(thr, 0);
    if (res_kill == 0)
    {
        res = 0;
    }
    return res;
}

void businessHandle(unsigned char *cmd)
{
    unsigned _cmd_frist = *cmd;
    switch (_cmd_frist)
    {
    case 's':
    {
        //向can发送数据
        struct can_frame frame;
        frame.can_id = 0x18000040 | CAN_EFF_FLAG;
        frame.can_dlc = 1;
        frame.data[0] = 0xAB;
        socket_can_send_frame(&Can, frame);
        break;
    }
    default:
        break;
    }
}