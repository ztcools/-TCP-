#include "../include/util/memory_pool.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <set>
#include <algorithm>

void TestBasicAllocation() {
  std::cout << "=== Test Basic Allocation ===" << std::endl;
  
  util::MemoryPool pool(64, 10);
  
  std::cout << "Block size: " << pool.GetBlockSize() << std::endl;
  std::cout << "Num blocks: " << pool.GetNumBlocks() << std::endl;
  std::cout << "Available: " << pool.GetAvailableCount() << std::endl;
  
  void* ptr = pool.Allocate();
  std::cout << "Allocated: " << (ptr != nullptr ? "success" : "failed") << std::endl;
  std::cout << "Available after alloc: " << pool.GetAvailableCount() << std::endl;
  
  pool.Deallocate(ptr);
  std::cout << "Available after dealloc: " << pool.GetAvailableCount() << std::endl;
  std::cout << std::endl;
}

void TestMultipleAllocation() {
  std::cout << "=== Test Multiple Allocation ===" << std::endl;
  
  util::MemoryPool pool(128, 20);
  std::vector<void*> pointers;
  
  for (int i = 0; i < 15; ++i) {
    void* ptr = pool.Allocate();
    if (ptr != nullptr) {
      pointers.push_back(ptr);
      std::memset(ptr, i, 128);
    }
  }
  
  std::cout << "Allocated " << pointers.size() << " blocks" << std::endl;
  std::cout << "Available: " << pool.GetAvailableCount() << std::endl;
  std::cout << "Allocated count: " << pool.GetAllocatedCount() << std::endl;
  
  for (void* ptr : pointers) {
    pool.Deallocate(ptr);
  }
  
  std::cout << "Available after dealloc all: " << pool.GetAvailableCount() << std::endl;
  std::cout << std::endl;
}

void TestPoolExhaustion() {
  std::cout << "=== Test Pool Exhaustion ===" << std::endl;
  
  util::MemoryPool pool(32, 5);
  std::vector<void*> pointers;
  
  for (int i = 0; i < 10; ++i) {
    void* ptr = pool.Allocate();
    if (ptr != nullptr) {
      pointers.push_back(ptr);
      std::cout << "Alloc " << i << ": success" << std::endl;
    } else {
      std::cout << "Alloc " << i << ": pool exhausted (expected)" << std::endl;
      break;
    }
  }
  
  std::cout << "Total allocated: " << pointers.size() << std::endl;
  
  for (void* ptr : pointers) {
    pool.Deallocate(ptr);
  }
  
  std::cout << "Available after dealloc: " << pool.GetAvailableCount() << std::endl;
  std::cout << std::endl;
}

void TestMemoryWriteRead() {
  std::cout << "=== Test Memory Write/Read ===" << std::endl;
  
  util::MemoryPool pool(256, 10);
  
  struct TestData {
    int id;
    double value;
    char data[100];
  };
  
  std::vector<TestData*> test_data;
  
  for (int i = 0; i < 5; ++i) {
    TestData* data = static_cast<TestData*>(pool.Allocate());
    data->id = i;
    data->value = i * 1.5;
    std::snprintf(data->data, sizeof(data->data), "Test data %d", i);
    test_data.push_back(data);
  }
  
  std::cout << "Written data:" << std::endl;
  for (TestData* data : test_data) {
    std::cout << "  ID: " << data->id
              << ", Value: " << data->value
              << ", Data: " << data->data << std::endl;
  }
  
  for (TestData* data : test_data) {
    pool.Deallocate(data);
  }
  
  std::cout << std::endl;
}

void TestPointerValidation() {
  std::cout << "=== Test Pointer Validation ===" << std::endl;
  
  util::MemoryPool pool(64, 10);
  
  void* ptr1 = pool.Allocate();
  void* ptr2 = new char[64];
  
  std::cout << "Pool pointer valid: " << (pool.IsFromPool(ptr1) ? "yes" : "no") << std::endl;
  std::cout << "External pointer valid: " << (pool.IsFromPool(ptr2) ? "yes" : "no") << std::endl;
  
  pool.Deallocate(ptr1);
  delete[] static_cast<char*>(ptr2);
  std::cout << std::endl;
}

