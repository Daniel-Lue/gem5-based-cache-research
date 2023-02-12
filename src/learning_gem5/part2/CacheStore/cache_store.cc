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

}
