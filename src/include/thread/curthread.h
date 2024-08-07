#pragma once

#include <sys/syscall.h>
#include <unistd.h>

namespace CurrentThread {
extern __thread int
    t_cachedTid;  // 保存tid缓存 因为系统调用非常耗时 拿到tid后将其保存

void cacheTid();

inline int tid() {
  // __builtin_expect 是一种底层优化 此语句意思是如果还未获取tid 进入if 通过cacheTid()系统调用获取tid
  if (__builtin_expect(t_cachedTid == 0, 0)) {
    cacheTid();
  }
  return t_cachedTid;
}
}  // namespace CurrentThread