#include "server_utils.hpp"
#include <vector>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#else
#include <unistd.h>
#endif

namespace {

std::vector<std::string> g_commandLineArgs;

}

void storeCommandLineArguments(int argc, char* argv[]) {
    g_commandLineArgs.clear();
    for (int i = 0; i < argc; ++i) {
        g_commandLineArgs.emplace_back(argv[i]);
    }
}

std::vector<std::string> getCommandLineArguments() {
    return g_commandLineArgs;
}

std::string serverHostName()
{
#if defined(_WIN32)
    char buffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameA(buffer, &size) != 0 && size > 0) {
        return std::string(buffer, size);
    }
    return "windows";
#elif defined(__APPLE__)
    char buffer[256];
    std::size_t size = sizeof(buffer);
    if (sysctlbyname("kern.hostname", buffer, &size, nullptr, 0) == 0 && size > 0) {
        return std::string(buffer, size - 1);
    }
    return "macos";
#elif defined(__linux__)
    char buffer[256];
    if (gethostname(buffer, sizeof(buffer) - 1) == 0) {
        buffer[sizeof(buffer) - 1] = '\0';
        return std::string(buffer);
    }
    return "linux";
#else
    return "unknown";
#endif
}