#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "utils/app_paths.hpp"

#if defined(_WIN32)
#include <ShlObj.h>
#include <combaseapi.h>
#elif defined(__APPLE__)
#include <sys/types.h>
#include <sys/stat.h>
#elif defined(__linux__)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#else
#endif

namespace SnowOwl::Utils::Paths {

namespace {

namespace fs = std::filesystem;

fs::path ensureDirectory(const fs::path& dir) {
    if (dir.empty()) {
        return dir;
    }

    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        std::cerr << "SnowOwl: failed to create directory '" << dir << "': " << ec.message() << std::endl;
    }
    return dir;
}

fs::path resolvePlatformRoot() {
    if (const char* envHome = std::getenv("ARCTICOWL_HOME")) {
        if (*envHome) {
            return ensureDirectory(fs::path(envHome));
        }
    }

#if defined(_WIN32)
    PWSTR rawPath = nullptr;
    fs::path base;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &rawPath))) {
        base = fs::path(rawPath);
        CoTaskMemFree(rawPath);
    } else if (const char* appData = std::getenv("APPDATA")) {
        base = fs::path(appData);
    }

    if (!base.empty()) {
        return ensureDirectory(base / "SnowOwl");
    }

    if (const char* home = std::getenv("USERPROFILE")) {
        return ensureDirectory(fs::path(home) / "AppData" / "Roaming" / "SnowOwl");
    }

    return ensureDirectory(fs::current_path() / "SnowOwl");
#elif defined(__APPLE__)
    if (const char* home = std::getenv("HOME")) {
        return ensureDirectory(fs::path(home) / "Library" / "Application Support" / "SnowOwl");
    }
    return ensureDirectory(fs::current_path() / "Library" / "Application Support" / "SnowOwl");
#elif defined(__linux__)
    if (const char* home = std::getenv("HOME")) {
        if (const char* xdgConfigHome = std::getenv("XDG_CONFIG_HOME")) {
            return ensureDirectory(fs::path(xdgConfigHome) / "snowowl");
        }
        return ensureDirectory(fs::path(home) / ".config" / "snowowl");
    }
    return ensureDirectory(fs::current_path() / ".config" / "snowowl");
#else
    return ensureDirectory(fs::current_path() / "SnowOwl");
#endif
}

} 

std::filesystem::path dataRoot() {
    static const std::filesystem::path root = resolvePlatformRoot();
    return root;
}

std::filesystem::path configFile(const std::string& fileName) {
    const auto root = dataRoot();
    const auto target = root / fileName;

    if (const auto parent = target.parent_path(); !parent.empty()) {
        ensureDirectory(parent);
    }

    return target;
}

}
