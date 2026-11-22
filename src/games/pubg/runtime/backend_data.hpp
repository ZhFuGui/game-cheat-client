#pragma once
#include "backend.hpp"
#include "games/pubg/model/esp_layers.hpp"
#include "games/pubg/diagnostics/reader_metrics.hpp"
#include <deque>
#include <mutex>

namespace mousebox::application
{
// Internal synchronization boundary shared only by Backend implementation files.
// Notifications are copied under mutex and invoked after releasing it.
struct Backend::Data
{
    std::mutex mutex;
    Snapshot snapshot;
    struct ReaderState
    {
        std::optional<control::Frame> livingContext, viewContext;
        std::uint64_t livingConnection = 0, viewConnection = 0;
        ports::SourcePacket livingInspection, localInspection;
    } reader;
    ReaderMetrics readerMetrics;
    std::deque<std::string> logs;
    control::CalibrationProfiles profiles;
    std::shared_ptr<const control::Frame> previewLiving, previewView;
    control::EspLayers espLayers;
    std::function<void()> sceneNotifier;
    void resolveCalibration();
};
} // namespace mousebox::application
