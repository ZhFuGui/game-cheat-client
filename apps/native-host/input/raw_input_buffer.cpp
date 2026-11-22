#include "raw_input_buffer.hpp"
#include <cstring>
#include <stdexcept>

namespace mousebox::desktop
{
void visitRawMouseBuffer(std::span<const std::byte> bytes, UINT records,
                         const std::function<void(const RAWINPUT &)> &receive)
{
    static_assert(sizeof(void *) == 8, "The desktop build is native x64, not WOW64");
    std::size_t offset = 0;
    for (UINT i = 0; i < records; ++i)
    {
        if (offset > bytes.size() || bytes.size() - offset < sizeof(RAWINPUTHEADER))
            throw std::runtime_error("原始输入缓冲区头部不完整");
        RAWINPUT record{};
        std::memcpy(&record.header, bytes.data() + offset, sizeof(record.header));
        const auto size = record.header.dwSize;
        if (size < sizeof(RAWINPUTHEADER) || size > bytes.size() - offset)
            throw std::runtime_error("原始输入缓冲区长度不合法");
        if (record.header.dwType == RIM_TYPEMOUSE)
        {
            if (size < offsetof(RAWINPUT, data) + sizeof(RAWMOUSE))
                throw std::runtime_error("原始鼠标记录不完整");
            std::memcpy(&record.data.mouse, bytes.data() + offset + offsetof(RAWINPUT, data), sizeof(RAWMOUSE));
            receive(record);
        }
        offset += (static_cast<std::size_t>(size) + 7) & ~std::size_t{7};
    }
}

void RawInputBuffer::drain(const std::function<void(const RAWINPUT &)> &receive)
{
    // Bound each turn so that cancellation and focus checks cannot starve. Any
    // remaining WM_INPUT stays queued for the next message-loop turn.
    for (unsigned turn = 0; turn < 8; ++turn)
    {
        UINT size = static_cast<UINT>(bytes_.size());
        const auto count =
            GetRawInputBuffer(reinterpret_cast<RAWINPUT *>(bytes_.data()), &size, sizeof(RAWINPUTHEADER));
        if (count == static_cast<UINT>(-1))
            throw std::runtime_error("原始输入批量读取失败");
        if (!count)
            return;
        visitRawMouseBuffer(bytes_, count, receive);
    }
}
} // namespace mousebox::desktop
