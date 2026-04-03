#include <iostream>
#include <ctime>
#include <iomanip>

int main() {
  std::time_t now = std::time(nullptr);
  
  std::cout << "=== Time Test ===" << std::endl;
  
  std::cout << "Unix time: " << now << std::endl;
  
  std::tm* local_tm = std::localtime(&now);
  char local_buffer[80];
  std::strftime(local_buffer, sizeof(local_buffer), "%Y-%m-%d %H:%M:%S %Z", local_tm);
  std::cout << "Local time: " << local_buffer << std::endl;
  
  std::tm* utc_tm = std::gmtime(&now);
  char utc_buffer[80];
  std::strftime(utc_buffer, sizeof(utc_buffer), "%Y-%m-%d %H:%M:%S %Z", utc_tm);
  std::cout << "UTC time:   " << utc_buffer << std::endl;
  
  return 0;
}
