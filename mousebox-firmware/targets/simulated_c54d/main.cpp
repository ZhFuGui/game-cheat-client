#include <cassert>
#include <iostream>
#include "../../apps/relay/runtime.hpp"
#include "../../core/control/session.hpp"
#include "../../core/control/ingress.hpp"
#include "../../core/relay/pipeline.hpp"
#include "../../profiles/logitech_c54d/profile.hpp"
#include "../leonardo_max3421e_c54d/limits.hpp"
namespace simulation
{
uint32_t timeUs = 0;
struct Clock
{
    static uint32_t nowUs()
    {
        return timeUs;
    }
};
struct Device
{
    static uint8_t generation;
    static unsigned submitted;
    static bool configured()
    {
        return true;
    }
    static uint8_t epoch()
    {
        return generation;
    }
    static void serviceControl() {}
    static int trySend(uint8_t endpoint, const void *bytes, uint16_t length)
    {
        assert(endpoint == 1 && length == 13);
        const uint8_t *data = static_cast<const uint8_t *>(bytes);
        assert(data[8] == uint8_t(submitted));
        ++submitted;
        return length;
    }
};
uint8_t Device::generation = 1;
unsigned Device::submitted = 0;
struct Channel
{
    static bool enabled;
    static unsigned feedback;
    static void begin(uint32_t baud)
    {
        assert(baud == 500000);
    }
    static void enableFeedback()
    {
        enabled = true;
    }
    static int available()
    {
        return 0;
    }
    static int read()
    {
        assert(false);
        return -1;
    }
    static int availableForWrite()
    {
        return 0;
    }
    static void write(const uint8_t *, uint16_t)
    {
        ++feedback;
    }
};
bool Channel::enabled = false;
unsigned Channel::feedback = 0;
typedef mousebox_firmware::ControlSession<mousebox_target::Limits> Session;
struct Resources
{
    enum
    {
        MouseQueueDepth = 32,
        AuxiliaryQueueDepth = 4,
        VendorQueueDepth = 8
    };
};
typedef mousebox_firmware::ReportPipeline<mousebox_firmware::C54dProfile, Resources, Session, Device, Clock> Pipeline;
typedef mousebox_firmware::ControlIngress<Session, Channel, Clock, mousebox_target::Capabilities> Ingress;
Pipeline pipeline;
Session control;
Ingress ingress;
mousebox_firmware::ButtonState buttons;
struct Host
{
    unsigned arrivals = 0, cancels = 0;
    uint32_t next = 0;
    bool online = true;
    void begin() {}
    void service() {}
    bool needsRecovery() const
    {
        return false;
    }
    void recover()
    {
        assert(false);
    }
    bool ready() const
    {
        return online;
    }
    void cancelControl()
    {
        ++cancels;
    }
    void poll()
    {
        if (timeUs < next || arrivals >= 2000 || !online)
            return;
        assert(pipeline.canReceive(1));
        uint8_t bytes[13] = {};
        bytes[0] = mousebox_buttons::Back;
        bytes[8] = uint8_t(arrivals);
        mousebox_wire::write16(bytes + 2, 7);
        pipeline.receive(1, 13, bytes);
        ++arrivals;
        next += 1000;
    }
} host;
struct Assembly
{
    typedef simulation::Device Device;
    static Session &control()
    {
        return simulation::control;
    }
    static Ingress &ingress()
    {
        return simulation::ingress;
    }
    static Pipeline &pipeline()
    {
        return simulation::pipeline;
    }
    static mousebox_firmware::ButtonState &buttons()
    {
        return simulation::buttons;
    }
    static Host &host()
    {
        return simulation::host;
    }
};
} // namespace simulation
int main()
{
    using namespace simulation;
    mousebox_firmware::RelayRuntime<Assembly> runtime;
    runtime.begin();
    for (timeUs = 0; timeUs < 2000000; timeUs += 100)
        runtime.step();
    assert(Device::submitted == 2000 && host.arrivals == 2000 && buttons.known && !Channel::enabled);
    assert(control.accept({mousebox_wire::Start, 1, 1, 0, 0, 0, 0, 32}, timeUs));
    ++Device::generation;
    host.online = false;
    runtime.step();
    assert(!control.active() && !buttons.known && host.cancels == 1);
    for (unsigned i = 0; i < 30000; ++i)
    {
        timeUs += 1000;
        runtime.step();
    }
    assert(Device::submitted == 2000 && Channel::feedback == 0);
    std::cout
        << "PASS: simulated target/common runtime, 2000 ordered reports, reset clears output, idle adds no reports\n";
}
