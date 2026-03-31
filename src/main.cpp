#include <iostream>
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t g_running = 1;

void signalHandler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        std::cout << "Received signal " << sig << ", shutting down..." << std::endl;
        g_running = 0;
    }
}

int main() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    std::cout << "TCP Server starting..." << std::endl;

    while (g_running) {
        sleep(1);
    }
    std::cout << "TCP Server stopped." << std::endl;
    return 0;
}
