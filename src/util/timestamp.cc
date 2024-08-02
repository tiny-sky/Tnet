#include "timestamp.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace Tnet {
Timestamp::Timestamp()
    : microSecondsSinceEpoch_(now().microSecondsSinceEpoch_) {}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

Timestamp Timestamp::now() {
  using namespace std::chrono;
  auto now = system_clock::now();
  auto duration = now.time_since_epoch();
  return Timestamp(duration_cast<microseconds>(duration).count());
}

std::string Timestamp::toString() const {
  using namespace std::chrono;
  auto duration = microseconds(microSecondsSinceEpoch_);
  auto seconds = duration_cast<std::chrono::seconds>(duration);

  std::time_t time = seconds.count();
  std::tm tm = *std::localtime(&time);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}
}  // namespace Tnet
