#pragma once

#include <endian.h>
#include <buffer.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>

class TcpConn {
public:
    TcpConn(int sockfd) : sockfd(sockfd) {

    }
    ~TcpConn() {
    }
    
    int getFd() {
        return sockfd;
    }
    ssize_t send(char* buf, size_t len) {
        return write(this->sockfd, buf, len);
    }
    ssize_t send(Buffer& buffer) {
        return write(this->sockfd, buffer.data(), buffer.getLength());
    }
    ssize_t read(Buffer& buffer) {
        return ::read(sockfd, buffer.data(), buffer.getLength());
    }
    void close() {
        ::close(sockfd);
    }

    // this only works if the whole pattern is in one tmp !! TODO: fix.
    Buffer readUntil(std::string pattern) {
        Buffer buf(0);
        while (true) {
            Buffer tmp(1024);
            ssize_t rc = read(tmp);
            if (rc <= 0) break;

            std::string test((char*)tmp.data(), tmp.getLength());
            size_t found = test.find(pattern);
            if (found != std::string::npos) {
                tmp.setCapacity(found + pattern.length());
                buf.append(tmp);
                break;
            }
            buf.append(tmp);
        }
        return buf;
    }
private:
    int sockfd;
};

class TcpListener {
public:
    TcpListener(uint16_t port) : port(port) {
        listenfd = socket(AF_INET, SOCK_STREAM, 0);
        int opt_val = 1;
	    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt_val, sizeof(opt_val));
	    listenaddr.sin_family = AF_INET;
	    listenaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        listenaddr.sin_port = htons(port);
        bind(listenfd, (const sockaddr*)&listenaddr, sizeof(listenaddr));
    }
    ~TcpListener() {
        close(listenfd);
    }

    TcpConn accept() {
        listen(listenfd, 128);
        socklen_t client_len = sizeof(listenaddr);
        int connfd = ::accept(listenfd, (struct sockaddr *)&listenaddr, &client_len);
        return TcpConn(connfd);
    }

private:
    uint16_t port;
    int listenfd;
    struct sockaddr_in listenaddr;
};