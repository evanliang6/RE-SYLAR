#ifndef __SYLAR_TCP_SERVER_H__
#define __SYLAR_TCP_SERVER_H__  

#include <memory>
#include <functional>
#include "address.h"
#include "iomanager.h"
#include "socket.h"
#include "noncopyable.h"

namespace sylar{

class TcpServer:public std::enable_shared_from_this<TcpServer>,Noncopyable{
    public:
    typedef std::shared_ptr<TcpServer> ptr;
    TcpServer(sylar::IOManager* worker = IOManager::GetThis(),
              sylar::IOManager* accept_worker = IOManager::GetThis());
    virtual ~TcpServer();
    virtual bool bind(sylar::Address::ptr addr);
    virtual bool bind(const std::vector<sylar::Address::ptr>& addrs,std::vector<sylar::Address::ptr>& fails);
    virtual bool start();
    virtual void stop();
    uint64_t getRecvTimeout() const { return m_recvTimeout;}
    void setRecvTimeout(uint64_t v) { m_recvTimeout = v;}
    std::string getName() const { return m_name;}
    void setName(const std::string& v) { m_name = v;}
    bool isStop() const { return m_isStop;}
    protected:
    virtual void handleClient(Socket::ptr client);// accept到一个客户端连接后会调用这个函数
    virtual void startAccept(Socket::ptr sock);

    private:
    IOManager* m_worker;
    IOManager* m_acceptWorker;
    std::vector<Socket::ptr> m_socks;
    uint64_t m_recvTimeout;
    std::string m_name;
    bool m_isStop;
};

}



#endif