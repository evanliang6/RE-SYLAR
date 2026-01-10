#include"sylar/sylar.h"
sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

volatile int count=0;// 测试信号量用的全局变量

sylar::Mutex s_mutex;
void fun1(){
    SYLAR_LOG_INFO(g_logger) << "thread name=" << sylar::Thread::GetName()
        << " this.name:" << sylar::Thread::GetThis()->getName()
        <<"id=" << sylar::GetThreadId()
        <<"this.id"<< sylar::Thread::GetThis()->getId();
    for(int i=0;i<10000000;++i){
        sylar::Mutex::Lock lock(s_mutex);
        ++count;
        //sleep(1);
    }
    // 函数结束，lock析构函数自动解除s_mutex锁

}

void fun2(){
    while(true)
    SYLAR_LOG_INFO(g_logger) <<"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
}
void fun3(){
    while(true)
    SYLAR_LOG_INFO(g_logger) <<"==========================================";
}

int main(int argc,char** argv){
    std::vector<sylar::Thread::ptr> thrs;
    for(int i=0;i<5;++i){
        sylar::Thread::ptr thr(new sylar::Thread(&fun1,"name_"+std::to_string(i)));
        thrs.push_back(thr);
    }
    //sleep(20);
    time_t start = time(0);
    for(auto& i : thrs){
        i->join();
    }
    time_t end = time(0);
    SYLAR_LOG_INFO(g_logger) << "cost time=" << (end-start);
    SYLAR_LOG_INFO(g_logger) << "count=" << count;
    SYLAR_LOG_INFO(g_logger) << "thread test end";

}