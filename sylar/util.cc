#include "util.h"

namespace sylar {
pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

uint32_t GetFiberId() {
    // 协程号无法通过
    return 0;
}

}
