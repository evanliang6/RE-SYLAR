#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include<string>
#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include<semaphore.h>
#include<stdint.h>
#include<atomic>
namespace sylar {

class Semaphore{
    public:
    Semaphore(uint32_t count=0);
    ~Semaphore();
    void wait();
    void notify();
    private:
    Semaphore(const Semaphore&) = delete;
    Semaphore(const Semaphore&&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

    sem_t m_semaphore;
};

/*
    * 互斥锁的封装。防止忘记解锁
*/
template<class T>
struct ScopedLockImpl{
    public:
    ScopedLockImpl(T& mutex)
        :m_mutex(mutex){
        m_mutex.lock();// Type T 必须实现 lock 和 unlock 方法。一般是mutex等
        m_locked=true;
    }
    ~ScopedLockImpl(){
        m_mutex.unlock();
    }
    void lock(){
        if(!m_locked){
            m_mutex.lock();
            m_locked=true;
        }
    }
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked=false;
        }
    }
    private:
    T& m_mutex;
    bool m_locked=false;
};

template<class T>
class ReadScopedLockImpl{
    public:
    ReadScopedLockImpl(T& mutex)
        :m_mutex(mutex){
        m_mutex.rdlock();// Type T 必须实现 rdlock 和 unlock 方法。一般是RWMutex等
        m_locked=true;
    }
    ~ReadScopedLockImpl(){
        m_mutex.unlock();
    }
    void lock(){
        if(!m_locked){
            m_mutex.rdlock();
            m_locked=true;
        }
    }
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked=false;
        }
    }
    private:
    T& m_mutex;
    bool m_locked=false;
};

template<class T>
class WriteScopedLockImpl{
    public:
    WriteScopedLockImpl(T& mutex)
        :m_mutex(mutex){
        m_mutex.wrlock();// Type T 必须实现 wrlock 和 unlock 方法。一般是RWMutex等
        m_locked=true;
    }
    ~WriteScopedLockImpl(){
        m_mutex.unlock();
    }
    void lock(){
        if(!m_locked){
            m_mutex.wrlock();
            m_locked=true;
        }
    }
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked=false;
        }
    }
    private:
    T& m_mutex;
    bool m_locked=false;
};
class NullMutex{
    // 测试用的空锁
    typedef ScopedLockImpl<NullMutex> Lock;
    public:
    NullMutex(){}
    ~NullMutex(){}
    void lock(){}
    void unlock(){}
};
class Mutex{
public:
    typedef ScopedLockImpl<Mutex> Lock;
    Mutex(){
        pthread_mutex_init(&m_mutex, nullptr);
    }
    ~Mutex(){
        pthread_mutex_destroy(&m_mutex);
    }
    void lock(){
        pthread_mutex_lock(&m_mutex);// 当其他线程已经锁住该互斥锁时，调用线程会阻塞，直到互斥锁被解锁
    }
    void unlock(){
        pthread_mutex_unlock(&m_mutex);
    }
    private:
    pthread_mutex_t m_mutex;
};
class NullRWMutex{
    // 测试用的空读写锁
    typedef ReadScopedLockImpl<NullRWMutex> ReadLock;
    typedef WriteScopedLockImpl<NullRWMutex> WriteLock;
    public:
    NullRWMutex(){}
    ~NullRWMutex(){}
    void rdlock(){}
    void wrlock(){}
    void unlock(){}
};
class RWMutex{
public:
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    typedef WriteScopedLockImpl<RWMutex> WriteLock;
    RWMutex(){
        pthread_rwlock_init(&m_lock, nullptr);
    }
    ~RWMutex(){
        pthread_rwlock_destroy(&m_lock);
    }
    void rdlock(){
        pthread_rwlock_rdlock(&m_lock);
    }
    void wrlock(){
        pthread_rwlock_wrlock(&m_lock);
    }
    void unlock(){
        pthread_rwlock_unlock(&m_lock);
    }
    private:
    pthread_rwlock_t m_lock;
};

// 自旋锁，针对冲突时间较短的场景.避免切换线程的开销
class spinlock{
public:
    typedef ScopedLockImpl<spinlock> Lock;
    spinlock(){
        pthread_spin_init(&m_mutex, 0);
    }
    ~spinlock(){
        pthread_spin_destroy(&m_mutex);
    }
    void lock(){
        pthread_spin_lock(&m_mutex);
    }
    void unlock(){
        pthread_spin_unlock(&m_mutex);
    }
private:
    pthread_spinlock_t m_mutex;
};

// CAS自旋锁。和spinlock一样对atomic封装，但是不依赖pthread
class CASLock{
public:
    typedef ScopedLockImpl<CASLock> Lock;
    CASLock(){
        m_mutex.clear();
    }
    ~CASLock(){
        
    }
    void lock(){
        while(std::atomic_flag_test_and_set_explicit(&m_mutex,std::memory_order_acquire));
    }
    void unlock(){
        std::atomic_flag_clear_explicit(&m_mutex,std::memory_order_release);// explicit 显式内存序
    }
private:
    volatile std::atomic_flag m_mutex = ATOMIC_FLAG_INIT;
};

class Thread{
    public:
    typedef std::shared_ptr<Thread> ptr;    
    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();
    const std::string& getName() const { return m_name; }
    
    pid_t getId() const { return m_id; }
    void join();// 等待线程执行完毕
    static Thread* GetThis();// 获取当前线程指针
    static const std::string& GetName();
    static void SetName(const std::string& name);


    private:
    Thread(const Thread&) =delete;// 禁止默认拷贝构造函数
    Thread(const Thread&&) =delete;// 禁止默认移动构造函数
    Thread& operator=(const Thread&) = delete;// 禁止默认拷贝赋值
    static void* run(void* arg);// 线程执行函数

    pid_t m_id=-1;// 线程id
    pthread_t m_thread=0;// 线程句柄
    std::function<void()> m_cb;
    std::string m_name;
    Semaphore m_semaphore; // 信号量，确保线程创建成功

};


}


#endif