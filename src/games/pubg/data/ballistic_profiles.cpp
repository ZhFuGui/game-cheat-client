#include "ballistic_profiles.hpp"
#include "application/storage/json_store.hpp"
#include <cmath>
#include <set>
namespace mousebox::application
{
namespace
{
void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
std::string text(const Json &value)
{
    return value.is_string() ? value.get<std::string>() : std::string{};
}
double positive(const Json &value, bool optional = false)
{
    if (optional && value.is_null())
        return 0;
    require(value.is_number(), "弹道资料包含无效正数参数");
    const auto number = value.get<double>();
    require(std::isfinite(number) && number > 0, "弹道资料包含无效正数参数");
    return number;
}
std::vector<std::string> strings(const Json &value)
{
    require(value.is_array() && value.size() <= 128, "弹道资料名称映射无效");
    std::vector<std::string> result;
    for (const auto &v : value)
    {
        auto t = text(v);
        require(!t.empty() && t.size() <= 1024, "弹道资料名称映射为空或过长");
        result.push_back(std::move(t));
    }
    return result;
}
std::shared_ptr<const control::BallisticCatalog> parse(std::string_view bytes, const std::string &source)
{
    require(!bytes.empty() && bytes.size() <= 8 * 1024 * 1024, "弹道资料包为空或过大");
    const auto pack = Json::parse(bytes);
    require(pack.is_object(), "弹道资料包JSON无效");
    require(pack.value("schema_version", -1) == 1, "不支持此弹道资料包版本");
    const auto revision = text(pack.value("pack_id", Json{}));
    require(!revision.empty(), "弹道资料包缺少revision");
    const auto units = pack.value("units", Json::object());
    require(units.value("distance", std::string{}) == "m" && units.value("time", std::string{}) == "s" &&
                units.value("speed", std::string{}) == "m/s" && units.value("angle", std::string{}) == "rad",
            "弹道资料包单位不符");
    const auto records = pack.value("profiles", Json::array());
    require(records.is_array() && !records.empty() && records.size() <= 5000, "弹道资料包缺少有效目录");
    auto catalog = std::make_shared<control::BallisticCatalog>();
    catalog->revision = revision + "/sha256:" + storage::sha256(bytes);
    catalog->sourcePath = source;
    catalog->sourceBuild = text(pack.value("source", Json::object()).value("game_build", Json{}));
    std::set<std::string> ids;
    for (const auto &object : records)
    {
        require(object.is_object(), "弹道资料记录无效");
        control::BallisticProfile p;
        p.id = text(object.value("profile_id", Json{}));
        p.name = text(object.value("display_name", Json{}));
        require(!p.id.empty() && ids.insert(p.id).second, "弹道资料ID为空或重复");
        const auto bindings = object.value("bindings", Json::object());
        p.actorClasses = strings(bindings.value("actor_class_names", Json{}));
        p.itemIds = strings(bindings.value("item_ids", Json{}));
        const auto model = object.value("model", Json::object());
        require(model.value("id", std::string{}) == "published_reference.v1", "不支持此弹道模型类型");
        const auto params = model.value("parameters", Json::object());
        p.initialSpeed = positive(params.value("initial_speed_mps", Json{}), true);
        if (!params.value("max_travel_distance_m", Json{}).is_null())
            p.maximumTravel = positive(params["max_travel_distance_m"]);
        const auto velocity = params.value("velocity_curve", Json{});
        if (!velocity.is_null())
        {
            require(velocity.is_object() && velocity.value("abscissa", std::string{}) == "published_distance_m" &&
                        velocity.value("ordinate", std::string{}) == "published_speed_mps",
                    "弹速曲线坐标语义不符");
            const auto keys = velocity.value("keys", Json{});
            require(keys.is_array() && keys.size() <= 10000, "弹速曲线点数无效");
            for (const auto &pair : keys)
            {
                require(pair.is_array() && pair.size() == 2 && pair[0].is_number() && pair[1].is_number(),
                        "弹速曲线点无效");
                const auto x = pair[0].get<double>(), y = pair[1].get<double>();
                require(std::isfinite(x) && std::isfinite(y), "弹速曲线点无效");
                p.velocityKeys.emplace_back(x, y);
            }
        }
        p.packRevision = catalog->revision;
        p.source = text(object.value("provenance", Json::object()).value("source_ref", Json{}));
        if (object.contains("notes") && object["notes"].is_array())
            for (const auto &note : object["notes"])
                if (note.is_string())
                    p.notes.push_back(note.get<std::string>());
        catalog->entries.emplace_back(std::move(p));
    }
    return catalog;
}
#include "embedded_profiles.inc"
} // namespace
std::shared_ptr<const control::BallisticCatalog> loadBallisticProfiles(const std::filesystem::path &path)
{
    return parse(storage::readBytes(path, 8 * 1024 * 1024), storage::utf8(path.wstring()));
}
std::shared_ptr<const control::BallisticCatalog> defaultBallisticProfiles(
    const std::filesystem::path &executableDirectory)
{
    const auto external = executableDirectory / L"data/games/pubg/weapons/profiles.json";
    if (std::filesystem::exists(external))
        return loadBallisticProfiles(external);
    return parse(embeddedProfiles(), "embedded:data/games/pubg/weapons/profiles.json");
}
} // namespace mousebox::application
