#include "modules/monitoring/resource_tracker.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace {

double safePercentage(std::uint64_t used, std::uint64_t total) {
	if (total == 0) {
		return 0.0;
	}
	const double ratio = static_cast<double>(used) / static_cast<double>(total);
	return std::clamp(ratio * 100.0, 0.0, 100.0);
}

double readTemperatureC() {
#if defined(__linux__)
	const std::vector<std::string> candidates = {
		"/sys/class/thermal/thermal_zone0/temp",
		"/sys/class/thermal/thermal_zone1/temp",
		"/sys/class/hwmon/hwmon0/temp1_input"
	};

	for (const auto& path : candidates) {
		std::ifstream stream(path);
		if (!stream.is_open()) {
			continue;
		}

		double raw = 0.0;
		stream >> raw;
		if (!stream.fail()) {
			if (raw > 1000.0) {
				raw /= 1000.0;
			}
			return raw;
		}
	}
#endif
	return std::numeric_limits<double>::quiet_NaN();
}

#if defined(__linux__)
struct MemInfo {
	std::uint64_t total{0};
	std::uint64_t available{0};
};

MemInfo readMemInfo() {
	std::ifstream stream("/proc/meminfo");
	MemInfo info;
	if (!stream.is_open()) {
		return info;
	}

	std::string key;
	std::uint64_t value = 0;
	std::string unit;
	while (stream >> key >> value >> unit) {
		if (key == "MemTotal:") {
			info.total = value * 1024ULL;
		} else if (key == "MemAvailable:") {
			info.available = value * 1024ULL;
		}

		if (info.total != 0 && info.available != 0) {
			break;
		}
	}
	return info;
}
#endif

} // namespace

namespace SnowOwl::Edge::Monitoring {

ResourceTracker::ResourceTracker() = default;

ResourceTracker::~ResourceTracker() {
	stop();
}

void ResourceTracker::start(std::chrono::milliseconds interval) {
	std::lock_guard<std::mutex> lock(mutex_);
	if (running_.exchange(true)) {
		interval_ = interval;
		return;
	}

	interval_ = interval;
	thread_ = std::thread(&ResourceTracker::samplingLoop, this);
}

void ResourceTracker::stop() {
	if (!running_.exchange(false)) {
		return;
	}

	if (thread_.joinable()) {
		thread_.join();
	}
}

ResourceSnapshot ResourceTracker::latestSnapshot() const {
	std::lock_guard<std::mutex> lock(snapshotMutex_);
	return snapshot_;
}

ResourceSnapshot ResourceTracker::sampleNow() {
	const auto snapshot = collect(); {
		std::lock_guard<std::mutex> lock(snapshotMutex_);
		snapshot_ = snapshot;
	}
	return snapshot;
}

void ResourceTracker::samplingLoop() {
	while (running_.load()) {
		const auto snapshot = collect(); {
			std::lock_guard<std::mutex> lock(snapshotMutex_);
			snapshot_ = snapshot;
		}

		std::this_thread::sleep_for(interval_);
	}
}

ResourceSnapshot ResourceTracker::collect() {
	ResourceSnapshot snapshot = collectFromSystem();
	if (!snapshot.valid) {
		snapshot.timestamp = std::chrono::system_clock::now();
	}
	return snapshot;
}

ResourceTracker::CpuTimes ResourceTracker::readCpuTimes() const {
	CpuTimes times{};
#if defined(__linux__)
	std::ifstream stream("/proc/stat");
	if (!stream.is_open()) {
		return times;
	}

	std::string cpuLabel;
	std::uint64_t user = 0, nice = 0, system = 0, idle = 0;
	std::uint64_t iowait = 0, irq = 0, softirq = 0, steal = 0;
	stream >> cpuLabel >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
	if (cpuLabel.rfind("cpu", 0) == 0) {
		const std::uint64_t idleTotal = idle + iowait;
		const std::uint64_t nonIdle = user + nice + system + irq + softirq + steal;
		times.total = idleTotal + nonIdle;
		times.idle = idleTotal;
	}
#endif
	return times;
}

ResourceSnapshot ResourceTracker::collectFromSystem() {
	ResourceSnapshot snapshot;
#if defined(__linux__)
	const CpuTimes current = readCpuTimes();
	if (current.total == 0) {
		return snapshot;
	}

	double cpuPercent = 0.0;
	if (lastCpuTimes_.has_value() && current.total > lastCpuTimes_->total) {
		const auto totalDiff = static_cast<double>(current.total - lastCpuTimes_->total);
		const auto idleDiff = static_cast<double>(current.idle - lastCpuTimes_->idle);
		const double busy = totalDiff - idleDiff;
		cpuPercent = busy > 0.0 ? std::clamp((busy / totalDiff) * 100.0, 0.0, 100.0) : 0.0;
	}
	lastCpuTimes_ = current;

	const MemInfo memInfo = readMemInfo();
	std::uint64_t memoryTotalMb = memInfo.total / (1024ULL * 1024ULL);
	std::uint64_t memoryUsedMb = 0;
	if (memInfo.total > 0 && memInfo.available <= memInfo.total) {
		memoryUsedMb = (memInfo.total - memInfo.available) / (1024ULL * 1024ULL);
	}

	const double memoryPercent = memInfo.total > 0 ? safePercentage(memInfo.total - memInfo.available, memInfo.total) : 0.0;

	snapshot.valid = true;
	snapshot.cpuPercent = cpuPercent;
	snapshot.memoryPercent = memoryPercent;
	snapshot.memoryTotalMb = memoryTotalMb;
	snapshot.memoryUsedMb = memoryUsedMb;
	snapshot.temperatureC = readTemperatureC();
	snapshot.gpuPercent = -1.0;
	snapshot.timestamp = std::chrono::system_clock::now();
#else
	(void)this;
#endif
	return snapshot;
}

} // namespace SnowOwl::Edge::Monitoring