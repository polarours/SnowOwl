#pragma once

#include <string>
#include <vector>

#include "modules/config/device_profile.hpp"

namespace SnowOwl::Edge::Utils {

enum class EncoderKind {
	Software,
	NvidiaNVENC,
	IntelQSV,
	VAAPI,
	AMF,
	AppleVT,
	Unknown
};

struct EncoderChoice {
	EncoderKind kind{EncoderKind::Software};
	std::string name{"software"};
	std::vector<std::string> codecPriority{"libx264", "libx265"};
	bool supportsFp16{false};
};

class EncoderSelector {
public:
	EncoderSelector() = default;

	EncoderChoice select(const Config::DeviceProfile& profile, const std::string& preferred = {}) const;
	static std::string toString(EncoderKind kind);
};

}
