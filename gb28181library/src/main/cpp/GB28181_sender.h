//
// Created by autulin on 2018/9/13.
//

#ifndef GB28181ANDROID_GB28181_SENDER_H
#define GB28181ANDROID_GB28181_SENDER_H

#include <fstream>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/endian.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "base_include.h"

#include "user_arguments.h"

#define RUNING 1
#define STOPED 0

class GB28181_sender {
public:
    GB28181_sender(UserArguments *arg);

    static void* processSend(void *obj);

    int initSender();

    int addPkt(uint8_t *pkt);

    int sendCloseSignal();

    int closeSender();
private:
    UserArguments * args;
    ofstream fout;
    threadsafe_queue<uint8_t *> pkt_queue;
    volatile int isRuning;
    volatile int stoped;

    int sockfd;
    struct sockaddr_in serveraddr;
    int serverlen;

    int initSocket(char* hostname, int port, int localPort);
    ssize_t sendData(uint8_t * buf, int len);
    int closeSocket();


};


#endif //GB28181ANDROID_GB28181_SENDER_H
