#include "backend_data.hpp"
#include "platform/windows/input.hpp"
#include <stdexcept>
#include "adapters/sources/pubg_reader/source.hpp"

namespace mousebox::application
{
void Backend::disconnectReader()
{
    // Revoke live output before waiting for either network worker to finish.
    {
        const std::lock_guard lock(data_->mutex);
        data_->snapshot.readerRunning = false;
        data_->snapshot.livingConnected = false;
        data_->snapshot.viewConnected = false;
    }
    livingWorker_.request_stop();
    viewWorker_.request_stop();
    if (livingWorker_.joinable())
        livingWorker_.join();
    if (viewWorker_.joinable())
        viewWorker_.join();
    {
        const std::lock_guard lock(data_->mutex);
        auto &s = data_->snapshot;
        s.readerRunning = false;
        s.living.reset();
        s.view.reset();
        s.livingRtt.reset();
        s.viewRtt.reset();
        s.source = "未连接";
        ++s.generation;
        s.readerLivingConnecting = s.readerLocalConnecting = false;
        s.readerLivingError.clear();
        s.readerLocalError.clear();
        ++s.observationRevision;
        data_->reader.livingContext.reset();
        data_->reader.viewContext.reset();
        data_->reader.livingInspection = {};
        data_->reader.localInspection = {};
        data_->previewLiving.reset();
        data_->previewView.reset();
        data_->espLayers.clear();
        data_->readerMetrics.subscribe({});
        data_->resolveCalibration();
    }
    notifyScene();
}

void Backend::loadRecording(const std::filesystem::path &requested)
{
    // Validate both files before replacing the visible scene or stopping the current connection.
    const auto frames = reader::loadRecordedFrames(requested);
    disconnectReader();
    {
        const std::lock_guard lock(data_->mutex);
        data_->snapshot.living = frames.living;
        data_->snapshot.view = frames.local;
        reader::bindSourceLocal(*data_->snapshot.living, &*data_->snapshot.view);
        data_->previewLiving = std::make_shared<const control::Frame>(*data_->snapshot.living);
        data_->previewView = std::make_shared<const control::Frame>(frames.local);
        data_->espLayers.accept(data_->previewLiving);
        data_->espLayers.accept(data_->previewView);
        ++data_->snapshot.observationRevision;
        data_->snapshot.source = "录制样本 · 离线预览";
        data_->resolveCalibration();
    }
    notifyScene();
    log("已载入经过协议核验的 Living / Local 录制样本。离线预览不会输出鼠标动作。");
}

void Backend::connectReader(const std::string &host, unsigned short port, double hz)
{
    disconnectReader();
    {
        const std::lock_guard lock(data_->mutex);
        data_->snapshot.readerRunning = true;
        data_->snapshot.source = "正在连接 Reader";
        data_->readerMetrics.subscribe({{reader::EntityChannel, "Living"}, {reader::LocalChannel, "Local"}});
    }
    reader::SourceOptions options;
    options.host = host;
    options.port = port;
    options.hz = hz;
    options.instance = "mousebox-native-" + std::to_string(randomSession());
    options.onLog = [this](const std::string &message) { log(message); };
    options.onTraffic = [this](unsigned, std::uint64_t sent, std::uint64_t received) {
        data_->readerMetrics.traffic(monotonicMilliseconds(), sent, received);
    };
    options.onConnection = [this](unsigned category, reader::ConnectionPhase phase, const std::string &error) {
        const std::lock_guard lock(data_->mutex);
        auto &s = data_->snapshot;
        if (phase == reader::ConnectionPhase::Connecting)
            (category == reader::EntityChannel ? data_->reader.livingConnection : data_->reader.viewConnection) =
                data_->readerMetrics.begin(category);
        (category == reader::EntityChannel ? s.readerLivingConnecting : s.readerLocalConnecting) =
            phase == reader::ConnectionPhase::Connecting;
        (category == reader::EntityChannel ? s.readerLivingError : s.readerLocalError) =
            phase == reader::ConnectionPhase::Error ? error.substr(0, 1024) : std::string{};
    };
    options.onState = [this](unsigned category, const reader::SourceState &source) {
        const auto current = source.frame;
        bool changed = false;
        {
            const std::lock_guard lock(data_->mutex);
            auto &s = data_->snapshot;
            const auto revision = s.observationRevision;
            (category == reader::EntityChannel ? data_->reader.livingInspection : data_->reader.localInspection) =
                source.inspection;
            (category == reader::EntityChannel ? s.livingConnected : s.viewConnected) = source.ready;
            auto &frame = category == reader::EntityChannel ? s.living : s.view;
            auto &observed = category == reader::EntityChannel ? s.livingObservedMs : s.viewObservedMs;
            auto &context = category == reader::EntityChannel ? data_->reader.livingContext : data_->reader.viewContext;
            if (!current)
            {
                if (frame)
                    ++s.observationRevision;
                frame.reset();
                observed = 0;
            }
            else if (source.stateChanged)
            {
                ++s.observationRevision;
                auto next = *current;
                next.header.consumerConnection =
                    category == reader::EntityChannel ? data_->reader.livingConnection : data_->reader.viewConnection;
                next.header.consumerReceivedAtMs =
                    source.chainTiming ? std::optional(source.chainTiming->receivedAtMs) : std::nullopt;
                const bool contextChanged = !context || next.header.epoch != context->header.epoch ||
                                            next.header.reader != context->header.reader ||
                                            (next.header.world && next.header.world != context->header.world) ||
                                            (next.header.level && next.header.level != context->header.level) ||
                                            (next.controller && next.controller != context->controller);
                if (contextChanged)
                    ++s.generation;
                // Living has no controller in the Local protocol. Remember each
                // stream's World/Level independently, before binding Local facts.
                if (next.header.world && next.header.level)
                {
                    control::Frame saved;
                    saved.header = next.header;
                    saved.controller = next.controller;
                    saved.self = next.self;
                    // Retain a known identity only for change detection across an
                    // independent failed read; it never supplies current facts.
                    if (!contextChanged && context && !saved.controller)
                        saved.controller = context->controller;
                    context = std::move(saved);
                }
                if (!frame || next.header.session != frame->header.session ||
                    next.header.sourceFinishNs > frame->header.sourceFinishNs)
                    observed = monotonicMilliseconds();
                frame = std::move(next);
            }
            if (frame)
            {
                const auto mapped = current ? current->header.sourceClock : std::nullopt;
                if (frame->header.sourceClock != mapped)
                {
                    frame->header.sourceClock = mapped;
                    frame->scene = current ? current->scene : nullptr;
                    ++s.observationRevision;
                }
            }
            auto &rtt = category == reader::EntityChannel ? s.livingRtt : s.viewRtt;
            data_->readerMetrics.observe(category, frame ? &frame->header : nullptr,
                                         source.receivedAtMs.value_or(monotonicMilliseconds()),
                                         source.chainTiming ? &*source.chainTiming : nullptr);
            rtt = source.roundTripMs;
            s.source = s.living && s.view ? "Reader 实时预览" : "Reader · 等待当前数据";
            if (s.living)
                reader::bindSourceLocal(*s.living, s.view ? &*s.view : nullptr);
            data_->resolveCalibration();
            changed = revision != s.observationRevision;
            if (changed)
            {
                data_->previewLiving = s.living ? std::make_shared<const control::Frame>(*s.living) : nullptr;
                data_->previewView = s.view ? std::make_shared<const control::Frame>(*s.view) : nullptr;
                if (data_->previewLiving)
                    data_->espLayers.accept(data_->previewLiving);
                else
                    data_->espLayers.invalidate(control::Category::Living);
                if (data_->previewView)
                    data_->espLayers.accept(data_->previewView);
                else
                    data_->espLayers.invalidate(control::Category::Local);
            }
        }
        if (changed)
            notifyScene();
    };
    const auto worker = [this, options](unsigned category, std::stop_token stop) {
        try
        {
            reader::observeSource(options, category, stop);
        }
        catch (const std::exception &error)
        {
            options.onState(category, {});
            if (!stop.stop_requested())
                options.onConnection(category, reader::ConnectionPhase::Error, error.what());
            log(std::string("Reader 连接结束：") + error.what());
        }
    };
    livingWorker_ = std::jthread([worker](std::stop_token stop) { worker(reader::EntityChannel, stop); });
    viewWorker_ = std::jthread([worker](std::stop_token stop) { worker(reader::LocalChannel, stop); });
}
} // namespace mousebox::application
