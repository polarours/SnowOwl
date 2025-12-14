#include "web_client_launcher.hpp"
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#elif __APPLE__
#include <cstdlib>
#else
#include <cstdlib>
#endif

namespace SnowOwl::Client::Web {

bool WebClientLauncher::launch(const std::string& url) {
    std::cout << "Launching web client at: " << url << std::endl;
    
#ifdef _WIN32
    HINSTANCE result = ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    return (reinterpret_cast<INT_PTR>(result) > 32);
#elif __APPLE__
    std::string command = "open \"" + url + "\"";
    int result = system(command.c_str());
    return result == 0;
#else
    const char* browsers[] = {"xdg-open", "firefox", "google-chrome", "chromium", nullptr};
    
    for (int i = 0; browsers[i] != nullptr; ++i) {
        std::string command = std::string(browsers[i]) + " \"" + url + "\" >/dev/null 2>&1 &";
        int result = system(command.c_str());
        if (result == 0) {
            return true;
        }
    }
    
    std::cerr << "Failed to launch web browser. Please manually open: " << url << std::endl;
    return false;
#endif
}

}