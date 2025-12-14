#include "modules/monitoring/system_probe.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#include <sys/utsname.h>

namespace SnowOwl::Edge::Monitoring {

namespace {

std::string trim(std::string value) {
    const auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](unsigned char ch) { return !isSpace(ch); }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [&](unsigned char ch) { return !isSpace(ch); }).base(), value.end());
    return value;
}

struct GpuVendor {
    bool nvidia{false};
    bool amd{false};
    bool intel{false};
    bool apple{false};
};

#if defined(__linux__)
struct MemInfo {
    std::uint64_t total{0};
};

MemInfo readMemInfo() {
    MemInfo info;
    std::ifstream stream("/proc/meminfo");
    if (!stream.is_open()) {
        return info;
    }

    std::string key;
    std::uint64_t value = 0;
    std::string unit;
    while (stream >> key >> value >> unit) {
        if (key == "MemTotal:") {
            info.total = value * 1024ULL;
            break;
        }
    }
    return info;
}
#endif

GpuVendor detectGpuVendors() {
    GpuVendor result;
#if defined(__linux__)
    const std::string drmPath = "/sys/class/drm";
    std::error_code ec;
    if (std::filesystem::exists(drmPath, ec)) {
        for (const auto& entry : std::filesystem::directory_iterator(drmPath, ec)) {
            if (!entry.is_directory()) {
                continue;
            }

            const auto vendorPath = entry.path() / "device" / "vendor";
            std::ifstream vendorStream(vendorPath);
            if (!vendorStream.is_open()) {
                continue;
            }

            std::string vendor;
            vendorStream >> vendor;
            if (vendor == "0x10de") {
                result.nvidia = true;
            } else if (vendor == "0x1002" || vendor == "0x1022") {
                result.amd = true;
            } else if (vendor == "0x8086") {
                result.intel = true;
            }
        }
    }
#endif
    return result;
}


} // namespace



SystemInfo SystemProbe::collect() {
    SystemInfo info;

    if (struct utsname uts; uname(&uts) == 0) {
        info.architecture = uts.machine;
        info.kernel = uts.release;
    }

    info.logicalCores = static_cast<std::uint32_t>(std::thread::hardware_concurrency());

#if defined(__linux__)
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    while (std::getline(cpuinfo, line)) {
        const auto colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }

        const std::string key = trim(line.substr(0, colonPos));
        const std::string value = trim(line.substr(colonPos + 1));

        if (key == "model name" && info.cpuModel.empty()) {
            info.cpuModel = value;
        } else if (key == "Hardware" && info.cpuModel.empty()) {
            info.cpuModel = value;
        } else if (key == "vendor_id" && info.cpuVendor.empty()) {
            info.cpuVendor = value;
        } else if (key == "Processor" && info.cpuVendor.empty()) {
            info.cpuVendor = value;
        } else if (key == "cpu cores" && info.physicalCores == 0) {
            info.physicalCores = static_cast<std::uint32_t>(std::stoul(value));
        }
    }
#endif

    if (info.cpuVendor.empty()) {
        info.cpuVendor = "unknown";
    }
    if (info.cpuModel.empty()) {
        info.cpuModel = info.cpuVendor;
    }
    if (info.physicalCores == 0) {
        info.physicalCores = info.logicalCores;
    }

#if defined(__linux__)
    const MemInfo memInfo = readMemInfo();
    if (memInfo.total > 0) {
        info.memoryTotalMb = memInfo.total / (1024ULL * 1024ULL);
    }
#endif

    const GpuVendor vendors = detectGpuVendors();
    info.hasNvidiaGpu = vendors.nvidia;
    info.hasAmdGpu = vendors.amd;
    info.hasIntelGpu = vendors.intel;

    return info;
}

}
