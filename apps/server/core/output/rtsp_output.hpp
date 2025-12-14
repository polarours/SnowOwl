#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

#include <opencv2/core.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include "core/streams/stream_dispatcher.hpp"

namespace SnowOwl::Server::Core {

class RtspOutput : public StreamOutput {
public:
    explicit RtspOutput(StreamOutputConfig config);
    ~RtspOutput() override;

    bool start() override;
    void stop() override;
    void publishFrame(const cv::Mat& frame) override;
    void publishEvents(const std::vector<Detection::DetectionResult>& events) override;

private:
    bool ensureInitialized(const cv::Mat& frame);
    bool openOutput(int width, int height);
    void closeOutput();
    bool encodeFrame(const cv::Mat& frame);
    bool flushEncoder();
    
    StreamOutputConfig config_;
    std::string rtspUrl_;

    std::mutex mutex_;
    std::atomic<bool> started_{false};
    std::atomic<bool> initialized_{false};

    AVFormatContext* formatCtx_{nullptr};
    AVCodecContext* codecCtx_{nullptr};
    AVStream* videoStream_{nullptr};
    SwsContext* swsCtx_{nullptr};
    AVFrame* frame_{nullptr};
    AVPacket* packet_{nullptr};
    
    std::int64_t pts_{0};
    int fpsNumerator_{30};
};    

}