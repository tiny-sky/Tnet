#pragma once
#include <google/protobuf/io/zero_copy_stream.h>
#include "util/buffer.h"

namespace Tnet {

class BufferOutStream : public google::protobuf::io::ZeroCopyOutputStream {
  public:
  BufferOutStream(Buffer* buf)
      : buffer_(buf), originalSize_(buffer_->readableBytes()) {}

  virtual bool Next(void** data, int* size) {
    buffer_->ensureWritableBytes(4096);
    *data = buffer_->beginWrite();
    *size = static_cast<int>(buffer_->writableBytes());
    buffer_->hasWritten(*size);
    return true;
  }

  virtual void Backup(int count) {
    buffer_->unwrite(count);
  }

  virtual int64_t ByteCount() const {
    return buffer_->readableBytes() - originalSize_;
  }


  private:
  Buffer* buffer_;
  std::size_t originalSize_;
};
}  // namespace Tnet
