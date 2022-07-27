#include "pch.h"

#include "NvControlInjection.h"

namespace unity
{
namespace webrtc
{

    ControllerBase::ControllerBase() {
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
        ws = WebSocket::from_url("ws://localhost:8080/proxy/encoder");
        assert(ws);
    }

    void ControllerBase::SubmitMetrics(int64_t time_us, double ssim, double psnr, double sse)
    {
        json j = {
            { "time_us", time_us },
            { "ssim", ssim },
            { "psnr", psnr },
            { "sse", sse },
        };

        ws->send(j.dump());
        ws->poll();
    }

} // end namespace webrtc
} // end namespace unity
