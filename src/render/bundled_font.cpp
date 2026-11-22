#include "bundled_font.hpp"
#include "directx_error.hpp"
#include <dwrite_3.h>
#include <wrl/client.h>
#include <limits>
#include <stdexcept>
#include <vector>

namespace mousebox::render::detail
{
using Microsoft::WRL::ComPtr;
struct BundledFont::Impl
{
    ComPtr<IDWriteFactory5> factory;
    ComPtr<IDWriteInMemoryFontFileLoader> loader;
    ComPtr<IDWriteFontCollection1> collection;
    std::wstring family;
    bool registered = false;

    ~Impl()
    {
        collection.Reset();
        if (registered)
            factory->UnregisterFontFileLoader(loader.Get());
    }
    void initialize(IDWriteFactory *source, std::span<const std::byte> bytes)
    {
        check(source->QueryInterface(IID_PPV_ARGS(&factory)), "Bundled font factory");
        if (bytes.empty() || bytes.size() > std::numeric_limits<UINT32>::max())
            throw std::runtime_error("Bundled primary font resource is missing or invalid");
        check(factory->CreateInMemoryFontFileLoader(&loader), "Bundled font loader");
        check(factory->RegisterFontFileLoader(loader.Get()), "Register bundled font loader");
        registered = true;
        ComPtr<IDWriteFontFile> file;
        // DirectWrite copies the bytes when the memory owner is null.
        check(loader->CreateInMemoryFontFileReference(factory.Get(), bytes.data(), static_cast<UINT32>(bytes.size()),
                                                      nullptr, &file),
              "Bundled font file");
        ComPtr<IDWriteFontSetBuilder1> builder;
        check(factory->CreateFontSetBuilder(&builder), "Bundled font set builder");
        check(builder->AddFontFile(file.Get()), "Bundled font face");
        ComPtr<IDWriteFontSet> set;
        check(builder->CreateFontSet(&set), "Bundled font set");
        check(factory->CreateFontCollectionFromFontSet(set.Get(), &collection), "Bundled font collection");
        if (collection->GetFontFamilyCount() != 1)
            throw std::runtime_error("Bundled primary font must contain one family");
        ComPtr<IDWriteFontFamily> first;
        check(collection->GetFontFamily(0, &first), "Bundled font family");
        ComPtr<IDWriteLocalizedStrings> names;
        check(first->GetFamilyNames(&names), "Bundled font names");
        UINT32 index = 0, length = 0;
        BOOL exists = FALSE;
        check(names->FindLocaleName(L"zh-cn", &index, &exists), "Bundled font locale");
        if (!exists)
            index = 0;
        check(names->GetStringLength(index, &length), "Bundled font name length");
        std::vector<wchar_t> name(static_cast<std::size_t>(length) + 1);
        check(names->GetString(index, name.data(), length + 1), "Bundled font name");
        family.assign(name.data(), length);
    }
};
BundledFont::BundledFont(IDWriteFactory *factory, std::span<const std::byte> bytes) : impl_(std::make_unique<Impl>())
{
    impl_->initialize(factory, bytes);
}
BundledFont::~BundledFont() = default;
IDWriteFontCollection *BundledFont::collection() const noexcept
{
    return impl_->collection.Get();
}
const std::wstring &BundledFont::family() const noexcept
{
    return impl_->family;
}
} // namespace mousebox::render::detail
