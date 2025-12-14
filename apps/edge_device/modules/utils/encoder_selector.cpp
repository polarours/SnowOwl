#include "modules/utils/encoder_selector.hpp"

#include <algorithm>

namespace SnowOwl::Edge::Utils {

namespace {

EncoderKind guessFromPreferred(const std::string& preferred) {
	const std::string lower = [&]() {
		std::string tmp = preferred;
		std::transform(tmp.begin(), tmp.end(), tmp.begin(), [](unsigned char ch) {
			return static_cast<char>(std::tolower(ch));
		});
		return tmp;
	}();

	if (lower.find("nvenc") != std::string::npos || lower.find("cuda") != std::string::npos) {
		return EncoderKind::NvidiaNVENC;
	}
	if (lower.find("qsv") != std::string::npos || lower.find("intel") != std::string::npos) {
		return EncoderKind::IntelQSV;
	}
	if (lower.find("vaapi") != std::string::npos) {
		return EncoderKind::VAAPI;
	}
	if (lower.find("amf") != std::string::npos || lower.find("radeon") != std::string::npos) {
		return EncoderKind::AMF;
	}
	if (lower.find("videotoolbox") != std::string::npos || lower.find("apple") != std::string::npos) {
		return EncoderKind::AppleVT;
	}
	if (lower.find("software") != std::string::npos || lower.find("cpu") != std::string::npos) {
		return EncoderKind::Software;
	}
	return EncoderKind::Unknown;
}

EncoderChoice makeChoice(EncoderKind kind) {
	EncoderChoice choice;
	choice.kind = kind;
	choice.supportsFp16 = false;

	switch (kind) {
		case EncoderKind::NvidiaNVENC:
			choice.name = "nvenc";
			choice.codecPriority = {"h264_nvenc", "hevc_nvenc"};
			choice.supportsFp16 = true;
			break;
		case EncoderKind::IntelQSV:
			choice.name = "qsv";
			choice.codecPriority = {"h264_qsv", "hevc_qsv"};
			break;
		case EncoderKind::VAAPI:
			choice.name = "vaapi";
			choice.codecPriority = {"h264_vaapi", "hevc_vaapi"};
			break;
		case EncoderKind::AMF:
			choice.name = "amf";
			choice.codecPriority = {"h264_amf", "hevc_amf"};
			break;
		case EncoderKind::AppleVT:
			choice.name = "videotoolbox";
			choice.codecPriority = {"h264_videotoolbox", "hevc_videotoolbox"};
			break;
		case EncoderKind::Software:
		case EncoderKind::Unknown:
			choice.name = "software";
			choice.codecPriority = {"libx264", "libx265"};
			break;
	}

	return choice;
}

}

EncoderChoice EncoderSelector::select(const Config::DeviceProfile& profile, const std::string& preferred) const {
	EncoderKind kind = EncoderKind::Unknown;

	if (!preferred.empty()) {
		kind = guessFromPreferred(preferred);
	}

	if (kind == EncoderKind::Unknown) {
		switch (profile.computeTier) {
			case Config::ComputeTier::FullInference:
				kind = profile.hasDiscreteGpu ? EncoderKind::NvidiaNVENC : EncoderKind::IntelQSV;
				break;
			case Config::ComputeTier::LightweightInference:
				kind = profile.hasDiscreteGpu ? EncoderKind::IntelQSV : EncoderKind::VAAPI;
				break;
			case Config::ComputeTier::CaptureOnly:
				kind = EncoderKind::Software;
				break;
		}
	}

	if (kind == EncoderKind::Unknown) {
		kind = profile.hasDiscreteGpu ? EncoderKind::NvidiaNVENC : EncoderKind::Software;
	}

	auto choice = makeChoice(kind);
	if (profile.supportsFp16) {
		choice.supportsFp16 = true;
	}

	return choice;
}

std::string EncoderSelector::toString(EncoderKind kind) {
	switch (kind) {
		case EncoderKind::Software:
			return "software";
		case EncoderKind::NvidiaNVENC:
			return "nvenc";
		case EncoderKind::IntelQSV:
			return "qsv";
		case EncoderKind::VAAPI:
			return "vaapi";
		case EncoderKind::AMF:
			return "amf";
		case EncoderKind::AppleVT:
			return "videotoolbox";
		case EncoderKind::Unknown:
			return "unknown";
	}
	return "unknown";
}

}
