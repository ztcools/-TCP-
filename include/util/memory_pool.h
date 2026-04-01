#ifndef UTIL_MEMORY_POOL_H
#define UTIL_MEMORY_POOL_H

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>
#include <atomic>

namespace util {

class MemoryPool {
 public:
  explicit MemoryPool(size_t block_size, size_t num_blocks);
  ~MemoryPool();

  MemoryPool(const MemoryPool&) = delete;
  MemoryPool& operator=(const MemoryPool&) = delete;

  void* Allocate();
  void Deallocate(void* ptr);

  size_t GetBlockSize() const;
  size_t GetNumBlocks() const;
  size_t GetAllocatedCount() const;
  size_t GetAvailableCount() const;
  bool IsFromPool(void* ptr) const;

 private:
  struct Block {
    Block* next;
  };

  void Initialize();
  bool IsValidPointer(void* ptr) const;

  size_t block_size_;
  size_t num_blocks_;
  std::atomic<size_t> allocated_count_;

  Block* free_list_;
  char* memory_;
  std::mutex pool_mutex_;
};

}  // namespace util

#endif  // UTIL_MEMORY_POOL_H
