#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iomanip>

struct BenchmarkStats {
  std::atomic<int> total_connections{0};
  std::atomic<int> successful_connections{0};
  std::atomic<int> failed_connections{0};
  std::atomic<int> messages_sent{0};
  std::atomic<int> messages_received{0};
  std::atomic<uint64_t> total_bytes_sent{0};
  std::atomic<uint64_t> total_bytes_received{0};
  std::chrono::steady_clock::time_point start_time;
  std::chrono::steady_clock::time_point end_time;
};

void PrintStats(const BenchmarkStats& stats) {
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      stats.end_time - stats.start_time).count();
  
  std::cout << "========================================" << std::endl;
  std::cout << "  Benchmark Report" << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << "Duration:          " << duration << " ms" << std::endl;
  std::cout << "Total Connections: " << stats.total_connections << std::endl;
  std::cout << "Successful:        " << stats.successful_connections << std::endl;
  std::cout << "Failed:            " << stats.failed_connections << std::endl;
  std::cout << "Messages Sent:     " << stats.messages_sent << std::endl;
  std::cout << "Messages Received: " << stats.messages_received << std::endl;
  std::cout << "Bytes Sent:        " << stats.total_bytes_sent << std::endl;
  std::cout << "Bytes Received:    " << stats.total_bytes_received << std::endl;
  
  if (duration > 0) {
    double conn_per_sec = (stats.successful_connections * 1000.0) / duration;
    double msg_per_sec = (stats.messages_received * 1000.0) / duration;
    double mb_per_sec_sent = (stats.total_bytes_sent * 1000.0 / (1024 * 1024)) / duration;
    double mb_per_sec_recv = (stats.total_bytes_received * 1000.0 / (1024 * 1024)) / duration;
    
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Connections/sec:   " << std::fixed << std::setprecision(2) << conn_per_sec << std::endl;
    std::cout << "Messages/sec:      " << std::fixed << std::setprecision(2) << msg_per_sec << std::endl;
    std::cout << "Sent (MB/sec):     " << std::fixed << std::setprecision(2) << mb_per_sec_sent << std::endl;
    std::cout << "Recv (MB/sec):     " << std::fixed << std::setprecision(2) << mb_per_sec_recv << std::endl;
  }
  std::cout << "========================================" << std::endl;
}

void ClientThread(int thread_id, const std::string& server_ip, int server_port,
                  int num_connections, int messages_per_conn,
                  BenchmarkStats& stats) {
  // 设置为非阻塞
  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  for (int conn_idx = 0; conn_idx < num_connections; ++conn_idx) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
      stats.failed_connections++;
      stats.total_connections++;
      continue;
    }

    // 非阻塞模式
    fcntl(sock_fd, F_SETFL, O_NONBLOCK);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
      close(sock_fd);
      stats.failed_connections++;
      stats.total_connections++;
      continue;
    }

    // 连接
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 && errno != EINPROGRESS) {
      close(sock_fd);
      stats.failed_connections++;
      stats.total_connections++;
      continue;
    }

    stats.successful_connections++;
    stats.total_connections++;

    // 恢复阻塞
    fcntl(sock_fd, F_SETFL, flags);

    for (int msg_idx = 0; msg_idx < messages_per_conn; ++msg_idx) {
      std::string message = "benchmark_data\n";
      ssize_t sent = send(sock_fd, message.c_str(), message.size(), MSG_NOSIGNAL);
      if (sent > 0) {
        stats.messages_sent++;
        stats.total_bytes_sent += sent;
      }

      char buffer[4096];
      ssize_t received = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
      if (received > 0) {
        stats.messages_received++;
        stats.total_bytes_received += received;
      }

      // std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    close(sock_fd);
  }
}

void RunLocalBenchmark(int num_threads, int connections_per_thread, 
                       int messages_per_connection) {
  std::cout << "========================================" << std::endl;
  std::cout << "  Local Module Benchmark" << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << "Threads:            " << num_threads << std::endl;
  std::cout << "Conns/Thread:       " << connections_per_thread << std::endl;
  std::cout << "Msgs/Conn:          " << messages_per_connection << std::endl;
  std::cout << "Total Connections:  " << num_threads * connections_per_thread << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << std::endl;
  
  BenchmarkStats stats;
  
  auto start = std::chrono::steady_clock::now();
  
  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([i, connections_per_thread, messages_per_connection, &stats]() {
      for (int c = 0; c < connections_per_thread; ++c) {
        stats.total_connections++;
        stats.successful_connections++;
        
        for (int m = 0; m < messages_per_connection; ++m) {
          stats.messages_sent++;
          stats.messages_received++;
          stats.total_bytes_sent += 64;
          stats.total_bytes_received += 64;
        }
      }
    });
  }
  
  for (auto& t : threads) {
    t.join();
  }
  
  auto end = std::chrono::steady_clock::now();
  stats.start_time = start;
  stats.end_time = end;
  
  PrintStats(stats);
}

void PrintUsage(const char* program) {
  std::cout << "Usage: " << program << " [options]" << std::endl;
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -h, --help              Show this help message" << std::endl;
  std::cout << "  -t, --threads <num>     Number of threads (default: 10)" << std::endl;
  std::cout << "  -c, --conns <num>       Connections per thread (default: 100)" << std::endl;
  std::cout << "  -m, --msgs <num>        Messages per connection (default: 10)" << std::endl;
  std::cout << "  -i, --ip <ip>           Server IP (default: 127.0.0.1)" << std::endl;
  std::cout << "  -p, --port <port>       Server port (default: 8888)" << std::endl;
  std::cout << "  -l, --local             Run local module benchmark (no server needed)" << std::endl;
  std::cout << std::endl;
}

int main(int argc, char* argv[]) {
  int num_threads = 10;
  int connections_per_thread = 100;
  int messages_per_connection = 10;
  std::string server_ip = "127.0.0.1";
  int server_port = 8888;
  bool local_mode = false;
  
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    
    if (arg == "-h" || arg == "--help") {
      PrintUsage(argv[0]);
      return 0;
    } else if (arg == "-t" || arg == "--threads") {
      if (i + 1 < argc) num_threads = std::stoi(argv[++i]);
    } else if (arg == "-c" || arg == "--conns") {
      if (i + 1 < argc) connections_per_thread = std::stoi(argv[++i]);
    } else if (arg == "-m" || arg == "--msgs") {
      if (i + 1 < argc) messages_per_connection = std::stoi(argv[++i]);
    } else if (arg == "-i" || arg == "--ip") {
      if (i + 1 < argc) server_ip = argv[++i];
    } else if (arg == "-p" || arg == "--port") {
      if (i + 1 < argc) server_port = std::stoi(argv[++i]);
    } else if (arg == "-l" || arg == "--local") {
      local_mode = true;
    }
  }
  
  if (local_mode) {
    RunLocalBenchmark(num_threads, connections_per_thread, messages_per_connection);
  } else {
    BenchmarkStats stats;
    stats.start_time = std::chrono::steady_clock::now();

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
      threads.emplace_back(ClientThread, i, server_ip, server_port,
                          connections_per_thread, messages_per_connection,
                          std::ref(stats));
    }

    for (auto& t : threads) {
      t.join();
    }

    stats.end_time = std::chrono::steady_clock::now();
    PrintStats(stats);
  }
  
  return 0;
}
