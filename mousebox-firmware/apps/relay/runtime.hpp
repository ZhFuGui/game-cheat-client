#pragma once
#include "../../core/contracts/ports.hpp"

namespace mousebox_firmware
{
template <class Assembly> class RelayRuntime
{
    uint8_t previousEpoch = 0;

  public:
    void begin()
    {
        Assembly::ingress().begin();
        Assembly::pipeline().attachControl(Assembly::control());
        Assembly::pipeline().attachButtons(Assembly::buttons());
        previousEpoch = Assembly::Device::epoch();
        Assembly::host().begin();
    }
    void step()
    {
        const uint8_t epoch = Assembly::Device::epoch();
        if (epoch != previousEpoch)
        {
            Assembly::pipeline().clear();
            Assembly::host().cancelControl();
            previousEpoch = epoch;
        }
        Assembly::host().service();
        Assembly::pipeline().service();
        if (Assembly::host().needsRecovery())
        {
            Assembly::pipeline().clear();
            Assembly::host().recover();
        }
        Assembly::host().poll();
        Assembly::Device::serviceControl();
        Assembly::pipeline().service();
        Assembly::ingress().service(Assembly::control(), Assembly::host().ready() && Assembly::Device::configured(),
                                    Assembly::buttons());
        Assembly::pipeline().service();
    }
};
} // namespace mousebox_firmware
