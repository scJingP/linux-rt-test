/*
 * @Description: Socket Can 操作接口
 * @Author: pengjing
 * @Date: 2019-08-01 09:38:46
 * @LastEditTime: 2019-08-01 11:33:42
 * @LastEditors: Please set LastEditors
 */
#include "socket_can.h"

/**
 * @description: 
 * @param {PSocketCan can} SOCKET_CAN指针数据, 外部定义 SOCKET_CAN结构体变量, 传入内部进行赋值
 * @param {uchar* socket_can_name} ifconfig -a 中的 can设备名称
 * @return: 结果状态, < 0 失败, == 0 成功
 */
uchar socket_can_init(PSocketCan can, uchar *socket_can_name)
{
    if ((can->socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
    {
        perror("Create socket failed");
        return -1;
    }

    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_filter rfilter[1];

    /* set up can interface */
    strcpy(ifr.ifr_name, socket_can_name);
    /* assign can device */
    ioctl(can->socket, SIOCGIFINDEX, &ifr);
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    /* bind can device */
    if (bind(can->socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Bind can device failed\n");
        close(can->socket);
        return -2;
    }

    /* set filter for only receiving packet with can id 0x1F */
    rfilter[0].can_id = 0x1F;
    rfilter[0].can_mask = CAN_SFF_MASK;
    if (setsockopt(can->socket, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter)) < 0)
    {
        perror("set receiving filter error\n");
        close(can->socket);
        return -3;
    }
    return 0;
}

uchar socket_can_send(PSocketCan can, const uint frame_id, const uchar *send_data, uint send_data_num, bool is_extern)
{
    struct can_frame frame;
    frame.can_id = frame_id;
    if (is_extern == 1)
    {
        frame.can_id &= 0x1FFFFFFF;   //对前三位清零
        frame.can_id |= CAN_EFF_FLAG; //设置扩展帧
    }
    frame.can_dlc = send_data_num;
    printf("[socket_can_send] ID=%#x; data length=%d\n", frame.can_id, frame.can_dlc);
    /* prepare data for sending: 0x11,0x22...0x88 */
    int i = 0;
    for (i = 0; i < 8; i++)
    {
        frame.data[i] = send_data[i];
        printf("[socket_can_send] %#x", frame.data[i]);
    }

    /* Sending data */
    if (write(can->socket, &frame, sizeof(frame)) < 0)
    {
        perror("[socket_can_send] send failed");
        return -1;
    }
    return 0;
}

uchar socket_can_send_frame(PSocketCan can, struct can_frame frame)
{
    /* Sending data */
    if (write(can->socket, &frame, sizeof(frame)) < 0)
    {
        perror("[socket_can_send] send failed");
        return -1;
    }
    return 0;
}

uchar socket_can_recv(PSocketCan can, uint *frame_id, uchar *recv_data, uint *recv_frame_num, bool *is_extern)
{
    struct can_frame frame;
    if (read(can->socket, &frame, sizeof(frame)) > 0)
    {
        printf("[socket_can_recv] ID=%#x; data length=%d\n", frame.can_id, frame.can_dlc);
        *frame_id = frame.can_id;
        *recv_frame_num = frame.can_dlc;
        *is_extern = 0;
        if (frame.can_id & 0xE0000000 == CAN_EFF_FLAG)
        {
            *is_extern = 1;
        }
        int i = 0;
        for (i = 0; i < frame.can_dlc; i++)
        {
            recv_data[i] = frame.data[i];
        }
    }
}

uchar socket_can_close(PSocketCan can)
{
    if(can->socket != 0){
        close(can->socket);
    }
}