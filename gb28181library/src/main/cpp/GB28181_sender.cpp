//
// Created by autulin on 2018/9/13.
//

#include "GB28181_sender.h"

GB28181_sender::GB28181_sender(UserArguments *arg) : args(arg) {}

int GB28181_sender::initSender() {
    switch (args->outType) {
        case 0: // udp
            LOGE("ip:%s, port:%d", args->ip_addr, args->port);
            break;
        case 1: // tcp
            LOGE("ip:%s, port:%d", args->ip_addr, args->port);
            break;
        case 2: // file
            //打开ps文件
            LOGE("media path: %s", args->media_path);
            fout.open(args->media_path, ios::binary);
            break;
        default:
            break;
    }

    isRuning = RUNING;
    pthread_t thread;
    pthread_create(&thread, NULL, GB28181_sender::processSend, this);
    LOGI("发送器初始化完成")
    return 0;
}

int GB28181_sender::addPkt(uint8_t *pkt) {
    pkt_queue.push(pkt);
    return 0;
}

void *GB28181_sender::processSend(void *obj) {
    GB28181_sender * gb28181Sender = (GB28181_sender *) obj;
    while (gb28181Sender->isRuning) {
        uint8_t *pkt_buf = *gb28181Sender->pkt_queue.wait_and_pop().get();
        uint16_t len = bytes2short(pkt_buf);
        LOGE("get pkt len: %d", len);

        switch (gb28181Sender->args->outType) {
            case 0: // udp
                break;
            case 1: // tcp
                break;
            case 2: // file
                gb28181Sender->fout.write((const char *) (pkt_buf + 2), len);
                break;
            default:
                break;
        }

        delete(pkt_buf);
    }
    LOGI("发送结束");
    gb28181Sender->closeSender();
    return nullptr;
}

int GB28181_sender::sendCloseSignal() {
    isRuning = STOPED;
    return 0;
}

int GB28181_sender::closeSender() {
    if (stoped) {
        return 0;
    } else {
        stoped = 1;
    }
    switch (args->outType) {
        case 0: // udp
            break;
        case 1: // tcp
            break;
        case 2: // file
            fout.close();
            break;
        default:
            break;
    }
    return 0;
}

