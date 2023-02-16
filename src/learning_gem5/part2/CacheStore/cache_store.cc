#include "learning_gem5/part2/CacheStore/cache_store.hh"
#include <algorithm>
#include <iterator>
namespace gem5
{

  CacheStore::CacheStore(int s, int E, int b) : s(s), E(E), b(b)
  {
    // allocate the heap space for the cache simulator
    cache_store = new cache_line *[1 << this->s];
    for (int i = 0; i < (1 << this->s); ++i)
    {
      cache_store[i] = new cache_line[this->E];
      for (int j = 0; j < this->E; ++j)
        cache_store[i][j].block = new uint8_t[1 << this->b];
    }

    DPRINTF(CacheStore, "Finish Constructing CacheStore...\n");
    DPRINTF(CacheStore, "set no: %d, line no: %d, block no: %d\n", (1 << this->s), this->E, (1 << this->b));
  }

  CacheStore::~CacheStore()
  {
    for (int i = 0; i < (1 << this->s); ++i)
    {
      for (int j = 0; j < this->E; ++j)
        delete[] cache_store[i][j].block;
      delete[] this->cache_store[i];
    }
    delete[] this->cache_store;

    DPRINTF(CacheStore, "Finish Destructing CacheStore...\n");
  }

  // Function 'find' in simple_cache.cc has been replaced by CacheStore
  std::pair<gem5::Addr, uint8_t *> CacheStore::find(Addr block_addr)
  {
    DPRINTF(CacheStore, "whether or not addr %#x can be found\n", block_addr);

    // step 01: parse the block_addr and find the target set
    panic_if(this->b >= 32, "non-negative int has 31 bits at most");
    int block = block_addr & ((1 << this->b) - 1); // block offset
    assert(block == 0);                            // block address is aligned in this function

    panic_if(this->s >= 32, "non-negative int has 31 bits at most");
    int set = (block_addr & (((1 << this->s) - 1) << this->b)) >> this->b; // set number

    uint64_t tag = block_addr >> (this->s + this->b);

    DPRINTF(CacheStore, "parsing result: (block set tag) %d %d %#x\n", block, set, tag);

    // step 02: find the cache line according to block, set and tag
    cache_line *target = this->cache_store[set];
    std::pair<gem5::Addr, uint8_t *> res(block_addr, nullptr);
    for (int i = 0; i < this->E; ++i)
    {
      if (target[i].valid_bit == 0)
        continue; // this is not a valid line
      // if tag matches, then the cache line is found
      if (target[i].tag == tag)
      {
        res.second = target[i].block;
        DPRINTF(CacheStore, "!!!Cache hit!!!\n");
        // remember to update the counter of every cache line when visited (LRU)
        target[i].counter = curTick();
        break;
      }
    }

    // step 03: exit the cycle, nullptr marks no cache line can be found
    return res;
  }

  // Function 'set' in simple_cache.cc has been replaced by CacheStore
  // handle response, store data into the cache line (vacancy can be assured in this function)
  void CacheStore::set(gem5::Addr address, uint8_t *data)
  {
    DPRINTF(CacheStore, "set addr %#x into CacheStore\n", address);

    // step 01: parse the target address into three parts
    panic_if(this->b >= 32, "non-negative int has 31 bits at most");
    int block = address & ((1 << this->b) - 1); // block offset
    assert(block == 0);                         // block address is aligned in this function

    panic_if(this->s >= 32, "non-negative int has 31 bits at most");
    int set = (address & (((1 << this->s) - 1) << this->b)) >> this->b; // set number

    uint64_t tag = address >> (this->s + this->b);

    DPRINTF(CacheStore, "parsing result: (block set tag) %d %d %#x\n", block, set, tag);

    // step 02: determine the line number to be stored (cache miss but there must be one vacant line)
    cache_line *target = this->cache_store[set];
    int i = 0, j = -1; // j is used to record the line number in the set, -1 means not found
    for (; i < this->E; ++i)
    {
      if (target[i].valid_bit == 1 && target[i].tag == tag)
        panic("cache line should not hit!");
      if (target[i].valid_bit == 0 && j == -1) // this line can be replaced if j = -1
        j = i;
    }
    panic_if(j == -1, "no vacant line can be found");

    DPRINTF(CacheStore, "the line number chosen to place the address: %d\n", j);

    // step 03: modify the cache line in CacheStore
    target[j].valid_bit = 1;
    target[j].tag = tag;
    target[j].block = data;              // data has not been written into the cache line block
    target[j].creation_time = curTick(); // current tick when the cache line is set for FIFO
    target[j].counter = curTick();       // current tick when the cache line is set
  }

