#include "fiber.h"
#include<atomic>
#include"log.h"
namespace sylar
{
    std::atomic<uint64_t> s_fiber_id{0};// 全局协程ID
    std::atomic<uint64_t> s_fiber_count{0};// 全局协程数量

    static thread_local Fiber* t_fiber=nullptr;
    static thread_local std::shared_ptr<Fiber> t_threadFiber=nullptr;// MASTER协程
    static ConfigVar<uint32_t>::ptr g_fiber_stack_size=Config::Lookup<uint32_t>("fiber.stack_size",128*1024,"fiber stack size");

    static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    class MallocStackAllocator
    {
    public:
        static void *Alloc(size_t size)
        {
            return malloc(size);
        }
        static void Dealloc(void *vp, size_t size)
        {
            free(vp);
        }
    };

    using StackAllocator = MallocStackAllocator;// 以后可以改成其他分配器

    Fiber::Fiber(){// 主协程构造函数
        m_state = EXEC;
        SetThis(this);
        if(getcontext(&m_ctx)){// 获取线程上下文，保存到m_ctx
            SYLAR_ASSERT2(false,"getcontext");
        }
        ++s_fiber_count;
    }

    Fiber::Fiber(std::function<void()> cb, size_t stacksize):m_id(++s_fiber_id),m_cb(cb){// 普通协程构造函数
        ++s_fiber_count;
        m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
        m_stack = StackAllocator::Alloc(m_stacksize);
        if(getcontext(&m_ctx)){// 获取线程上下文，保存到m_ctx
            SYLAR_ASSERT2(false,"getcontext");
        }
        m_ctx.uc_link = nullptr;// 结束后返回的上下文。设为空则结束后直接退出线程
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        makecontext(&m_ctx, &Fiber::MainFunc, 0);// 设置协程入口函数
    
    }
    Fiber::~Fiber(){
        --s_fiber_count;
        if(m_stack){
            SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
            StackAllocator::Dealloc(m_stack, m_stacksize);// 释放栈内存
        }else{// 没有栈，说明是主协程
            SYLAR_ASSERT(m_state == EXEC);
            Fiber* cur = t_fiber;
            if(cur == this){// 要把t_fiber置空
                SetThis(nullptr);
            }
        }
    }
    void Fiber::reset(std::function<void()> cb){
        SYLAR_ASSERT(m_stack);// 只能重置非主协程
        SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT);
        m_cb=cb;
        if(getcontext(&m_ctx)){// 获取线程上下文，保存到m_ctx
            SYLAR_ASSERT2(false,"getcontext");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        makecontext(&m_ctx, &Fiber::MainFunc, 0);
        m_state = INIT;

    }
    void Fiber::swapIn(){
        SetThis(this);
        SYLAR_ASSERT(m_state != EXEC);
        m_state = EXEC;
        if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)){// 保存当前主协程上下文，切换到当前协程上下文
            SYLAR_ASSERT2(false,"swapcontext");
        }
    }
    void Fiber::swapOut(){
        SetThis(t_threadFiber.get());
        if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)){// 保存
            SYLAR_ASSERT2(false,"swapcontext");
        }

    }

    Fiber::ptr Fiber::GetThis(){
        if(t_fiber){
            return t_fiber->shared_from_this();
        }
        // 如果t_fiber为空，说明当前线程没有协程
        Fiber::ptr main_fiber(new Fiber);// 创建主协程
        SYLAR_ASSERT(t_fiber == main_fiber.get());
        t_threadFiber = main_fiber;
        return t_fiber->shared_from_this();
    }

    void Fiber::SetThis(Fiber *f)
    {
        t_fiber = f;
    }
    void Fiber::YieldToReady(){
        Fiber::ptr cur = GetThis();
        // SYLAR_ASSERT(cur->m_state == EXEC);
        cur->m_state = READY;
        cur->swapOut();
    }
    void Fiber::YieldToHold(){
        Fiber::ptr cur = GetThis();
        // SYLAR_ASSERT(cur->m_state == EXEC);
        cur->m_state = HOLD;
        cur->swapOut();
    }

    uint64_t Fiber::TotalFibers(){
        return s_fiber_count;
    }
    void Fiber::MainFunc(){
        Fiber::ptr cur = GetThis();
        SYLAR_ASSERT(cur);
        try{
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        }catch(std::exception& e){
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Fiber Except: " << e.what()
                << "\n" << sylar::BacktraceToString();
        }catch(...){
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Fiber Except: unknown";
        }

        Fiber::ptr temp = cur;
        temp->swapOut();

        SYLAR_ASSERT2(false,"Fiber MainFunc never reach here");
    }

    uint64_t Fiber::GetFiberId() {
        if(t_fiber){
            return t_fiber->getId();
        }
        return 0;

    }

}