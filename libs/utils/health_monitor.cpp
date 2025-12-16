#include <cmath>

#include "health_monitor.hpp"

namespace SnowOwl::Utils::SystemResources {

HealthMonitor::HealthMonitor() = default;
HealthMonitor::~HealthMonitor() = default;

void HealthMonitor::setThresholds(const HealthThresholds& thresholds) {
	std::lock_guard<std::mutex> lock(mutex_);
	thresholds_ = thresholds;
}

HealthThresholds HealthMonitor::thresholds() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return thresholds_;
}

HealthStatus HealthMonitor::evaluate(const ResourceSnapshot& snapshot) const {
	HealthThresholds currentThresholds; {
		std::lock_guard<std::mutex> lock(mutex_);
		currentThresholds = thresholds_;
	}

	HealthStatus status;
	status.snapshot = snapshot;

	if (!snapshot.valid) {
		status.healthy = false;
		status.warnings.emplace_back("The resource monitoring data is not available");
		return status;
	}

	if (snapshot.cpuPercent >= currentThresholds.maxCpuPercent) {
		status.healthy = false;
		status.warnings.emplace_back("CPU usage is too high");
	}

	if (snapshot.memoryPercent >= currentThresholds.maxMemoryPercent) {
		status.healthy = false;
		status.warnings.emplace_back("Memory usage is too high");
	}

	if (!std::isnan(snapshot.temperatureC) && snapshot.temperatureC >= currentThresholds.maxTemperatureC) {
		status.healthy = false;
		status.warnings.emplace_back("Device temperature is too high");
	}

	if (snapshot.gpuPercent >= 0.0 && snapshot.gpuPercent > 95.0) {
		status.healthy = false;
		status.warnings.emplace_back("GPU usage is too high");
	}

	return status;
}

}
