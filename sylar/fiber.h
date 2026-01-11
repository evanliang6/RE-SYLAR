#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include<ucontext.h>
#include<functional>
#include<memory>
#include"thread.h" 
#include"macro.h"
#include"config.h"

namespace sylar {

class Fiber: public std::enable_shared_from_this<Fiber> {// 继承这个类可以使用自己的shared_ptr.但是不可以在栈上创建对象
public:
    typedef std::shared_ptr<Fiber> ptr;
    enum State{
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCEPT
    };
    Fiber(std::function<void()> cb, size_t stacksize=0);
    ~Fiber();
    void reset(std::function<void()> cb);
    void swapIn();// 切换到当前协程执行. 用于调度协程
    void swapOut();
    uint64_t getId() const { return m_id; }

    static Fiber::ptr GetThis();// 获取当前协程指针
    static void SetThis(Fiber* f);// 设置当前协程指针
    static void YieldToReady();// 将当前协程切换到后台，并且设置为READY状态
    static void YieldToHold();// 将当前协程切换到后台，并且设置为HOLD状态

    static uint64_t TotalFibers();// 获取协程总数
    static void MainFunc();// 协程执行函数入口
    static uint64_t GetFiberId();// 获取当前协程id

private:
    Fiber();
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    State m_state = INIT;
    ucontext_t m_ctx;// 保存协程上下文环境
    void* m_stack = nullptr;// 协程栈指针

    std::function<void()> m_cb;


};

}



#endif