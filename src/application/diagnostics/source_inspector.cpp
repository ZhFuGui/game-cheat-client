#include "source_inspector.hpp"
#include <condition_variable>
#include <mutex>
#include <thread>
#include <optional>
namespace mousebox::application
{
struct SourceInspector::Impl
{
    struct Work
    {
        ports::SourcePacket packet;
        std::uint64_t generation;
    };
    std::mutex mutex;
    std::condition_variable_any wake;
    std::optional<Work> pending;
    std::string scope, revision, payload, error;
    std::uint64_t generation = 0;
    std::jthread worker;
    Impl()
        : worker([this](std::stop_token stop) {
              while (!stop.stop_requested())
              {
                  std::optional<Work> work;
                  {
                      std::unique_lock lock(mutex);
                      wake.wait(lock, stop, [&] { return pending.has_value(); });
                      if (stop.stop_requested())
                          return;
                      work = std::move(pending);
                      pending.reset();
                  }
                  std::string text, failure;
                  try
                  {
                      text = work->packet.read().dump(-1, ' ', false, Json::error_handler_t::replace);
                      if (text.size() > 3 * 1024 * 1024)
                          throw std::runtime_error("源数据超过3 MiB查看上限，请暂停并缩小订阅范围");
                  }
                  catch (const std::exception &e)
                  {
                      failure = e.what();
                      text.clear();
                  }
                  {
                      const std::lock_guard lock(mutex);
                      if (work->generation == generation)
                      {
                          revision = work->packet.revision;
                          payload = std::move(text);
                          error = std::move(failure);
                      }
                  }
              }
          })
    {
    }
    ~Impl()
    {
        worker.request_stop();
        wake.notify_all();
    }
};
SourceInspector::SourceInspector() : impl_(std::make_unique<Impl>()) {}
SourceInspector::~SourceInspector() = default;
Json SourceInspector::read(ports::SourcePackets packets, std::string_view selected, std::string_view after)
{
    auto &d = *impl_;
    Json streams = Json::array();
    const ports::SourcePacket *packet = nullptr;
    for (const auto &p : packets)
    {
        streams.push_back(
            {{"id", p.id}, {"title", p.title}, {"available", static_cast<bool>(p.read)}, {"format", p.format}});
        if (p.id == selected || (selected.empty() && !packet))
            packet = &p;
    }
    const std::string scope = packet && packet->read ? packet->id + ":" + packet->identity : std::string{};
    const std::lock_guard lock(d.mutex);
    if (scope != d.scope || scope.empty())
    {
        d.scope = scope;
        ++d.generation;
        d.pending.reset();
        d.revision.clear();
        d.payload.clear();
        d.error.clear();
    }
    const auto sample = d.revision.empty() ? Json{} : Json(d.revision);
    const bool unchanged = !d.revision.empty() && d.revision == after;
    Json result{{"streams", std::move(streams)},
                {"selected", packet ? Json(packet->id) : Json{}},
                {"revision", sample},
                {"payload", !unchanged && !d.payload.empty() ? Json(d.payload) : Json{}},
                {"unchanged", unchanged},
                {"error", d.error.empty() ? Json{} : Json(d.error)}};
    if (packet && packet->read && packet->revision != d.revision)
    {
        d.pending = Impl::Work{*packet, d.generation};
        d.wake.notify_one();
    }
    return result;
}
} // namespace mousebox::application
