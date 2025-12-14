#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>

namespace SnowOwl::Edge::Monitoring {

struct ResourceSnapshot {
	bool valid{false};
	double cpuPercent{0.0};
	double memoryPercent{0.0};
	std::uint64_t memoryTotalMb{0};
	std::uint64_t memoryUsedMb{0};
	double temperatureC{0.0};
	double gpuPercent{0.0};
	std::chrono::system_clock::time_point timestamp{};
};

class ResourceTracker {
public:
	ResourceTracker();
	~ResourceTracker();

	void start(std::chrono::milliseconds interval = std::chrono::milliseconds(1000));
	void stop();

	ResourceSnapshot latestSnapshot() const;
	ResourceSnapshot sampleNow();

private:
	void samplingLoop();
	ResourceSnapshot collect();

	struct CpuTimes {
		std::uint64_t total{0};
		std::uint64_t idle{0};
	};

	CpuTimes readCpuTimes() const;
	ResourceSnapshot collectFromSystem();

	std::chrono::milliseconds interval_;
	mutable std::mutex mutex_;
	mutable std::mutex snapshotMutex_;
	std::thread thread_;
	std::atomic<bool> running_{false};
	std::optional<CpuTimes> lastCpuTimes_;
	ResourceSnapshot snapshot_{};
};

}
