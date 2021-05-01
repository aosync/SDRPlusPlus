#include "endian.hpp"

#include <thread>
#include <iostream>
#include <stdio.h>

#include "tcp_sockets.hpp"

#include "vfs.h"

void handleClient(TcpConn t) {
    while (true) {
        TcpBuffer req(4);

        if (t.read(req) <= 0) break;

        uint32_t size = Endian::LittleEndian::get<uint32_t>(&req);
        if (size > 256) break;
        req.setCapacity(size);

        req += 4;
        if (t.read(req) != req.getLength()) break;

        req.rewind();
        std::string request(&req, req.getLength());
        std::cout << request << std::endl;
    }
    t.close();
}

/*int main() {
    TcpListener tcpl(8080);
    while(true) {
        std::cout << "Accepting new clients." << std::endl;
        TcpConn t = tcpl.accept();
        std::thread(handleClient, t).detach();
    }
}*/

int main() {
    Stat s;
    s.type = 432;
    s.dev = 423;
    s.qid = Qid((uint8_t)(2 >> 24), 65, 423);
    s.mode = 4;
    s.atime = 123;
    s.mtime = 4;
    s.length = 32;
    s.name = "filename";
    s.uid = "owner";
    s.gid = "group";
    s.muid = "modifier";
    size_t bruh = s.size();
    char* buf = new char[bruh];
    s.put(buf);
    std::string t(buf, bruh);
    std::cout << t << std::endl;
    std::cout << bruh << std::endl;
    delete[] buf;
}