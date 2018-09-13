//
// Created by autulin on 2018/9/13.
//

#ifndef GB28181ANDROID_GB28181_SENDER_H
#define GB28181ANDROID_GB28181_SENDER_H

#include <fstream>
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
    int isRuning;
    volatile int stoped;


};


#endif //GB28181ANDROID_GB28181_SENDER_H
