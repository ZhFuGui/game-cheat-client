#include "adapters/sources/pubg_reader/evidence.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <type_traits>
namespace mousebox::reader
{
namespace
{
void quoted(std::ostream &out, std::string_view value)
{
    out << '"';
    for (const unsigned char c : value)
    {
        if (c == '"' || c == '\\')
            out << '\\' << static_cast<char>(c);
        else if (c < 32)
            out << "\\u00" << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(c) << std::dec;
        else
            out << static_cast<char>(c);
    }
    out << '"';
}
void sample(std::ostream &out, const monitor::SampleInterval &s)
{
    const auto ns = [](auto time) {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(time.time_since_epoch()).count();
    };
    out << "{\"reader_started_monotonic_ns\":" << ns(s.started)
        << ",\"reader_finished_monotonic_ns\":" << ns(s.finished) << '}';
}
void group(std::ostream &out, const consumer::GroupSample &g)
{
    out << "{\"wire_status\":" << sync::FactStatusToWire(g.status) << ",\"has_value\":" << (g.value ? "true" : "false")
        << ",\"source_capture\":" << g.source_capture_sequence;
    if (g.source_capture_sequence)
    {
        out << ",\"source_sample\":";
        sample(out, g.source);
    }
    out << ",\"sources\":[";
    bool first = true;
    for (const auto &[path, source] : g.sources)
    {
        if (!first)
            out << ',';
        first = false;
        out << "{\"path\":[";
        for (std::size_t i = 0; i < path.size(); ++i)
        {
            if (i)
                out << ',';
            out << path[i];
        }
        out << "],\"capture\":" << source.capture_sequence
            << ",\"wire_status\":" << sync::FactStatusToWire(source.status)
            << ",\"reused\":" << (source.reused ? "true" : "false")
            << ",\"category\":" << static_cast<unsigned>(source.source_category) << ",\"sample\":";
        sample(out, source.source);
        out << '}';
    }
    out << "]}";
}
} // namespace
Evidence::Evidence(const std::string &root, const char *category, unsigned connection)
{
    if (!root.empty())
    {
        folder_ = std::filesystem::path(root) / (std::string(category) + "-" + std::to_string(connection));
        std::filesystem::create_directories(folder_);
    }
}
void Evidence::write(const char *name, std::string_view data)
{
    if (folder_.empty())
        return;
    std::ofstream file(folder_ / name, std::ios::binary | std::ios::trunc);
    file.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!file)
        throw std::runtime_error("Reader evidence write failed");
}
void Evidence::received(std::string_view body, unsigned payload)
{
    if (folder_.empty())
        return;
    if (payload == 6)
        write("accepted.body.pb", body);
    if (payload == 19)
    {
        write("baseline.body.pb", body);
        latestUpdate_.clear();
        write("update.body.pb", {});
    }
    if (payload == 20)
        latestUpdate_.assign(body);
    if (payload == 18)
        write("probe-reply.body.pb", body);
}
void Evidence::sentAck(std::string_view packet)
{
    write("ack.packet.pb", packet);
}
void Evidence::snapshot(const Session &session)
{
    if (folder_.empty())
        return;
    if (!latestUpdate_.empty())
        write("update.body.pb", latestUpdate_);
    const auto current = session.current();
    std::ostringstream out;
    out << "{\"evidence_kind\":\"validated-consumer-summary\",\"current\":" << (current ? "true" : "false")
        << ",\"session\":" << session.binding().session.session_id << ",\"reader_stream\":";
    quoted(out, session.binding().session.stream_id);
    if (current)
    {
        out << ",\"epoch\":" << current->epoch << ",\"revision\":" << current->revision
            << ",\"capture\":" << current->capture_sequence
            << ",\"batch_category\":" << static_cast<unsigned>(current->category) << ",\"context\":{";
        bool first = true;
        for (const auto &[id, g] : current->context.local_groups)
        {
            if (!first)
                out << ',';
            first = false;
            quoted(out, std::to_string(static_cast<unsigned>(id)));
            out << ':';
            group(out, g);
        }
        out << "},\"entities\":[";
        first = true;
        for (const auto &[id, entity] : current->entities)
        {
            if (!first)
                out << ',';
            first = false;
            out << "{\"id\":\"" << id << "\",\"kind\":" << static_cast<unsigned>(entity.kind) << ",\"groups\":{";
            bool firstGroup = true;
            for (const auto &[idGroup, g] : entity.groups)
            {
                if (!firstGroup)
                    out << ',';
                firstGroup = false;
                quoted(out, std::to_string(sync::PropertyGroupToWire(idGroup)));
                out << ':';
                group(out, g);
            }
            out << "}}";
        }
        out << ']';
    }
    out << ",\"value_records\":\"baseline.body.pb + update.body.pb\"}";
    write("latest-state.json", out.str());
    if (current && !current->entities.empty())
        write("nonempty-state.json", out.str());
}
} // namespace mousebox::reader
