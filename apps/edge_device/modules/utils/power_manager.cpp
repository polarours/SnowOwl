#include "modules/utils/power_manager.hpp"

#include <iostream>

namespace SnowOwl::Edge::Utils {

PowerPolicy PowerPolicy::fromProfile(const Config::DeviceProfile& profile) {
	PowerPolicy policy;

	switch (profile.computeTier) {
		case Config::ComputeTier::CaptureOnly:
			policy.mode = PowerMode::PowerSave;
			policy.preferLowPowerEncoders = true;
			break;
		case Config::ComputeTier::LightweightInference:
			policy.mode = PowerMode::Balanced;
			policy.preferLowPowerEncoders = !profile.hasDiscreteGpu;
			break;
		case Config::ComputeTier::FullInference:
			policy.mode = PowerMode::Performance;
			policy.preferLowPowerEncoders = false;
			break;
	}

	policy.allowGpuBoost = profile.hasDiscreteGpu;
	policy.allowFp16 = profile.supportsFp16;
	return policy;
}

std::string PowerPolicy::toString(PowerMode mode) {
	switch (mode) {
		case PowerMode::PowerSave:
			return "power save";
		case PowerMode::Balanced:
			return "balanced";
		case PowerMode::Performance:
			return "performance";
	}
	return "balanced";
}

void PowerManager::applyPolicy(const PowerPolicy& policy) {
	std::lock_guard<std::mutex> lock(mutex_);
	if (policy.mode == policy_.mode && policy.allowFp16 == policy_.allowFp16 &&
		policy.allowGpuBoost == policy_.allowGpuBoost && policy.preferLowPowerEncoders == policy_.preferLowPowerEncoders) {
		return;
	}

	logTransition(policy_, policy);
	policy_ = policy;
}

PowerPolicy PowerManager::currentPolicy() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return policy_;
}

void PowerManager::onHealthUpdate(const Monitoring::HealthStatus& status) {
	if (!status.snapshot.valid) {
		return;
	}

	std::lock_guard<std::mutex> lock(mutex_);
	PowerPolicy updated = policy_;

	if (!status.healthy) {
		updated.mode = PowerMode::PowerSave;
		updated.preferLowPowerEncoders = true;
	}else if (status.snapshot.cpuPercent < 75.0 && status.snapshot.memoryPercent < 75.0) {
		updated.mode = PowerMode::Balanced;
		updated.preferLowPowerEncoders = false;
	}else {
		updated.mode = PowerMode::Performance;
		updated.preferLowPowerEncoders = false;
	}

	if (updated.mode != policy_.mode ||
		updated.preferLowPowerEncoders != policy_.preferLowPowerEncoders) {
		logTransition(policy_, updated);
		policy_ = updated;
	}
}

void PowerManager::logTransition(const PowerPolicy& from, const PowerPolicy& to) const {
	std::cout << "PowerManager: policy "
			  << PowerPolicy::toString(from.mode) << " -> " << PowerPolicy::toString(to.mode)
			  << ", gpu_boost=" << (to.allowGpuBoost ? "on" : "off")
			  << ", fp16=" << (to.allowFp16 ? "on" : "off")
			  << ", prefer_low_power_encoders=" << (to.preferLowPowerEncoders ? "on" : "off")
			  << std::endl;
}

}
