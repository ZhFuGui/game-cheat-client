#pragma once
#include "../../core/control/session.hpp"
#include "../../core/control/ingress.hpp"
#include "../../core/relay/pipeline.hpp"
#include "../../profiles/logitech_c54d/profile.hpp"
#include "../../platforms/mcu/avr32u4/clock.hpp"
#include "../../platforms/mcu/avr32u4/uart.hpp"
#include "../../platforms/usb/avr_device/device.hpp"
#include "../../platforms/usb/avr_device/c54d_interfaces.hpp"
#include "../../platforms/usb/max3421e_host/backend.hpp"
#include "../../platforms/boards/leonardo_r3_max3421e/board.hpp"
#include "../../apps/relay/runtime.hpp"
#include "limits.hpp"

namespace mousebox_target
{
typedef mousebox_firmware::ControlSession<Limits> Session;
typedef mousebox_firmware::ReportPipeline<mousebox_firmware::C54dProfile, mousebox_avr::LeonardoHostBoard, Session,
                                          mousebox_avr::Device, mousebox_avr::Clock>
    Pipeline;
typedef mousebox_max3421e::HostBackend<mousebox_firmware::C54dProfile, Pipeline, mousebox_avr::Device,
                                       mousebox_avr::Clock>
    Host;
typedef mousebox_firmware::ControlIngress<Session, mousebox_avr::Uart, mousebox_avr::Clock, Capabilities> Ingress;
Session session;
mousebox_firmware::ButtonState buttonState;
Pipeline reports;
Host usbHost(reports);
Ingress commands;
mousebox_avr::NativeUsbInterfaces nativeInterfaces;

struct Assembly
{
    typedef mousebox_avr::Device Device;
    static Session &control()
    {
        return session;
    }
    static mousebox_firmware::ButtonState &buttons()
    {
        return buttonState;
    }
    static Pipeline &pipeline()
    {
        return reports;
    }
    static Host &host()
    {
        return usbHost;
    }
    static Ingress &ingress()
    {
        return commands;
    }
};
mousebox_firmware::RelayRuntime<Assembly> runtime;
} // namespace mousebox_target

// Existing AVR USB core hooks stay at the platform's composition boundary.
bool USB_BridgeStart(const USBSetup &setup, const uint8_t *payload)
{
    const mousebox_firmware::ControlRequest request = {setup.bmRequestType, setup.bRequest, setup.wValueL,
                                                       setup.wValueH,       setup.wIndex,   setup.wLength};
    return mousebox_target::usbHost.beginControl(request, payload);
}
int USB_BridgePoll(uint8_t *reply, uint8_t &length)
{
    return mousebox_target::usbHost.finishControl(reply, length);
}
void USB_BridgeCancel()
{
    mousebox_target::usbHost.cancelControl();
}
