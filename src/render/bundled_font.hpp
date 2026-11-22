#pragma once
#include <cstddef>
#include <memory>
#include <span>
#include <string>

struct IDWriteFactory;
struct IDWriteFontCollection;
namespace mousebox::render::detail
{
// A private DirectWrite collection constructed from caller-provided font bytes.
class BundledFont
{
    struct Impl;
    std::unique_ptr<Impl> impl_;

  public:
    BundledFont(IDWriteFactory *, std::span<const std::byte>);
    ~BundledFont();
    IDWriteFontCollection *collection() const noexcept;
    const std::wstring &family() const noexcept;
};
} // namespace mousebox::render::detail
