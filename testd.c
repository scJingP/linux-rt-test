/*
 * @Description: In User Settings Edit
 * @Author: your name
 * @Date: 2019-08-01 09:23:53
 * @LastEditTime: 2019-08-21 10:06:48
 * @LastEditors: Please set LastEditors
 */
/* 设置亲和力1 */
#define _GNU_SOURCE

#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

/* 守护进程所需要的头文件 */
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>
#include <syslog.h>

/* 设置亲和力2 */
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

/* 设置实时进程优先级 */
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>

#include "can/socket_can.h"
//服务器端

void *fun_thrReceiveHandler(void *socketInfo);
void *fun_thrAcceptHandler(void *socketListen);
//1:是 0：否
int checkThrIsKill(pthread_t thr);
void businessHandle(unsigned char *cmd);
int daemon_init(void);
int setaffinityFor3Cpu(void);

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
    /* 设置进程为实时进程, 提高优先级 */
    pid_t pid = getpid();
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);   // 也可用SCHED_RR
    sched_setscheduler(pid, SCHED_RR, &param);                   // 设置当前进程

    setaffinityFor3Cpu();

    daemon_init();
    syslog(LOG_USER|LOG_INFO,"testd daemon start! \n");


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
        /*
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
         */

        // 可以录入用户操作选项，并进行相应操作

        sleep(1);
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
            //printf("%s:%d 客户端关闭\n", _socketInfo.ipaddr, _socketInfo.port);
            syslog(LOG_USER|LOG_INFO,"%s:%d 客户端关闭\n", _socketInfo.ipaddr, _socketInfo.port);
            conClientCount--;
            break;
        }
        else if (buffer_length < 0)
        {
            //printf("接受客户端数据失败\n");
            syslog(LOG_USER|LOG_INFO,"接受客户端数据失败\n");
            break;
        }
        buffer[buffer_length] = '\0';
        //printf("%s:%d [%d]说：%s\n", _socketInfo.ipaddr, _socketInfo.port, _count++, buffer);
        syslog(LOG_USER|LOG_INFO,"%s:%d [%d]说：%s\n", _socketInfo.ipaddr, _socketInfo.port, _count++, buffer);
        businessHandle((unsigned char *)buffer);
        sleep(0.2);
    }
    //printf("接受数据线程结束了\n");
    syslog(LOG_USER|LOG_INFO,"接受数据线程结束了\n");
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

int daemon_init(void) 
{ 
	int pid; 
	int i; 
 
	//忽略终端I/O信号，STOP信号
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGHUP,SIG_IGN);
	
	pid = fork();
	if(pid > 0) {
		exit(0); //结束父进程，使得子进程成为后台进程
	}
	else if(pid < 0) { 
		return -1;
	}
 
	//建立一个新的进程组,在这个新的进程组中,子进程成为这个进程组的首进程,以使该进程脱离所有终端
	setsid();
 
	//再次新建一个子进程，退出父进程，保证该进程不是进程组长，同时让该进程无法再打开一个新的终端
	pid=fork();
	if( pid > 0) {
		exit(0);
	}
	else if( pid< 0) {
		return -1;
	}
 
	//关闭所有从父进程继承的不再需要的文件描述符
	for(i=0;i< NOFILE;close(i++));
 
	//改变工作目录，使得进程不与任何文件系统联系
	chdir("/");
 
	//将文件当时创建屏蔽字设置为0
	umask(0);
 
	//忽略SIGCHLD信号
	signal(SIGCHLD,SIG_IGN); 
	
	return 0;
}

int setaffinityFor3Cpu(void){
    int cpus = 0;
    int  i = 0;
    cpu_set_t mask;
    cpu_set_t get;

    cpus = sysconf(_SC_NPROCESSORS_CONF);
    printf("cpus: %d\n", cpus);

    CPU_ZERO(&mask);    /* 初始化set集，将set置为空*/
    //CPU_SET(0, &mask);  /* 依次将0、1、2、3号cpu加入到集合，前提是你的机器是多核处理器*/
    //CPU_SET(1, &mask);
    //CPU_SET(2, &mask);
    CPU_SET(3, &mask);
    
    /*设置cpu 亲和性（affinity）*/
    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
        printf("Set CPU affinity failue, ERROR:%s\n", strerror(errno));
        return -1; 
    }
    usleep(1000); /* 让当前的设置有足够时间生效*/

    /*查看当前进程的cpu 亲和性*/
    CPU_ZERO(&get);
    if (sched_getaffinity(0, sizeof(get), &get) == -1) {
        printf("get CPU affinity failue, ERROR:%s\n", strerror(errno));
        return -1; 
    }   
    
    /*查看运行在当前进程的cpu*/
    for(i = 0; i < cpus; i++) {
        if (CPU_ISSET(i, &get)) { /*查看cpu i 是否在get 集合当中*/
            printf("this process %d of running processor: %d\n", getpid(), i); 
        }    
    }

    printf("setaffinityFor3Cpu end\n"); 
    return 0;
}