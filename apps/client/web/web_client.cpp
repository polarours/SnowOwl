
#include <curl/curl.h>
#include <iostream>
#include <thread>
#include <chrono>

#include "web_client.hpp"
#include "web_client_launcher.hpp"

namespace SnowOwl::Client::Web {

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t totalSize = size * nmemb;
    response->append((char*)contents, totalSize);
    return totalSize;
}

WebClient::WebClient(ClientState& state) : state_(state) {}

bool WebClient::checkServer() {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }
    
    std::string response;
    std::string url = state_.getServerUrl() + "/api/v1/status";
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    
    CURLcode res = curl_easy_perform(curl);
    long responseCode = 0;
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    }
    
    curl_easy_cleanup(curl);
    
    return (res == CURLE_OK && responseCode == 200);
}

bool WebClient::launch() {
    std::cout << "Checking server connectivity..." << std::endl;
    
    int attempts = 0;
    const int maxAttempts = 10;
    
    while (attempts < maxAttempts) {
        if (checkServer()) {
            std::cout << "Server is reachable, launching web client..." << std::endl;
            return WebClientLauncher::launch(state_.getServerUrl());
        }
        
        std::cout << "Server not reachable, retrying in 2 seconds... (" 
                  << (attempts + 1) << "/" << maxAttempts << ")" << std::endl;
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
        attempts++;
    }
    
    std::cerr << "Unable to connect to server after " << maxAttempts << " attempts." << std::endl;
    std::cerr << "Please make sure the SnowOwl server is running at: " << state_.getServerUrl() << std::endl;
    
    return WebClientLauncher::launch(state_.getServerUrl());
}

}