#pragma once

#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>

// Runs fn(i) for every i in 0 .. count-1, using all CPU cores.
// Each thread keeps grabbing the next unused i until none are left, so a
// thread that gets easy jobs just does more of them.
// fn must only write to data that belongs to its own i.
template <typename F> void parallel_for(int count, F fn) {
  static const int threads =
      std::max(1, (int)std::thread::hardware_concurrency());
  std::atomic<int> next{0};
  auto worker = [&]() {
    for (int i = next++; i < count; i = next++)
      fn(i);
  };
  std::vector<std::thread> pool;
  for (int t = 1; t < threads; ++t)
    pool.emplace_back(worker);
  worker(); // this thread works too
  for (auto &th : pool)
    th.join();
}

// Same, but hands out [begin, end) ranges of `chunk` items instead of single
// items (cheaper when there are hundreds of thousands of tiny jobs).
template <typename F> void parallel_for_chunks(int count, int chunk, F fn) {
  int chunks = (count + chunk - 1) / chunk;
  parallel_for(chunks, [&](int c) {
    int begin = c * chunk;
    int end = std::min(count, begin + chunk);
    fn(begin, end);
  });
}
