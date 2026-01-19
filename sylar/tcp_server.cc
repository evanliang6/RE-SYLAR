#include "tcp_server.h"
#include"config.h"
#include"log.h"

namespace sylar{
static ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
    Config::Lookup<uint64_t>("tcp.server.read_timeout"
                ,(uint64_t)(60 * 1000 * 2)
                ,"tcp server read timeout");

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

TcpServer::TcpServer(sylar::IOManager* worker, sylar::IOManager* accept_worker)
:m_worker(worker)
,m_acceptWorker(accept_worker)
,m_recvTimeout(g_tcp_server_read_timeout->getValue())
,m_name("sylar/1.0.0")
,m_isStop(true){    
}

TcpServer::~TcpServer(){
    for(auto & i : m_socks){
        i->close();
    }
    m_socks.clear();
}

bool TcpServer::bind(sylar::Address::ptr addr){
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs,fails);
}
bool TcpServer::bind(const std::vector<Address::ptr>& addrs,std::vector<Address::ptr>& fails){
    for(auto & addr: addrs){
        Socket::ptr sock=Socket::CreateTCP(addr);
        if(!sock->bind(addr)){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"bind fail errno="
            <<errno<<" errstr="<<strerror(errno)
            <<" addr="<<addr->toString();
            fails.push_back(addr);
            continue;
        }
        if(!sock->listen()){// 监听失败
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"listen fail errno="
            <<errno<<" errstr="<<strerror(errno)
            <<" addr="<<addr->toString();
            fails.push_back(addr);
            continue;
        }
        m_socks.push_back(sock);
    }
    if(!fails.empty()){
        m_socks.clear();
        return false;
    }
    for(auto & i : m_socks){
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"bind success "
        <<*i;
    }
    return true;
}

void TcpServer::startAccept(Socket::ptr sock){
    while(!m_isStop){
        Socket::ptr client = sock->accept();
        if(client){
            client->setRecvTimeout(m_recvTimeout);
            m_worker->schedule(std::bind(&TcpServer::handleClient,
                shared_from_this(),client));
        }else{
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"accept errno="
            <<errno<<" errstr="<<strerror(errno);
            continue;
        }
    }

}

bool TcpServer::start(){
    if(!m_isStop){// 已经启动
        return true;
    }
    m_isStop=false;
    for(auto& sock:m_socks){
        m_acceptWorker->schedule(std::bind(&TcpServer::startAccept,
            shared_from_this(),sock));
    }
    return true;

}

void TcpServer::stop() {
    m_isStop = true;
    auto self = shared_from_this();
    m_acceptWorker->schedule([this, self]() {// 多捕获一个self是为了防止TcpServer对象提前析构
        for(auto& sock : m_socks) {
            sock->cancelAll();
            sock->close();
        }
        m_socks.clear();
    });
}

void TcpServer::handleClient(Socket::ptr client) {// 可以通过重载这个函数实现自定义的业务逻辑
    SYLAR_LOG_INFO(g_logger) << "handleClient: " << *client;
}

void handleClient2(Socket::ptr client) {// 这个handleclient可以发送回数据
    SYLAR_LOG_INFO(g_logger) << "handleClient: " << client;

    char buf[1024];

    while(true) {
        int rt = client->recv(buf, sizeof(buf));

        if(rt == 0) {
            SYLAR_LOG_INFO(g_logger) << "client close: " << client;
            break;
        }
        if(rt < 0) {
            SYLAR_LOG_ERROR(g_logger) << "client error: " << client;
            break;
        }

        client->send(buf, rt); // echo 回去
    }
}


}
