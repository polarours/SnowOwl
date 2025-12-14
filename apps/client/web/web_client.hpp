#pragma once

#include <string>

#include "core/client_state.hpp"
#include "web_client_launcher.hpp"

namespace SnowOwl::Client::Web {

using SnowOwl::Client::Core::ClientState;

class WebClient {
public:
    WebClient(ClientState& state);
    
    bool launch();
    
    bool checkServer();
    
private:
    ClientState& state_;
};

}
