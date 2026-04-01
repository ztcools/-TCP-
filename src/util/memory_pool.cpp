#include "../../include/util/memory_pool.h"

#include <cstring>
#include <stdexcept>
#include <iostream>

namespace util {

MemoryPool::MemoryPool(size_t block_size, size_t num_blocks)
    : block_size_(block_size < sizeof(Block*) ? sizeof(Block*) : block_size),
      num_blocks_(num_blocks),
      allocated_count_(0),
      free_list_(nullptr),
      memory_(nullptr) {
  if (block_size_ == 0 || num_blocks_ == 0) {
    throw std::invalid_argument("block_size and num_blocks must be greater than 0");
  }

  Initialize();
}

MemoryPool::~MemoryPool() {
  delete[] memory_;
}

void* MemoryPool::Allocate() {
  std::lock_guard<std::mutex> lock(pool_mutex_);

  if (free_list_ == nullptr) {
    return nullptr;
  }

  Block* block = free_list_;
  free_list_ = block->next;
  ++allocated_count_;

  return static_cast<void*>(block);
}

void MemoryPool::Deallocate(void* ptr) {
  if (ptr == nullptr) {
    return;
  }

  if (!IsValidPointer(ptr)) {
    std::cerr << "MemoryPool: Invalid pointer detected" << std::endl;
    return;
  }

  std::lock_guard<std::mutex> lock(pool_mutex_);

  Block* block = static_cast<Block*>(ptr);
  block->next = free_list_;
  free_list_ = block;
  --allocated_count_;
}

size_t MemoryPool::GetBlockSize() const {
  return block_size_;
}

size_t MemoryPool::GetNumBlocks() const {
  return num_blocks_;
}

size_t MemoryPool::GetAllocatedCount() const {
  return allocated_count_.load();
}

size_t MemoryPool::GetAvailableCount() const {
  return num_blocks_ - allocated_count_.load();
}

bool MemoryPool::IsFromPool(void* ptr) const {
  return IsValidPointer(ptr);
}

void MemoryPool::Initialize() {
  size_t total_size = block_size_ * num_blocks_;
  memory_ = new char[total_size];
  std::memset(memory_, 0, total_size);

  free_list_ = reinterpret_cast<Block*>(memory_);
  Block* current = free_list_;

  for (size_t i = 0; i < num_blocks_ - 1; ++i) {
    current->next = reinterpret_cast<Block*>(memory_ + (i + 1) * block_size_);
    current = current->next;
  }

  current->next = nullptr;
  allocated_count_.store(0);
}

bool MemoryPool::IsValidPointer(void* ptr) const {
  char* char_ptr = static_cast<char*>(ptr);

  if (char_ptr < memory_ || char_ptr >= memory_ + (num_blocks_ * block_size_)) {
    return false;
  }

  size_t offset = char_ptr - memory_;
  return (offset % block_size_) == 0;
}

}  // namespace util
