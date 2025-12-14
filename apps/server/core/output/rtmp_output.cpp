#include <iostream>
#include <mutex>
#include <opencv2/imgproc.hpp>

#include "core/output/rtmp_output.hpp"

namespace SnowOwl::Server::Core {

namespace {

constexpr int kDefaultBitrateKbps = 2500;
constexpr int kDefaultGop = 60;

void safeReleasePacket(AVPacket*& packet) {
	if (packet) {
		av_packet_free(&packet);
		packet = nullptr;
	}
}

void safeReleaseFrame(AVFrame*& frame) {
	if (frame) {
		av_frame_free(&frame);
		frame = nullptr;
	}
}

void safeReleaseSws(SwsContext*& ctx) {
	if (ctx) {
		sws_freeContext(ctx);
		ctx = nullptr;
	}
}

}

RtmpOutput::RtmpOutput(StreamOutputConfig config)
	: config_(std::move(config))
{
	if (auto it = config_.parameters.find("url"); it != config_.parameters.end()) {
		rtmpUrl_ = it->second;
	}
}

RtmpOutput::~RtmpOutput() {
	stop();
}

bool RtmpOutput::start() {

	if (started_.load()) {
		return true;
	}

	if (rtmpUrl_.empty()) {
		std::cerr << "RtmpOutput: missing RTMP url in configuration" << std::endl;
		return false;
	}

    static std::once_flag networkInitFlag;
    std::call_once(networkInitFlag, []() {
        avformat_network_init();
    });

	started_ = true;
	return true;
}

void RtmpOutput::stop() {
	if (!started_.exchange(false)) {
		return;
	}

	std::lock_guard<std::mutex> lock(mutex_);
	flushEncoder();
	closeOutput();
}

void RtmpOutput::publishFrame(const cv::Mat& frame) {
	if (!started_.load()) {
		return;
	}

	if (frame.empty()) {
		return;
	}

	std::lock_guard<std::mutex> lock(mutex_);
	if (!ensureInitialized(frame)) {
		return;
	}

	encodeFrame(frame);
}

void RtmpOutput::publishEvents(const std::vector<Detection::DetectionResult>&) {

}

bool RtmpOutput::ensureInitialized(const cv::Mat& frame) {
	if (initialized_.load()) {
		return true;
	}

	if (!openOutput(frame.cols, frame.rows)) {
		std::cerr << "RtmpOutput: failed to initialise output" << std::endl;
		return false;
	}

	initialized_ = true;
	return true;
}

bool RtmpOutput::openOutput(int width, int height) {
	closeOutput();

	// 1. Allocate format context and set up RTMP output
	AVFormatContext* formatCtx = nullptr;
	if (avformat_alloc_output_context2(&formatCtx, nullptr, "flv", rtmpUrl_.c_str()) < 0 || !formatCtx) {
		std::cerr << "RtmpOutput: failed to allocate output context" << std::endl;
		return false;
	}

	// 2. Configure codec and stream
	const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec) {
		std::cerr << "RtmpOutput: H264 encoder not found" << std::endl;
		avformat_free_context(formatCtx);
		return false;
	}

	AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
	if (!codecCtx) {
		std::cerr << "RtmpOutput: failed to allocate codec context" << std::endl;
		avformat_free_context(formatCtx);
		return false;
	}

	codecCtx->codec_id = AV_CODEC_ID_H264;
	codecCtx->width = width;
	codecCtx->height = height;
	codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	codecCtx->time_base = AVRational{fpsDenominator_, fpsNumerator_};
	codecCtx->framerate = AVRational{fpsNumerator_, fpsDenominator_};
	codecCtx->gop_size = kDefaultGop;
	codecCtx->max_b_frames = 0;
	codecCtx->bit_rate = static_cast<long long>(kDefaultBitrateKbps) * 1000LL;

	if (codec->id == AV_CODEC_ID_H264) {
		av_opt_set(codecCtx->priv_data, "preset", "veryfast", 0);
		av_opt_set(codecCtx->priv_data, "tune", "zerolatency", 0);
	}

