#pragma once

#include <string>
#include <vector>
#include <memory>

namespace SnowOwl {

class PluginInterface {
public:
    virtual ~PluginInterface() = default;

    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;
    virtual std::string getType() const = 0; 

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
};

class ServerPluginInterface : public PluginInterface {
public:
    virtual void onServerStart() {}
    virtual void onServerStop() {}
};

class EdgePluginInterface : public PluginInterface {
public:
    virtual void onEdgeStart() {}
    virtual void onEdgeStop() {}
};

class ClientPluginInterface : public PluginInterface {
public:
    virtual void onClientStart() {}
    virtual void onClientStop() {}
};

}
