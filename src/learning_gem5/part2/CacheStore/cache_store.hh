#ifndef __CACHE_STORE_HH__
#define __CACHE_STORE_HH__

#include <bits/types.h>

#include "base/logging.hh"
#include "base/types.hh"
#include "base/trace.hh"
#include "debug/CacheStore.hh"
#include "sim/cur_tick.hh"

typedef __uint64_t uint64_t;
typedef unsigned char uint8_t;

// cache_line structure:
// **************************************
// ** VALID ** TAG ** BLOCK ** COUNTER **
// **************************************
typedef struct line
{
  int valid_bit;            // valid bit
  uint64_t tag;             // tag
  uint8_t *block;           // data block
  gem5::Tick counter;       // timestamp for the line
  gem5::Tick creation_time; // timestamp for the line for FIFO
  // constructors
  line()
  {
    valid_bit = 0; // 0 means invalid
    tag = 0;
    block = nullptr;
    counter = 0;
  }

  line(int B)
  {
    valid_bit = 0; // 0 means invalid
    tag = 0;
    block = new uint8_t[B];
    counter = 0;
    creation_time = 0;
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
    cache_line **cache_store;

  public:
    CacheStore(int s, int E, int b);

    ~CacheStore();

    /**
     * @param block_addr get block address(aligned), and find the content in cache
     * @return std::pair<gem5::Addr, uint8_t * > bytes stored in block_addr
     */
    std::pair<gem5::Addr, uint8_t *> find(Addr block_addr);

    /**
     * @param address block address(aligned), the starting address of the block to be stored
     * @param data the data content of memory to be stored inside the cache
     * @return none
     */
    void set(gem5::Addr address, uint8_t *data);

    /**
     * @param address for the given address, the function checks whether the corresponding set is full
     * @return true means the set is full and involves replacement policy
     */
    bool isFull(gem5::Addr address);

    /**
     * the address should be pre-determined and is not the packet address to be inserted
     * @param address erase the cache_line with 'address' being the starting Addr of the block
     * @return none
     */
    void erase(gem5::Addr address);

    /**
     * @param address should be the address of a new packet to be inserted into CacheStore
     * @return std::pair<gem5::Addr, uint8_t * > the line picked to be replaced
     */
    std::pair<gem5::Addr, uint8_t *> pick_line(gem5::Addr address);

    /**
     * @return combine three parameters into the gem5 address
     */
    gem5::Addr combine(uint64_t tag, int set, int block);

    /**
     * @param lines the set where we choose a cache line from
     * @return the line number we choose based on the LRU replacement policy
     */
    int policy_LRU(cache_line *lines);

    /**
     * @param lines the set where we choose a cache line from
     * @return the line number we choose based on the FIFO replacement policy
     */
    int policy_FIFO(cache_line *lines);
    /**
     * For debugging...
     * @param set print out the info about all the lines in this set
     */
    void print_set(int set);
  };

}

#endif // __CACHE_STORE_HH__
