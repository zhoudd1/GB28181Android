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
    LOGI("[sender]init ok.")
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
        if (!gb28181Sender->isRuning) break;
        uint16_t len = bytes2short(pkt_buf);

        uint64_t t1 = getCurrentTime();
        ssize_t n;
        switch (gb28181Sender->args->outType) {
            case 0: // udp
                n = gb28181Sender->sendData(pkt_buf + 2, len);
                LOGI("[sender][udp]get pkt len: %d. sent %ld. (queue left size: %d)", len, n, gb28181Sender->pkt_queue.size());
                break;
            case 1: // tcp
                n = gb28181Sender->sendData(pkt_buf, len + 2);
                LOGI("[sender][tcp]get pkt len: %d. sent %ld. (queue left size: %d)", len, n, gb28181Sender->pkt_queue.size());
                break;
            case 2: // file
                gb28181Sender->fout.write((const char *) (pkt_buf + 2), len);
                break;
            default:
                break;
        }

        delete(pkt_buf);
        uint64_t t2 = getCurrentTime();
        LOGI("[sender]read from queue：%lld\t I/O time：%lld", t1 - start_t, t2 - t1);
    }
    LOGI("[sender]sending thread is over.");
    gb28181Sender->closeSender();
    return nullptr;
}

int GB28181_sender::sendCloseSignal() {
    isRuning = STOPED;
    pkt_queue.push(NULL);
    return 0;
}

int GB28181_sender::closeSender() {
    LOGE("[sender]start close sender.")
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
        LOGE("close socket error.(%d)", n);
    }
    LOGE("[sender]close sender over.")
    return n;
}

int GB28181_sender::initSocket(char *hostname, int port) {
    switch (args->outType) {
        case 0: // udp
            sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            break;
        case 1: // tcp
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            break;
        default:
            sockfd = -99;
    }

    if (sockfd < 0){
        LOGE("ERROR opening socket.(%d)", sockfd);
        return sockfd;
    }

    // 域名解析相关
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

    if (args->outType == 1) { // tcp
        int ret = connect(sockfd, (const sockaddr *) &serveraddr, serverlen);
        if (ret < 0){
            LOGE("ERROR connect.(%d)", ret);
            return ret;
        }
    }

    return 0;
}

ssize_t GB28181_sender::sendData(uint8_t *buf, int len) {
    ssize_t n = 0;
    switch (args->outType) {
        case 0: // udp
            n = sendto(sockfd, buf, len, 0, (const sockaddr *) &serveraddr, serverlen);
            break;
        case 1: // tcp
            n = send(sockfd, buf, len, 0);
            break;
        default:
            return -1;
    }
    if (n < 0) {
        LOGE("send error.(%ld)", n);
    }
    return n;
}

int GB28181_sender::closeSocket() {
    close(sockfd);
    return 0;
}

