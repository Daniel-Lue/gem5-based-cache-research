# gem5-based-cache-research

## What we've done so far

1. Introduce CacheStore with a more delicate structure in `simple_cache.cc` to replace the original unordered map
2. Implement LRU replacement policy...(to be continued)

## Works to be done by Friday night(for Gordon and Idy)

Each of you is expected to involve two more replacement policies into the project. Some detailed instructions are listed as follows:

1. Learn two existing replacement policies on the Internet
2. Implement the policies in C++ and integrate into the project

## Notice

1. Please create your own branch before coding and we can do code review before merging into the main branch.
2. when `git commit`, you should add `--no-verify` to the command line. This will disable the code style checking but it is still recommended to keep the style identical.
3. To execute the program, there are various ways: `build/X86/gem5.opt configs/learning_gem5/part2/simple_cache.py` is the most basic; `build/X86/gem5.opt --debug-flags=CacheStore configs/learning_gem5/part2/simple_cache.py > src/learning_gem5/part2/debug.log` can produce a log file in the working directory.
4. Actually, it is not complicated to implement other policies. Understanding all of the related files might take some time, but the function responsible for replacement policy is decoupled from rest of the project. Therefore, just try to focus on `cache_store.cc` Line 207 `int CacheStore::policy_LRU(cache_line * lines)`, understand the meaning of parameters and return value, then imitate.
5. Remember to write comments, because other members in the group have no knowledge of the policy you are working on.
6. As for the number of replacement policies you want to work on, the more the better:) If you have any question about environment, policies or implementation, feel free to propose in our WeChat group.
