#pragma once

#include <cstdint>
#include <string>

namespace SnowOwl::Utils::SystemResources {

struct SystemInfo {
    std::string architecture;
    std::string kernel;
    std::string cpuModel;
    std::string cpuVendor;
    std::uint32_t logicalCores{0};
    std::uint32_t physicalCores{0};
    std::uint64_t memoryTotalMb{0};
    std::string gpuModel;
    std::uint64_t gpuMemoryMb{0};
    bool hasNvidiaGpu{false};
    bool hasAmdGpu{false};
    bool hasIntelGpu{false};
};

class SystemProbe {
public:
    static SystemInfo collect();
};

}