#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "modules/monitoring/resource_tracker.hpp"

namespace SnowOwl::Edge::Monitoring {

struct HealthThresholds {
	double maxCpuPercent{95.0};
	double maxMemoryPercent{95.0};
	double maxTemperatureC{90.0};
};

struct HealthStatus {
	bool healthy{true};
	std::vector<std::string> warnings;
	ResourceSnapshot snapshot;
};

class HealthMonitor {
public:
	HealthMonitor();
	~HealthMonitor();

	void setThresholds(const HealthThresholds& thresholds);
	HealthThresholds thresholds() const;

	HealthStatus evaluate(const ResourceSnapshot& snapshot) const;

private:
	HealthThresholds thresholds_;
	mutable std::mutex mutex_;
};

}
