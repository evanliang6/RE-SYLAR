#include "sylar/socket.h"
#include "sylar/sylar.h"
#include "sylar/iomanager.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_socket() {
    sylar::IPAddress::ptr addr =
        sylar::Address::LookupAnyIPAddress("45.113.192.101");
    if(!addr) {
        SYLAR_LOG_ERROR(g_logger) << "get address fail";
        return;
    }

    addr->setPort(80);
    SYLAR_LOG_INFO(g_logger) << "addr=" << addr->toString();

    sylar::Socket::ptr sock = sylar::Socket::CreateTCP(addr);
    if(!sock->connect(addr,-1)) {
        SYLAR_LOG_ERROR(g_logger)
            << "connect " << addr->toString() << " fail";
        return;
    }

    SYLAR_LOG_INFO(g_logger)
        << "connect " << addr->toString() << " connected";

    const char req[] =
        "GET /get HTTP/1.1\r\n"
        "Host: www.baidu.com\r\n"
        "Connection: close\r\n"
        "\r\n";

    int rt = sock->send(req, sizeof(req) - 1);
    SYLAR_LOG_INFO(g_logger) << "send rt=" << rt;
    if(rt <= 0) return;

    std::string buf;
    buf.resize(4096);

    while((rt = sock->recv(&buf[0], buf.size())) > 0) {
        SYLAR_LOG_INFO(g_logger) << buf.substr(0, rt);
    }
}

int main(int argc, char** argv) {
    sylar::IOManager iom;
    iom.schedule(test_socket);
    return 0;
}
