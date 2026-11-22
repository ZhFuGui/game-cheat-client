#include "reader_chain.hpp"
namespace mousebox::application
{
Json describeReaderChain(const domain::ReaderChainSnapshot &s)
{
    const auto stage = [](const domain::ReaderStageAverage &v) {
        return Json{{"averageMs", v.averageMs ? Json(*v.averageMs) : Json{}},
                    {"uncertaintyMs", v.uncertaintyMs ? Json(*v.uncertaintyMs) : Json{}},
                    {"samples", v.samples},
                    {"status", v.status}};
    };
    return {
        {"windowMs", 2000}, {"read", stage(s.read)}, {"prepare", stage(s.prepare)}, {"transport", stage(s.transport)}};
}
} // namespace mousebox::application
