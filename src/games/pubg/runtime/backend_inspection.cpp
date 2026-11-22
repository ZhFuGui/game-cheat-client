#include "backend_data.hpp"
namespace mousebox::application
{
ports::SourcePackets Backend::inspectSources()
{
    const std::lock_guard lock(data_->mutex);
    return {
        data_->reader.livingInspection.read ? data_->reader.livingInspection
                                            : ports::SourcePacket{"living", "PUBG Living", "", "", "materialized", {}},
        data_->reader.localInspection.read ? data_->reader.localInspection
                                           : ports::SourcePacket{"local", "PUBG Local", "", "", "materialized", {}}};
}
} // namespace mousebox::application
