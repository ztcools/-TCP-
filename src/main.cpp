#include <iostream>
#include "net/server.h"
#include "util/log.h"

void PrintUsage(const char* program) {
  std::cout << "Usage: " << program << " [options]" << std::endl;
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -h, --help              Show this help message" << std::endl;
  std::cout << "  -p, --port <port>       Listen port (default: 8888)" << std::endl;
  std::cout << "  -i, --ip <ip>           Listen IP (default: 0.0.0.0)" << std::endl;
  std::cout << "  -b, --backlog <num>     Listen backlog (default: 128)" << std::endl;
  std::cout << "  -c, --max-conn <num>    Max connections (default: 100000)" << std::endl;
  std::cout << "  -t, --timeout <ms>      Heartbeat timeout (default: 60000)" << std::endl;
  std::cout << "  -w, --workers <num>     Worker threads (default: auto)" << std::endl;
  std::cout << "  -v, --verbose           Enable verbose logging" << std::endl;
  std::cout << std::endl;
}

int main(int argc, char* argv[]) {
  net::Server::Config config;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      PrintUsage(argv[0]);
      return 0;
    } else if (arg == "-p" || arg == "--port") {
      if (i + 1 < argc) {
        config.port = static_cast<uint16_t>(std::stoi(argv[++i]));
      }
    } else if (arg == "-i" || arg == "--ip") {
      if (i + 1 < argc) {
        config.ip = argv[++i];
      }
    } else if (arg == "-b" || arg == "--backlog") {
      if (i + 1 < argc) {
        config.backlog = std::stoi(argv[++i]);
      }
    } else if (arg == "-c" || arg == "--max-conn") {
      if (i + 1 < argc) {
        config.max_connections = std::stoull(argv[++i]);
      }
    } else if (arg == "-t" || arg == "--timeout") {
      if (i + 1 < argc) {
        config.heartbeat_timeout_ms = std::stoll(argv[++i]);
      }
    } else if (arg == "-w" || arg == "--workers") {
      if (i + 1 < argc) {
        config.thread_pool_size = std::stoull(argv[++i]);
      }
    } else if (arg == "-v" || arg == "--verbose") {
      config.log_level = util::LogLevel::DEBUG;
    }
  }

  auto& server = net::Server::GetInstance();
  server.Init(config);
  server.Start();
  server.Wait();

  return 0;
}
