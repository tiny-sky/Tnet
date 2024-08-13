#pragma once

#define _BSD_SOURCE
#define __USE_BSD

#include <endian.h>
#include <stdint.h>
#include <byteswap.h>

namespace {

#define htobe16_(x) __bswap_16(x)
#define be16toh_(x) __bswap_16(x)

#define htobe32_(x) __bswap_32(x)
#define be32toh_(x) __bswap_32(x)

#define htobe64_(x) __bswap_64(x)
#define be64toh_(x) __bswap_64(x)

}  // namespace

namespace Tnet {

namespace Endian {

inline uint64_t hostToNetwork64(uint64_t host64) {
  return htobe64_(host64);
}

inline uint32_t hostToNetwork32(uint32_t host32) {
  return htobe32_(host32);
}

inline uint16_t hostToNetwork16(uint16_t host16) {
  return htobe16_(host16);
}

inline uint64_t networkToHost64(uint64_t net64) {
  return be64toh_(net64);
}

inline uint32_t networkToHost32(uint32_t net32) {
  return be32toh_(net32);
}

inline uint16_t networkToHost16(uint16_t net16) {
  return be16toh_(net16);
}

}  // namespace Endian

}  // namespace Tnet
