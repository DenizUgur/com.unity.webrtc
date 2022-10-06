#pragma once

#include "NvDecoderImpl.h"

#include <nlohmann/json.hpp>
#include <ws/wsclient.hpp>

namespace unity
{
namespace webrtc
{
    using json = nlohmann::json;
    using wsclient::WebSocket;

    // Forward decleration
    class NvEncoderImpl;

    class DecodedImageCallbackLocal : public ::webrtc::DecodedImageCallback
    {
    private:
        ::webrtc::VideoFrame* m_decodedImage = nullptr;

    public:
        DecodedImageCallbackLocal() { }
        ~DecodedImageCallbackLocal() { }

        int32_t Decoded(::webrtc::VideoFrame& decodedImage) { return 0; };
        int32_t Decoded(::webrtc::VideoFrame& decodedImage, int64_t decode_time_ms) { return 0; };

        void
        Decoded(::webrtc::VideoFrame& decodedImage, absl::optional<int32_t> decode_time_ms, absl::optional<uint8_t> qp)
        {
            m_decodedImage = &decodedImage;
        }

        ::webrtc::VideoFrame& GetDecodedImage() { return *m_decodedImage; }
    };

    class ControllerBase
    {
    private:
        DecodedImageCallbackLocal* decodedImageCallback;
        WebSocket::pointer ws;

        VideoEncoder::RateControlParameters* currentRateControlParameters = nullptr;

    public:
        bool overrideSetRates = false;
        bool encoderParametersNeedsChange = false;
        VideoEncoder::RateControlParameters* targetRateControlParameters = nullptr;

        struct EncoderMetrics
        {
            /* quality */
            double ssim;
            double psnr;
            double sse;
            /* bitrate */
            uint32_t estimated_bitrate;
            uint64_t encoded_frame_size;
        };

        ControllerBase();
        ~ControllerBase() {};

        int ModerateRateControl(const VideoEncoder::RateControlParameters& parameters);
        const VideoEncoder::RateControlParameters& GetCurrentRateControlParameters(const VideoEncoder::RateControlParameters& in_parameters)
        {
            if (overrideSetRates)
                return *currentRateControlParameters;
            else
                return in_parameters;
        }
        void ReceiveRateControlCommand(const std::string& message);
        void SubmitMetrics(int64_t time_us, EncoderMetrics metrics);

        DecodedImageCallbackLocal* GetDecodedImageCallback() { return decodedImageCallback; };
    };

} // end namespace webrtc
} // end namespace unity