	if (formatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
		codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
		std::cerr << "RtmpOutput: failed to open codec" << std::endl;
		avcodec_free_context(&codecCtx);
		avformat_free_context(formatCtx);
		return false;
	}

	AVStream* stream = avformat_new_stream(formatCtx, nullptr);
	if (!stream) {
		std::cerr << "RtmpOutput: failed to create stream" << std::endl;
		avcodec_free_context(&codecCtx);
		avformat_free_context(formatCtx);
		return false;
	}

	stream->time_base = codecCtx->time_base;
	if (avcodec_parameters_from_context(stream->codecpar, codecCtx) < 0) {
		std::cerr << "RtmpOutput: failed to derive codec parameters" << std::endl;
		avcodec_free_context(&codecCtx);
		avformat_free_context(formatCtx);
		return false;
	}

	// Important: connect to RTMP server here (mediamtx)
	if (!(formatCtx->oformat->flags & AVFMT_NOFILE)) {
		if (avio_open(&formatCtx->pb, rtmpUrl_.c_str(), AVIO_FLAG_WRITE) < 0) {
			std::cerr << "RtmpOutput: failed to open RTMP url " << rtmpUrl_ << std::endl;
			avcodec_free_context(&codecCtx);
			avformat_free_context(formatCtx);
			return false;
		}
	}

	if (avformat_write_header(formatCtx, nullptr) < 0) {
		std::cerr << "RtmpOutput: failed to write stream header" << std::endl;
		if (formatCtx->pb) {
			avio_closep(&formatCtx->pb);
		}
		avcodec_free_context(&codecCtx);
		avformat_free_context(formatCtx);
		return false;
	}

	SwsContext* swsCtx = sws_getContext(width,
			height,
			AV_PIX_FMT_BGR24,
			width,
			height,
			AV_PIX_FMT_YUV420P,
			SWS_BILINEAR,
			nullptr,
			nullptr,
			nullptr);
	if (!swsCtx) {
		std::cerr << "RtmpOutput: failed to create swscale context" << std::endl;
		av_write_trailer(formatCtx);
		if (formatCtx->pb) {
			avio_closep(&formatCtx->pb);
		}
		avcodec_free_context(&codecCtx);
		avformat_free_context(formatCtx);
		return false;
	}

	AVFrame* frame = av_frame_alloc();
	if (!frame) {
		std::cerr << "RtmpOutput: failed to allocate frame" << std::endl;
		safeReleaseSws(swsCtx);
		av_write_trailer(formatCtx);
		if (formatCtx->pb) {
			avio_closep(&formatCtx->pb);
		}
		avcodec_free_context(&codecCtx);
		avformat_free_context(formatCtx);
		return false;
	}

	frame->format = codecCtx->pix_fmt;
	frame->width = codecCtx->width;
	frame->height = codecCtx->height;

	if (av_frame_get_buffer(frame, 32) < 0) {
		std::cerr << "RtmpOutput: failed to allocate frame buffer" << std::endl;
		safeReleaseFrame(frame);
		safeReleaseSws(swsCtx);
		av_write_trailer(formatCtx);
		if (formatCtx->pb) {
			avio_closep(&formatCtx->pb);
		}
		avcodec_free_context(&codecCtx);
		avformat_free_context(formatCtx);
		return false;
	}

	AVPacket* packet = av_packet_alloc();
	if (!packet) {
		std::cerr << "RtmpOutput: failed to allocate packet" << std::endl;
		safeReleaseFrame(frame);
		safeReleaseSws(swsCtx);
		av_write_trailer(formatCtx);
		if (formatCtx->pb) {
			avio_closep(&formatCtx->pb);
		}
		avcodec_free_context(&codecCtx);
		avformat_free_context(formatCtx);
		return false;
	}

