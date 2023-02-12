#include "learning_gem5/part2/CacheStore/cache_store.hh"

namespace gem5
{

CacheStore::CacheStore(int s, int E, int b) : s(s), E(E), b(b)
{
  // allocate the heap space for the cache simulator
  cache_store = new cache_line * [1 << this->s];
  for (int i = 0; i < (1 << this->s); ++i)
  {
    cache_store[i] = new cache_line [this->E];
    for (int j = 0; j < this->E; ++j)
      cache_store[i][j].block = new uint8_t [1 << this->b];
  }

  DPRINTF(CacheStore, "Finish Constructing CacheStore...");
}

CacheStore::~CacheStore()
{
  for (int i = 0; i < (1 << this->s); ++i)
  {
    for (int j = 0; j < this->E; ++j)
      delete [] cache_store[i][j].block;
    delete [] this->cache_store[i];
  }
  delete [] this->cache_store;

  DPRINTF(CacheStore, "Finish Destructing CacheStore...");
}

std::pair<gem5::Addr, uint8_t * > CacheStore::find(Addr block_addr)
{
  // step 01: parse the block_addr and find the target set
  panic_if(this->b >= 32, "non-negative int has 31 bits at most");
  int block = block_addr & ((1 << this->b) - 1);  // block offset
  assert(block == 0);  // block address is aligned in this function

  panic_if(this->s >= 32, "non-negative int has 31 bits at most");
  int set = block_addr & (((1 << this->s) - 1) << this->b);  // set number

  uint64_t tag = block_addr >> (this->s + this->b);

  // step 02: find the cache line according to block, set and tag
  cache_line * target = this->cache_store[set];
  std::pair<gem5::Addr, uint8_t * > res (block_addr, nullptr);
  for (int i = 0; i < this->E; ++i)
  {
    if (target[i].valid_bit == 0) continue;  // this is not a valid line
    // if tag matches, then the cache line is found
    if (target[i].tag == tag)
    {
      res.second = target[i].block;
      break;
    }
  }

  // step 03: exit the cycle, nullptr marks no cache line can be found
  return res;
}

}
