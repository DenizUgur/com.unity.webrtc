#include "pch.h"

#include "NvControlInjection.h"

namespace unity
{
namespace webrtc
{

    ControllerBase::ControllerBase()
    {
        decodedImageCallback = new DecodedImageCallbackLocal();

#ifdef _WIN32
        INT rc;
        WSADATA wsaData;

        rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (rc)
        {
            printf("WSAStartup Failed.\n");
            return;
        }
#endif
        // Try to conenct to Web Socket server
        ws = WebSocket::from_url("ws://localhost:8080/proxy/encoder");
    }

    int ControllerBase::ModerateRateControl(const VideoEncoder::RateControlParameters& parameters)
    {
        // We don't have any information so far continue with WebRTC's request
        if (currentRateControlParameters == nullptr)
        {
            currentRateControlParameters = new VideoEncoder::RateControlParameters();
            memcpy(
                (void*)currentRateControlParameters, (void*)&parameters, sizeof(VideoEncoder::RateControlParameters));
            return 0;
        }

        // Continue to control the state
        if (targetRateControlParameters == nullptr)
        {
            memcpy(
                (void*)currentRateControlParameters, (void*)&parameters, sizeof(VideoEncoder::RateControlParameters));
            return 0;
        }

        // Proxy requested a rate control change that is different than the previous change
        if (targetRateControlParameters != currentRateControlParameters)
        {
            encoderParametersNeedsChange = false;
            memcpy(
                (void*)currentRateControlParameters,
                (void*)targetRateControlParameters,
                sizeof(VideoEncoder::RateControlParameters));
            return 0;
        }

        return 1;
    }

    void ControllerBase::ReceiveRateControlCommand(const std::string& message)
    {
        // We need currentRateControlParameters for fallback
        if (currentRateControlParameters == nullptr)
            return;

        json j = json::parse(message);
        VideoEncoder::RateControlParameters rcp;

        VideoBitrateAllocation vba;
        double fps = currentRateControlParameters->framerate_fps;
        DataRate dr = currentRateControlParameters->bandwidth_allocation;

        // If command has bitrate then create VideoBitrateAllocation if not then get current one
        if (j.contains("encoder_bitrate"))
            vba.SetBitrate(0, 0, (uint32_t)j["encoder_bitrate"]);
        else
            memcpy(&vba, &currentRateControlParameters->bitrate, sizeof(VideoBitrateAllocation));

        // If command has fps then use that if not then use current one
        if (j.contains("encoder_fps"))
            fps = (double)j["encoder_fps"];

        // if command has bandwidth_allocation then create DataRate if not then use current one
        if (j.contains("encoder_bandwidth_allocation"))
            rcp = VideoEncoder::RateControlParameters(
                vba, fps, DataRate::BitsPerSec((int)j["encoder_bandwidth_allocation"]));
        else
            rcp = VideoEncoder::RateControlParameters(vba, fps, dr);

        // After construction compare with current targetRateControlParameters if it's equal then return
        if (targetRateControlParameters != nullptr && targetRateControlParameters == &rcp)
            return;

        // Copy newly constructed parameters to targetRateControlParameters
        if (targetRateControlParameters == nullptr)
            targetRateControlParameters =
                (VideoEncoder::RateControlParameters*)malloc(sizeof(VideoEncoder::RateControlParameters));
        memcpy((void*)targetRateControlParameters, (void*)&rcp, sizeof(VideoEncoder::RateControlParameters));

        // Finish command by calling NvEncoderImpl::SetRates ourselves
        encoderParametersNeedsChange = true;
    }

    void ControllerBase::SubmitMetrics(int64_t time_us, EncoderMetrics metrics)
    {
        // Check if we have ws connection
        if (ws == nullptr || ws->getReadyState() == WebSocket::CLOSED)
        {
            // Can we create it then?
            ws = WebSocket::from_url("ws://localhost:8080/proxy/encoder");
            if (ws == nullptr)
                return;
        }

        // Send current data
        json j = { { "time_us", time_us },
                   { "ssim", metrics.ssim },
                   { "psnr", metrics.psnr },
                   { "sse", metrics.sse },
                   { "estimated_bitrate", metrics.estimated_bitrate },
                   { "encoded_frame_size", metrics.encoded_frame_size } };
        ws->send(j.dump());

        // Poll the data
        ws->poll();

        // While we are here, receive the message
        ws->dispatch([=](const std::string message) { ReceiveRateControlCommand(message); });
    }

} // end namespace webrtc
} // end namespace unity
