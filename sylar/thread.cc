#include"thread.h"
#include<unistd.h>
#include"log.h"
#include"util.h"
namespace sylar {

Semaphore::Semaphore(uint32_t count){
    if(sem_init(&m_semaphore, 0, count)){
        throw std::logic_error("sem_init error");
    }
}
Semaphore::~Semaphore(){
    sem_destroy(&m_semaphore);
}

void Semaphore::wait(){
    while(true){
        if(!sem_wait(&m_semaphore)){
            return;
        }
        if(errno != EINTR){// EINTR表示信号中断
            throw std::logic_error("sem_wait error");
        }
    }
}
void Semaphore::notify(){
    if(sem_post(&m_semaphore)){
        throw std::logic_error("sem_post error");
    }

}


// 静态全局变量无法被其他文件访问.thread_local 修饰的变量是每个线程独有的变量
static thread_local Thread* t_thread = nullptr;// 保存当前线程指针
static thread_local std::string t_thread_name = "UNKNOW";// 保存当前线程名称

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
Thread* Thread::GetThis(){
    return t_thread;
}

const std::string& Thread::GetName() {
    return t_thread_name;
}

void Thread::SetName(const std::string& name) {
    if(name.empty()) {
        return;
    }
    if(t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string& name){
    if(name.empty()){
        m_name = "UNKNOW";
    }
    m_cb=cb;
    int rt= pthread_create(&m_thread, nullptr, &Thread::run, this);// 创建线程，返回句柄给m_thread。执行函数Thread::run，参数是this
    if(rt){
        SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt
            << " name=" << name;
        throw std::logic_error("pthread_create error");
    }
    m_semaphore.wait();// 等待线程创建成功
}

void* Thread::run(void* arg){
    Thread* thread = (Thread*)arg;
    t_thread = thread;// 设置当前线程指针
    thread->m_id = sylar::GetThreadId();// 设置线程id
    pthread_setname_np(pthread_self(), thread->m_name.substr(0,15).c_str());// 设置线程名称，linux限制16字节，包括结束符
    std::function<void()> cb;
    cb.swap(thread->m_cb);// 交换函数对象，避免拷贝
    thread->m_semaphore.notify();// 通知线程创建成功
    cb();// 执行线程函数
    return 0;
}

void Thread::join(){
    if(m_thread){
        int rt=pthread_join(m_thread, nullptr);// 等待线程执行完毕
        if(rt){
            SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;// 清空线程句柄
    }
}

Thread::~Thread(){
    if(m_thread){
        pthread_detach(m_thread);// 分离线程
    }

}

}
