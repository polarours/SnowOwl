#include "flutter_client.hpp"
#include <iostream>
#include <string>

namespace SnowOwl::Client::Flutter {

FlutterClient::FlutterClient(ClientState& state) : state_(state) {}

int FlutterClient::launch(const std::string& device) {
    const std::string projectRoot = "/home/polarours/Projects/Personal/SnowOwl";
    const std::string flutterProjectPath = projectRoot + "/frontend/flutter/snowowl_app";
    
    std::string checkDirCommand = "test -d \"" + flutterProjectPath + "\"";
    int dirCheck = system(checkDirCommand.c_str());
    if (dirCheck != 0) {
        std::cerr << "Error: Flutter project not found in " << flutterProjectPath << std::endl;
        return 1;
    }
    
    std::cout << "Getting Flutter dependencies..." << std::endl;
    std::string flutterPubGetCommand = "cd \"" + flutterProjectPath + "\" && flutter pub get";
    int pubGetResult = system(flutterPubGetCommand.c_str());
    if (pubGetResult != 0) {
        std::cerr << "Error: Failed to get Flutter dependencies" << std::endl;
        return 1;
    }

    std::string flutterCommand = "cd \"" + flutterProjectPath + "\" && flutter run";
    if (!device.empty()) {
        flutterCommand += " -d \"" + device + "\"";
    }
    
    std::cout << "Launching Flutter client..." << std::endl;
    int result = system(flutterCommand.c_str());
    return result == 0 ? 0 : 1;
}

}