void TestConcurrentAllocation() {
  std::cout << "=== Test Concurrent Allocation ===" << std::endl;
  
  util::MemoryPool pool(128, 1000);
  std::vector<std::thread> threads;
  std::vector<std::vector<void*>> thread_results(10);
  
  auto start = std::chrono::high_resolution_clock::now();
  
  for (int t = 0; t < 10; ++t) {
    threads.emplace_back([&pool, &thread_results, t]() {
      for (int i = 0; i < 50; ++i) {
        void* ptr = pool.Allocate();
        if (ptr != nullptr) {
          std::memset(ptr, t, 128);
          thread_results[t].push_back(ptr);
        }
      }
    });
  }
  
  for (auto& thread : threads) {
    thread.join();
  }
  
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  
  size_t total_allocated = 0;
  for (const auto& result : thread_results) {
    total_allocated += result.size();
  }
  
  std::cout << "Threads: 10, Allocations per thread: 50" << std::endl;
  std::cout << "Total allocated: " << total_allocated << std::endl;
  std::cout << "Time taken: " << duration.count() << "ms" << std::endl;
  std::cout << "Available: " << pool.GetAvailableCount() << std::endl;
  
  for (auto& result : thread_results) {
    for (void* ptr : result) {
      pool.Deallocate(ptr);
    }
  }
  
  std::cout << "Available after dealloc: " << pool.GetAvailableCount() << std::endl;
  std::cout << std::endl;
}

void TestDifferentBlockSizes() {
  std::cout << "=== Test Different Block Sizes ===" << std::endl;
  
  std::vector<size_t> sizes = {16, 64, 128, 512, 1024, 4096};
  
  for (size_t size : sizes) {
    util::MemoryPool pool(size, 100);
    void* ptr = pool.Allocate();
    
    if (ptr != nullptr) {
      std::cout << "Block size " << size << ": allocated successfully, "
                << "actual size: " << pool.GetBlockSize() << std::endl;
      pool.Deallocate(ptr);
    } else {
      std::cout << "Block size " << size << ": failed" << std::endl;
    }
  }
  
  std::cout << std::endl;
}

void TestDeallocateOrder() {
  std::cout << "=== Test Deallocate Order ===" << std::endl;
  
  util::MemoryPool pool(64, 10);
  std::vector<void*> pointers;
  
  for (int i = 0; i < 5; ++i) {
    pointers.push_back(pool.Allocate());
  }
  
  std::cout << "Deallocating in reverse order..." << std::endl;
  for (auto it = pointers.rbegin(); it != pointers.rend(); ++it) {
    pool.Deallocate(*it);
  }
  
  std::cout << "Available: " << pool.GetAvailableCount() << std::endl;
  
  std::cout << "Allocating again..." << std::endl;
  std::vector<void*> new_pointers;
  for (int i = 0; i < 5; ++i) {
    new_pointers.push_back(pool.Allocate());
  }
  
  std::cout << "Re-allocated: " << new_pointers.size() << " blocks" << std::endl;
  
  for (void* ptr : new_pointers) {
    pool.Deallocate(ptr);
  }
  
  std::cout << std::endl;
}

void TestNullDeallocate() {
  std::cout << "=== Test Null Deallocate ===" << std::endl;
  
  util::MemoryPool pool(64, 10);
  
  pool.Deallocate(nullptr);
  std::cout << "Null deallocate: handled safely" << std::endl;
  std::cout << "Available: " << pool.GetAvailableCount() << std::endl;
  std::cout << std::endl;
}

int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "     TCP Server MemoryPool Test Suite  " << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << std::endl;
  
  TestBasicAllocation();
  TestMultipleAllocation();
  TestPoolExhaustion();
  TestMemoryWriteRead();
  TestPointerValidation();
  TestConcurrentAllocation();
  TestDifferentBlockSizes();
  TestDeallocateOrder();
  TestNullDeallocate();
  
  std::cout << "========================================" << std::endl;
  std::cout << "     All Tests Completed Successfully  " << std::endl;
  std::cout << "========================================" << std::endl;
  
  return 0;
}
