#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <string>
#include <vector>
#include "tcpserver/endian.h"
#include "util/log.h"
#include "util/macros.h"

namespace Tnet {
class Buffer {
  public:
  static const std::size_t kCheapPrepend = 8;    // 预留的前置空间
  static const std::size_t kInitialSize = 1024;  // 初始大小

  explicit Buffer(std::size_t initialSize = kInitialSize)
      : buffer_(kCheapPrepend + initialSize),
        readerIndex_(kCheapPrepend),
        writerIndex_(kCheapPrepend) {
    assert(readableBytes() == 0);
    assert(writableBytes() == initialSize);
    assert(prependableBytes() == kCheapPrepend);
  }

  DISALLOW_COPY(Buffer);

  // 可读数据的大小
  std::size_t readableBytes() const { return writerIndex_ - readerIndex_; }
  // 可写数据的大小
  std::size_t writableBytes() const { return buffer_.size() - writerIndex_; }
  // 前置空间的大小
  std::size_t prependableBytes() const { return readerIndex_; }

  // 获取可读数据的起始指针
  const char* peek() const { return begin() + readerIndex_; }

  // 从缓冲区中移除len长度的数据
  void retrieve(std::size_t len) {
    if (len < readableBytes()) {
      readerIndex_ += len;
    } else {
      retrieveAll();
    }
  }

  // 重置缓冲区
  void retrieveAll() { readerIndex_ = writerIndex_ = kCheapPrepend; }

  std::string retrieveAllAsString() {
    return retrieveAsString(readableBytes());
  }
  std::string retrieveAsString(std::size_t len) {
    std::string result(peek(), len);
    retrieve(len);
    return result;
  }

  // 确保有足够的可写空间
  void ensureWritableBytes(std::size_t len) {
    if (writableBytes() < len) {
      makeSpace(len);  // 扩容处理
    }
  }

  // 向缓冲区追加数据
  void append(const char* data, std::size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    writerIndex_ += len;
  }

  void append(const void* data, std::size_t len) {
    append(static_cast<const char*>(data), len);
  }

  void appendInt32(int32_t x) {
    int32_t be32 = Endian::hostToNetwork32(x);
    append(&be32, sizeof(be32));
  }

  int32_t peekInt32() const {
    assert(readableBytes() >= sizeof(int32_t));
    int32_t be32 = 0;
    ::memcpy(&be32,peek(),sizeof(int32_t));
    return Endian::networkToHost32(be32);
  }

  // 获取可写数据的起始指针
  char* beginWrite() { return begin() + writerIndex_; }
  const char* beginWrite() const { return begin() + writerIndex_; }

  // 读取数据
  ssize_t readFd(int fd, int* saveErrno);
  // 发送数据
  ssize_t writeFd(int fd, int* saveErrno);

  void hasWritten(std::size_t len) {
    assert(len <= writableBytes());
    writerIndex_ += len;
  }

  void unwrite(std::size_t len) {
    assert(len <= readableBytes());
    writerIndex_ -= len;
  }

  void prepend(const void* data, std::size_t len) {
    assert(len <= prependableBytes());
    readerIndex_ -= len;
    const char* d = static_cast<const char*>(data);
    std::copy(d, d + len, begin() + readerIndex_);
  }

  private:
  char* begin() { return &*buffer_.begin(); }
  const char* begin() const { return &*buffer_.begin(); }

  // 扩容逻辑
  void makeSpace(std::size_t len) {
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
      buffer_.resize(writerIndex_ + len);
    } else {
      std::size_t readable = readableBytes();
      std::copy(begin() + readerIndex_, begin() + writerIndex_,
                begin() + kCheapPrepend);
      readerIndex_ = kCheapPrepend;
      writerIndex_ = readerIndex_ + readable;
    }
  }

  std::vector<char> buffer_;  // 实际存储数据的vector
  std::size_t readerIndex_;   // 读索引
  std::size_t writerIndex_;   // 写索引
};

}  // namespace Tnet
