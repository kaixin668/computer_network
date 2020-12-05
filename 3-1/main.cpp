#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <fstream>
#include <vector>
#include<time.h>

using namespace std;

using namespace std;
const int Mlenx = 509;
const char ACK = 0x03;
const char NAK = 0x07;
const char LAST_PACK = 0x18;
const char NOTLAST_PACK = 0x08;
const char SHAKE_1 = 0x01;
const char SHAKE_2 = 0x02;
const char SHAKE_3 = 0x04;
const char WAVE_1 = 0x80;
const char WAVE_2 = 0x40;
const int TIMEOUT = 500;//毫秒
char buffer[2000000];
int len;

SOCKADDR_IN serverAddr, clientAddr;
SOCKET server = socket(AF_INET, SOCK_DGRAM, 0); //选择udp协议


char sum_cal(char *arr, int lent) {
    if (lent == 0)
        return ~(0);
    char ret = arr[0];

    for (int i = 1; i < lent; i++) {
        ret = arr[i] + (char) ((int(arr[i]) + ret) % ((1 << 8) - 1));
    }
    return ~ret;
}

void wait_shake_hand() {
    while (1) {
        char recv[2];
        int begintime = clock();
        int lentmp = sizeof(clientAddr);
        while (recvfrom(server, recv, 2, 0, (sockaddr *) &clientAddr, &lentmp) == SOCKET_ERROR);
        if (sum_cal(recv, 2) != 0 || recv[1] != SHAKE_1)
            continue;
        while (1) {
            recv[1] = SHAKE_2;
            recv[0] = sum_cal(recv + 1, 1);
            sendto(server, recv, 2, 0, (sockaddr *) &clientAddr, sizeof(clientAddr));
            while (recvfrom(server, recv, 2, 0, (sockaddr *) &clientAddr, &lentmp) == SOCKET_ERROR);
            if (sum_cal(recv, 2) == 0 && recv[1] == SHAKE_1)
                continue;
            if (sum_cal(recv, 2) == 0 || recv[1] == SHAKE_3)
                break;
        }
        break;
    }
}

void recv_message(char *message, int &len_recv) {
    char recv[Mlenx + 4];
    int lentmp = sizeof(clientAddr);
    static int last_order = -1;
    len_recv = -1;
    while (1) {
        while (1) {
            while (recvfrom(server, recv, Mlenx + 4, 0, (sockaddr *) &clientAddr, &lentmp) == SOCKET_ERROR);
            char send[3];
            if (sum_cal(recv, Mlenx + 4) == 0) {
                send[1] = ACK;
                send[2] = recv[2];
                send[0] = sum_cal(send + 1, 2);
                sendto(server, send, 3, 0, (sockaddr *) &clientAddr, sizeof(clientAddr));
                break;
            } else {
                send[1] = NAK;
                send[2] = recv[2];//NAK序号可能不准确
                send[0] = sum_cal(send + 1, 2);
                sendto(server, send, 3, 0, (sockaddr *) &clientAddr, sizeof(clientAddr));
                continue;
            }
        }
        if (last_order == recv[2])
            continue; //和上一个数据包序号相同，丢弃
        if (LAST_PACK == recv[1]) {
            for (int i = 4; i < recv[3] + 4; i++)
                message[++len_recv] = recv[i];
            break;
        } else {
            for (int i = 3; i < Mlenx + 3; i++)
                message[++len_recv] = recv[i];
        }
    }
}

int main() {
    WSADATA wsadata;
    int nError = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (nError) {
        printf("start error\n");
        return 0;
    }

    int Port = 11451;

    serverAddr.sin_family = AF_INET; //使用ipv4
    serverAddr.sin_port = htons(Port); //端口
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);


    if (server == INVALID_SOCKET) {
        printf("create fail");
        closesocket(server);
        return 0;
    }
    int retVAL = bind(server, (sockaddr *) (&serverAddr), sizeof(serverAddr));
    if (retVAL == SOCKET_ERROR) {
        printf("bind fail");
        closesocket(server);
        WSACleanup();
        return 0;
    }

    //等待接入
    printf("等待接入");
    wait_shake_hand();
    printf("用户已接入。\n正在接收数据...\n");
    recv_message(buffer, len);
    printf("第一条信息接收成功。")

    return 0;
}