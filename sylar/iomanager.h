#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__

#include "scheduler.h"
#include "timer.h"

namespace sylar {

class IOManager : public Scheduler , public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    enum Event {// 和epoll事件对应
        NONE    = 0x0,
        READ    = 0x1, //EPOLLIN
        WRITE   = 0x4, //EPOLLOUT
    };
private:
    struct FdContext { // 每个fd对应一个FdContext
        typedef Mutex MutexType;// 不用读写锁，因为每个fd的操作不会太频繁
        struct EventContext {
            Scheduler* scheduler = nullptr; //事件执行的scheduler
            Fiber::ptr fiber;               //事件协程
            std::function<void()> cb;       //事件的回调函数
        };

        EventContext& getContext(Event event);
        void resetContext(EventContext& ctx);
        void triggerEvent(Event event);

        EventContext read;      //读事件
        EventContext write;     //写事件
        int fd = 0;             //事件关联的句柄
        Event events = NONE;    //已经注册的事件
        MutexType mutex;
    };

public:
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    ~IOManager();

    //0 success, -1 error。 例如，fd=5， event=READ，则表示给fd=5注册读事件，cb是读事件发生时的回调函数
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);
    bool cancelEvent(int fd, Event event);

    bool cancelAll(int fd);

    static IOManager* GetThis();

protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;
    void onTimerInsertedAtFront() override;// 当有新的定时器插入到最前面时触发

    void contextResize(size_t size);// 如果不预先分配，注册事件时需要频繁的加锁和扩容vector
    bool stopping(uint64_t& timeout);
private:
    int m_epfd = 0;// epoll的文件描述符
    int m_tickleFds[2];

    std::atomic<size_t> m_pendingEventCount = {0};// 待处理的事件数量
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;// 每个fd对应一个FdContext,对读写事件的封装
};

}

#endif