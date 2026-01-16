#include "sylar/address.h"
#include "sylar/log.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test() {
    std::vector<sylar::Address::ptr> addrs;

    SYLAR_LOG_INFO(g_logger) << "begin";
    bool v = sylar::Address::Lookup(addrs, "www.baidu.com",AF_INET,SOCK_STREAM,IPPROTO_TCP);
    
    SYLAR_LOG_INFO(g_logger) << "end";
    if(!v) {
        SYLAR_LOG_ERROR(g_logger) << "lookup fail";
        return;
    }

    for(size_t i = 0; i < addrs.size(); ++i) {
        SYLAR_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }
}

void test_iface() {
    std::multimap<std::string, std::pair<sylar::Address::ptr, uint32_t> > results;

    bool v = sylar::Address::GetInterfaceAddresses(results);
    if(!v) {
        SYLAR_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }

    for(auto& i: results) {
        SYLAR_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - "
            << i.second.second;
    }
}

void test_ipv4() {
    //auto addr = sylar::IPAddress::Create("www.sylar.top");
    auto addr = sylar::IPAddress::Create("127.0.0.8");
    if(addr) {
        SYLAR_LOG_INFO(g_logger) << addr->toString();
    }
}

void test_broadcast(){
    // 创建 IPv4 地址
    sylar::IPv4Address::ptr ip = sylar::IPv4Address::Create("192.168.1.10", 8080);
    if (!ip) {
        std::cout << "create ip failed" << std::endl;
        return;
    }

    uint32_t prefix = 24;

    // 计算三个地址
    sylar::IPAddress::ptr network   = ip->networdAddress(prefix);
    sylar::IPAddress::ptr broadcast = ip->broadcastAddress(prefix);
    sylar::IPAddress::ptr mask      = ip->subnetMask(prefix);

    std::cout << "IP        : " << ip->toString() << std::endl;
    std::cout << "Prefix    : /" << prefix << std::endl;

    std::cout << "Network   : " << network->toString() << std::endl;
    std::cout << "Broadcast : " << broadcast->toString() << std::endl;
    std::cout << "Subnet    : " << mask->toString() << std::endl;

    // 再测一个
    sylar::IPv4Address::ptr ip2 = sylar::IPv4Address::Create("10.2.3.4");
    prefix = 16;

    std::cout << "\n---- Another test ----" << std::endl;
    std::cout << "IP        : " << ip2->toString() << std::endl;
    std::cout << "Prefix    : /" << prefix << std::endl;
    std::cout << "Network   : " << ip2->networdAddress(prefix)->toString() << std::endl;
    std::cout << "Broadcast : " << ip2->broadcastAddress(prefix)->toString() << std::endl;
    std::cout << "Subnet    : " << ip2->subnetMask(prefix)->toString() << std::endl;
}

int main(int argc, char** argv) {
    //test_ipv4();
    //test_iface();
    test();
    //test_broadcast();
    return 0;
}
