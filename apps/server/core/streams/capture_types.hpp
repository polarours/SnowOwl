#pragma once

#include <string>

namespace SnowOwl::Server::Core {

enum class CaptureSourceKind {
    Camera,
    RTMPStream,
    RTSPStream,
    NetworkStream,
    File,
    Other
};

struct CaptureSourceConfig {
    CaptureSourceKind kind{CaptureSourceKind::Camera};
    int cameraId{0};
    std::string primaryUri;
    std::string secondaryUri;
};

}
