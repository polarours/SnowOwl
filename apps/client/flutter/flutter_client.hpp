#pragma once

#include <string>
#include "core/client_state.hpp"

namespace SnowOwl::Client::Flutter {

using SnowOwl::Client::Core::ClientState;

class FlutterClient {
public:
    FlutterClient(ClientState& state);
    
    int launch(const std::string& device = "");
    
private:
    ClientState& state_;
};

}