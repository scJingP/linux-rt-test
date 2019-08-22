/*
 * @Description: Socket Can 操作接口
 * @Author: pengjing
 * @Date: 2019-08-01 10:47:38
 * @LastEditTime: 2019-08-01 11:33:32
 * @LastEditors: Please set LastEditors
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned char bool;
typedef struct
{
    int socket;
} SocketCan, *PSocketCan;

uchar socket_can_init(PSocketCan can, uchar *socket_can_name);
uchar socket_can_send(PSocketCan can, const uint frame_id, const uchar *send_data, uint send_data_num, bool is_extern);
uchar socket_can_send_frame(PSocketCan can, struct can_frame frame);
uchar socket_can_recv(PSocketCan can, uint *frame_id, uchar *recv_data, uint *recv_frame_num, bool *is_extern);
uchar socket_can_close(PSocketCan can);