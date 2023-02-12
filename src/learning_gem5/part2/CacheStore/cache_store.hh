#ifndef __CACHE_STORE_HH__
#define __CACHE_STORE_HH__

#include <bits/types.h>

#include "base/logging.hh"
#include "base/types.hh"
#include "base/trace.hh"
#include "debug/CacheStore.hh"

typedef __uint64_t uint64_t;
typedef unsigned char uint8_t;

// cache_line structure:
// **************************************
// ** VALID ** TAG ** BLOCK ** COUNTER **
// **************************************
typedef struct line
{
  int valid_bit;         // valid bit
  uint64_t tag;          // tag
  uint8_t * block;       // data block
  gem5::Tick counter;    // timestamp for the line

  // constructors
  line()
  {
    valid_bit = 0;  // 0 means invalid
    tag = 0;
    block = nullptr;
    counter = 0;
  }

  line(int B)
  {
    valid_bit = 0;  // 0 means invalid
    tag = 0;
    block = new uint8_t[B];
    counter = 0;
  }
} cache_line;

namespace gem5
{

// cache_store structure:
// S sets in total, and E cache_lines in each set
class CacheStore
{
private:

  // set number: S = 2 ^ s (sets)
  int s;

  // line number per set: E
  int E;

  // bytes number per block: B = 2 ^ b (Bytes)
  int b;

  // two-demension array: using set number and line number to locate a cache_line
  cache_line ** cache_store;

public:

  CacheStore(int s, int E, int b);

  ~CacheStore();

  /**
   * @param block_addr get block address(aligned), and find the content in cache
   * @return std::pair<gem5::Addr, uint8_t * > bytes stored in block_addr
  */
  std::pair<gem5::Addr, uint8_t * > find(Addr block_addr);

};

}

#endif // __CACHE_STORE_HH__