	formatCtx_ = formatCtx;
	codecCtx_ = codecCtx;
	videoStream_ = stream;
	swsCtx_ = swsCtx;
	frame_ = frame;
	packet_ = packet;
	pts_ = 0;

	return true;
}

void RtmpOutput::closeOutput() {
	if (!formatCtx_) {
		return;
	}

	if (codecCtx_) {
		flushEncoder();
	}

	if (formatCtx_) {
		av_write_trailer(formatCtx_);
		if (formatCtx_->pb) {
			avio_closep(&formatCtx_->pb);
		}
		avformat_free_context(formatCtx_);
		formatCtx_ = nullptr;
	}

	if (codecCtx_) {
		avcodec_free_context(&codecCtx_);
		codecCtx_ = nullptr;
	}

	safeReleasePacket(packet_);
	safeReleaseFrame(frame_);
	safeReleaseSws(swsCtx_);
	videoStream_ = nullptr;
	initialized_ = false;
}

bool RtmpOutput::encodeFrame(const cv::Mat& bgrFrame)
{
    if (!codecCtx_ || !frame_ || !packet_ || !swsCtx_) {
        return false;
    }

    if (av_frame_make_writable(frame_) < 0) {
        std::cerr << "RtmpOutput: frame buffer not writable" << std::endl;
        return false;
    }

    if (bgrFrame.empty()) {
        return false;
    }

    cv::Mat converted;
    const int channels = bgrFrame.channels();
    if (channels == 3) {
        if (bgrFrame.isContinuous()) {
            converted = bgrFrame;
        } else {
            converted = bgrFrame.clone();
        }
    } else if (channels == 4) {
        cv::cvtColor(bgrFrame, converted, cv::COLOR_BGRA2BGR);
    } else if (channels == 1) {
        cv::cvtColor(bgrFrame, converted, cv::COLOR_GRAY2BGR);
    } else {
        converted = bgrFrame.clone();
    }

    const cv::Mat& source = converted;
    if (source.type() != CV_8UC3) {
        std::cerr << "RtmpOutput: unsupported frame format" << std::endl;
        return false;
    }

    const uint8_t* srcSlice[1] = { source.data };
    const int srcStride[1] = { static_cast<int>(source.step[0]) };

    sws_scale(swsCtx_,
            srcSlice,
            srcStride,
            0,
            codecCtx_->height,
            frame_->data,
            frame_->linesize);

    frame_->pts = pts_++;

    if (avcodec_send_frame(codecCtx_, frame_) < 0) {
        std::cerr << "RtmpOutput: failed to send frame to encoder" << std::endl;
        return false;
    }

    while (true) {
        const int ret = avcodec_receive_packet(codecCtx_, packet_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            std::cerr << "RtmpOutput: failed to receive packet from encoder" << std::endl;
            return false;
        }

        av_packet_rescale_ts(packet_, codecCtx_->time_base, videoStream_->time_base);
        packet_->stream_index = videoStream_->index;

        if (av_interleaved_write_frame(formatCtx_, packet_) < 0) {
            std::cerr << "RtmpOutput: failed to write packet to RTMP server at " << rtmpUrl_ << std::endl;
            av_packet_unref(packet_);
            
            std::cerr << "RtmpOutput: attempting to reconnect to RTMP server" << std::endl;
            closeOutput();
            initialized_ = false;
            
            return false;
        }

        av_packet_unref(packet_);
    }

    return true;
}

bool RtmpOutput::flushEncoder() {
	if (!codecCtx_) {
		return true;
	}

	if (avcodec_send_frame(codecCtx_, nullptr) < 0) {
		return false;
	}

	while (true) {
		const int ret = avcodec_receive_packet(codecCtx_, packet_);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			break;
		} else if (ret < 0) {
			return false;
		}

		av_packet_rescale_ts(packet_, codecCtx_->time_base, videoStream_->time_base);
		packet_->stream_index = videoStream_->index;
		av_interleaved_write_frame(formatCtx_, packet_);
		av_packet_unref(packet_);
	}

	return true;
}

}
