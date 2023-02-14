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

// Function 'find' in simple_cache.cc has been replaced by CacheStore
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

// Function 'set' in simple_cache.cc has been replaced by CacheStore
// handle response, store data into the cache line (vacancy can be assured in this function)
void CacheStore::set(gem5::Addr address, uint8_t * data)
{
  // step 01: parse the target address into three parts
  panic_if(this->b >= 32, "non-negative int has 31 bits at most");
  int block = address & ((1 << this->b) - 1);  // block offset
  assert(block == 0);  // block address is aligned in this function

  panic_if(this->s >= 32, "non-negative int has 31 bits at most");
  int set = address & (((1 << this->s) - 1) << this->b);  // set number

  uint64_t tag = address >> (this->s + this->b);

  // step 02: determine the line number to be stored (cache miss but there must be one vacant line)
  cache_line * target = this->cache_store[set];
  int i = 0, j = -1;  // j is used to record the line number in the set, -1 means not found
  for ( ; i < this->E; ++i)
  {
    if (target[i].valid_bit == 1 && target[i].tag == tag)
      panic("cache line should not hit!");
    if (target[i].valid_bit == 0 && j == -1)  // this line can be replaced if j = -1
      j = i;
  }
  panic_if(j == -1, "no vacant line can be found");

  // step 03: modify the cache line in CacheStore
  target[j].valid_bit = 1;
  target[j].tag = tag;
  target[j].block = data;  // data has not been written into the cache line block
  target[j].counter = curTick();  // current tick when the cache line is set
}

bool CacheStore::isFull(gem5::Addr address)
{
  
}

}
