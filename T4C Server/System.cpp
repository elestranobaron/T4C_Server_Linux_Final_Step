#include "System.h"
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>

std::string System::GetMachineName() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == -1) {
        std::cerr << "Error getting hostname: " << strerror(errno) << std::endl;
        return "unknown";
    }
    return std::string(hostname);
}
