#include "random_range.hpp"
#include <cmath>
#include <stdexcept>

namespace mousebox::application
{
random::IntegerRange readRandomRange(const Json &value, unsigned minimum, unsigned maximum)
{
    if (!value.is_object() || !value.contains("min") || !value.contains("max") ||
        value.size() != (value.contains("distribution") ? 3u : 2u))
        throw std::invalid_argument("随机区间须包含下限、上限及可选的分布模式");

    const auto endpoint = [&](const char *key) {
        const auto &item = value.at(key);
        if (!item.is_number())
            throw std::invalid_argument("区间端点须为整数");
        const auto number = item.get<double>();
        if (!std::isfinite(number) || number < minimum || number > maximum || number != std::floor(number))
            throw std::invalid_argument("区间端点超出允许范围");
        return static_cast<unsigned>(number);
    };
    random::IntegerRange range{endpoint("min"), endpoint("max")};
    if (range.min > range.max)
        throw std::invalid_argument("区间上限须大于或等于下限");
    if (const auto mode = value.find("distribution"); mode != value.end())
    {
        if (!mode->is_string())
            throw std::invalid_argument("随机分布模式须为字符串");
        const auto parsed = random::distributionFromId(mode->get_ref<const std::string &>());
        if (!parsed)
            throw std::invalid_argument("随机分布模式无效");
        range.distribution = *parsed;
    }
    return range;
}

Json randomRangeJson(const random::IntegerRange &range)
{
    return {{"min", range.min}, {"max", range.max}, {"distribution", random::distributionId(range.distribution)}};
}
} // namespace mousebox::application
