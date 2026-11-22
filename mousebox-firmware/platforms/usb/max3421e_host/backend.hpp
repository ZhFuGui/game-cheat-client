#pragma once
#include "transport.hpp"

namespace mousebox_max3421e
{
template <class Profile, class Pipeline, class Device, class Clock> class HostBackend
{
    typedef HostTransport<Profile, Pipeline, Device, Clock> Transport;
    USB usb;
    Transport transport;
    class Receiver : public HIDComposite
    {
        Pipeline &pipeline;
        Transport &transport;
        uint32_t nextPollUs[3] = {};
        bool nativeReady = false;

      public:
        Receiver(USB *controller, Pipeline &reports, Transport &transfers)
            : HIDComposite(controller), pipeline(reports), transport(transfers)
        {
        }
        bool SelectInterface(uint8_t iface, uint8_t) override
        {
            return iface < 3;
        }
        bool isReady() override
        {
            return bPollEnable && nativeReady;
        }
        uint8_t OnInitSuccessful() override
        {
            nativeReady = false;
            if (!Profile::matches(VID, PID))
                return USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;
            for (uint8_t iface = 0; iface < 2; ++iface)
            {
                const uint8_t error = SetProtocol(iface, 1);
                if (error)
                    return error;
            }
            for (uint8_t i = 0; i < 3; ++i)
            {
                if (!pUsb->getEpInfoEntry(bAddress, Profile::endpointAt(i)))
                    return USB_ERROR_EP_NOT_FOUND_IN_TBL;
                nextPollUs[i] = Clock::nowUs();
            }
            nativeReady = true;
            return 0;
        }
        uint8_t Poll() override
        {
            if (!isReady() || transport.busy() || !Device::configured() || pUsb->getUsbTaskState() != USB_STATE_RUNNING)
                return 0;
            const uint32_t now = Clock::nowUs();
            for (uint8_t i = 0; i < 3; ++i)
            {
                const uint8_t ep = Profile::endpointAt(i);
                const uint32_t period = Profile::pollIntervalUs(ep);
                const int32_t late = now - nextPollUs[i];
                if (late < 0)
                    continue;
                if (!pipeline.canReceive(ep))
                {
                    nextPollUs[i] += (uint32_t(late) / period + 1) * period;
                    continue;
                }
                if (transport.beginInput(bAddress, ep))
                {
                    nextPollUs[i] += (uint32_t(late) / period + 1) * period;
                    return 0;
                }
            }
            return 0;
        }
        uint8_t Release() override
        {
            nativeReady = false;
            pipeline.clear();
            transport.reset();
            return HIDComposite::Release();
        }
    } receiver;
    bool initialized = false;

  public:
    explicit HostBackend(Pipeline &pipeline) : transport(usb, pipeline), receiver(&usb, pipeline, transport) {}
    void begin()
    {
        UsbDEBUGlvl = 0;
        initialized = usb.Init() == 0;
    }
    void service()
    {
        transport.service();
    }
    void poll()
    {
        if (initialized && !transport.busy())
            usb.Task();
    }
    bool ready()
    {
        return receiver.isReady();
    }
    bool needsRecovery() const
    {
        return transport.needsRecovery;
    }
    void recover()
    {
        transport.reset();
        initialized = usb.Init() == 0;
        usb.setUsbTaskState(USB_DETACHED_SUBSTATE_INITIALIZE);
    }
    void cancelControl()
    {
        transport.cancelControl();
    }
    bool beginControl(const ControlRequest &setup, const uint8_t *payload)
    {
        return ready() && Profile::acceptsRequest(setup) &&
               transport.beginControl(receiver.GetAddress(), setup, payload);
    }
    int finishControl(uint8_t *reply, uint8_t &length)
    {
        return transport.finishControl(reply, length);
    }
};
} // namespace mousebox_max3421e
