#pragma once

#include <mutex>
#include <string>

#include "modules/config/device_profile.hpp"
#include "libs/utils/health_monitor.hpp"

namespace SnowOwl::Edge::Utils {
class HealthMonitor;
class HealthStatus;
}

namespace SnowOwl::Edge::Utils {

enum class PowerMode {
	PowerSave,
	Balanced,
	Performance
};

struct PowerPolicy {
	PowerMode mode{PowerMode::Balanced};
	bool allowFp16{false};
	bool allowGpuBoost{false};
	bool preferLowPowerEncoders{true};

	static PowerPolicy fromProfile(const Config::DeviceProfile& profile);
	static std::string toString(PowerMode mode);
};

class PowerManager {
public:
	PowerManager() = default;

	void applyPolicy(const PowerPolicy& policy);
	PowerPolicy currentPolicy() const;

	void onHealthUpdate(const HealthStatus& status);

private:
	void logTransition(const PowerPolicy& from, const PowerPolicy& to) const;

	mutable std::mutex mutex_;
	PowerPolicy policy_{};
};

}
