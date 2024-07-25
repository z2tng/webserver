#pragma once

#include <sys/syscall.h>
#include <unistd.h>

namespace current_thread {

extern __thread int t_cached_tid;

inline int tid() {
    if (__builtin_expect(t_cached_tid == 0, 0)) {
        t_cached_tid = static_cast<int>(::syscall(SYS_gettid));
    }
    return t_cached_tid;
}

} // namespace current_thread
