//
// Created by autulin on 2018/9/13.
//

#include <netdb.h>
#include "GB28181_sender.h"

GB28181_sender::GB28181_sender(UserArguments *arg) : args(arg) {}

int GB28181_sender::initSender() {
    switch (args->outType) {
        case 0: // udp
            LOGE("ip:%s, port:%d, out_type:%d", args->ip_addr, args->port, args->outType);
            initSocket(args->ip_addr, args->port);
            break;
        case 1: // tcp
            LOGE("ip:%s, port:%d, out_type:%d", args->ip_addr, args->port, args->outType);
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
        uint64_t start_t = getCurrentTime();

        uint8_t *pkt_buf = *gb28181Sender->pkt_queue.wait_and_pop().get();
        uint16_t len = bytes2short(pkt_buf);

        uint64_t t1 = getCurrentTime();
//        char strBuf[16];
//        sprintf(strBuf, "get pkt len: %d", len);
        int n;
        switch (gb28181Sender->args->outType) {
            case 0: // udp
                n = gb28181Sender->sendData(pkt_buf + 2, len);
                LOGE("[sender]get pkt len: %d. sent %d. (queue left size: %d)", len, n, gb28181Sender->pkt_queue.size());
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
        uint64_t t2 = getCurrentTime();
        LOGI("[sender]队列获取用时：%lld\t发送用时：%lld", t1 - start_t, t2 - t1);
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
    int n = 0;
    switch (args->outType) {
        case 0: // udp
            n = closeSocket();
            break;
        case 1: // tcp
            n = closeSocket();
            break;
        case 2: // file
            fout.close();
            break;
        default:
            break;
    }
    if (n < 0) {
        LOGE("close socket error");
    }
    return n;
}

int GB28181_sender::initSocket(char *hostname, int port) {
    //todo
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0){
        LOGE("ERROR opening socket");
        return sockfd;
    }

//    struct hostent *server;
//    server = gethostbyname(hostname);
//    if (server == NULL) {
//        LOGE("ERROR, no such host as %s\n", hostname);
//        return -1;
//    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
//    bcopy((char *)server->h_addr,
//          (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    inet_aton(hostname, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(port);
    serverlen = sizeof(serveraddr);

    return 0;
}

int GB28181_sender::sendData(uint8_t *buf, int len) {
    int n = sendto(sockfd, buf, len, 0, (const sockaddr *) &serveraddr, serverlen);
    return n;
}

int GB28181_sender::closeSocket() {
    close(sockfd);
    return 0;
}

