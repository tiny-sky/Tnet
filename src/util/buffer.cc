#include "buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

namespace Tnet {

/**
 * 从文件描述符fd读取数据到缓冲区。
 * 使用readv进行高效读取，如果内部缓冲区不足，使用栈上额外空间暂存。
 */
ssize_t Buffer::readFd(int fd, int *saveErrno) {
  char extrabuf[65536];  // 额外的栈上空间，用于暂存数据

  struct iovec vec[2];
  const std::size_t writable = writableBytes();  // 缓冲区剩余可写空间

  vec[0].iov_base = begin() + writerIndex_;
  vec[0].iov_len = writable;
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof(extrabuf);

  const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
  const ssize_t n = ::readv(fd, vec, iovcnt);

  if (n < 0) {
    *saveErrno = errno;
  } else if (n <= static_cast<ssize_t>(writable)) {
    writerIndex_ += n;
  } else {
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  return n;
}

/**
 * 将缓冲区数据通过文件描述符fd发送出去。
 */
ssize_t Buffer::writeFd(int fd, int *saveErrno) {
  ssize_t n = ::write(fd, peek(), readableBytes());
  if (n < 0) {
    *saveErrno = errno;
  }
  return n;
}

}  // namespace Tnet