  bool CacheStore::isFull(gem5::Addr address)
  {
    DPRINTF(CacheStore, "Is addr %#x in a full set?\n", address);

    // return true iff all cache lines in that set are valid
    panic_if(this->b >= 32, "non-negative int has 31 bits at most");
    int block = address & ((1 << this->b) - 1); // block offset
    assert(block == 0);                         // block address is aligned in this function

    panic_if(this->s >= 32, "non-negative int has 31 bits at most");
    int set = (address & (((1 << this->s) - 1) << this->b)) >> this->b; // set number
    cache_line *target = this->cache_store[set];

    bool flag = true; // switch to false if valid bit == 0
    for (int i = 0; i < this->E; ++i)
    {
      if (target[i].valid_bit == 0)
      {
        flag = false;
        break;
      }
    }

    if (flag)
      DPRINTF(CacheStore, "The set is full!\n");
    else
      DPRINTF(CacheStore, "The set is NOT full!\n");

    return flag;
  }

  void CacheStore::erase(gem5::Addr address)
  {
    DPRINTF(CacheStore, "erase cache line at addr %#x\n", address);

    // by simply setting the valid_bit to be 0, we can erase the cache_line
    panic_if(this->b >= 32, "non-negative int has 31 bits at most");
    int block = address & ((1 << this->b) - 1); // block offset
    assert(block == 0);                         // block address is aligned in this function

    panic_if(this->s >= 32, "non-negative int has 31 bits at most");
    int set = (address & (((1 << this->s) - 1) << this->b)) >> this->b; // set number

    uint64_t tag = address >> (this->s + this->b);

    cache_line *target = this->cache_store[set];
    for (int i = 0; i < this->E; ++i)
    {
      // when this function is called, all of the cache lines should be occupied
      assert(target[i].valid_bit == 1);
      if (target[i].tag == tag) // having found the line to be deleted
      {
        target[i].valid_bit = 0;
        DPRINTF(CacheStore, "setting the valid_bit at line %d to be zero\n", i);
      }
    }
  }

  std::pair<gem5::Addr, uint8_t *> CacheStore::pick_line(gem5::Addr address)
  {
    DPRINTF(CacheStore, "The addr of a new pkt to be inserted: %#x\n", address);

    panic_if(this->b >= 32, "non-negative int has 31 bits at most");
    int block = address & ((1 << this->b) - 1); // block offset
    assert(block == 0);                         // block address is aligned in this function

    panic_if(this->s >= 32, "non-negative int has 31 bits at most");
    int set = (address & (((1 << this->s) - 1) << this->b)) >> this->b; // set number
    cache_line *target = this->cache_store[set];

    // introduce the replacement policy to pick a line in the selected set
    int line_number = policy_LRU(target);
    DPRINTF(CacheStore, "Line %d will be erased later\n", line_number);
    Addr addr = combine(target[line_number].tag, set, block);
    DPRINTF(CacheStore, "Line %d starts at addr %#x\n", line_number, addr);

    std::pair<gem5::Addr, uint8_t *> res(addr, target[line_number].block);
    return res;
  }

  gem5::Addr CacheStore::combine(uint64_t tag, int set, int block)
  {
    DPRINTF(CacheStore, "before combining: (block set tag) %d %d %#x\n", block, set, tag);

    assert(block == 0);
    Addr addr = tag << (this->s + this->b);
    return addr | (set << this->b);
  }

  void CacheStore::print_set(int set)
  {
    DPRINTF(CacheStore, "Printing info about set %d...\n", set);
    for (int i = 0; i < this->E; ++i)
    {
      DPRINTF(CacheStore, "[Set %d Line %d] valid %d tag %#x\n", set, i, this->cache_store[set][i].valid_bit, this->cache_store[set][i].tag);
    }
  }

  int CacheStore::policy_LRU(cache_line *lines)
  {
    DPRINTF(CacheStore, "Replacement Policy: LRU...\n");

    int res = 0;                        // the line number that will be returned to the caller
    gem5::Tick tick = lines[0].counter; // initialized to the first line in the set
    // go through other lines and update the res if we find a smaller tick

    for (int i = 1; i < this->E; ++i)
    {
      if (lines[i].counter < tick)
      {
        tick = lines[i].counter;
        res = i;
      }
    } // we find the stale cache line(res) in this set when exiting the cycle

    return res;
  }
  // IMPLEMENTATION OF FIFO REPLACEMENT POLICY
  int policy_FIFO(cache_line *lines)
  {
    DPRINTF(CacheStore, "Replacement Policy: FIFO...\n");

    int res = 0; // the line number that will be returned to the caller
    auto a = [=](cache_line a, cache_line b)
    { return a.creation_time < b.creation_time; };
    return std::distance(lines, std::min_element(lines, lines + this->E, a));
    }
}
