#pragma once
#include <cstdint>
namespace mousebox::design {
// Native test palette, in sRGB 0xRRGGBB. Production receives the resolved
// semantic canvas colors from the Web theme through surface.palette.
struct ThemeTokens {
    std::uint32_t background=0xffffff,foreground=0x09090b;
    std::uint32_t card=0xffffff,cardForeground=0x09090b,popover=0xffffff,popoverForeground=0x09090b;
    std::uint32_t primary=0x18181b,primaryForeground=0xfafafa;
    std::uint32_t secondary=0xf4f4f5,secondaryForeground=0x18181b;
    std::uint32_t muted=0xf4f4f5,mutedForeground=0x71717a,accent=0xf4f4f5,accentForeground=0x18181b;
    std::uint32_t border=0xe4e4e7,input=0xe4e4e7,ring=0x18181b;
    std::uint32_t destructive=0xef4444,success=0x2a9d8f,warning=0xe9c46a;
    std::uint32_t sidebar=0xfbfbfb,sidebarForeground=0x3f3f46,sidebarBorder=0xf4f4f5,statusbar=0xf4f4f5,brand=0xe76e50;
    std::uint32_t sidebarPrimary=0x18181b,sidebarPrimaryForeground=0xfafafa;
    std::uint32_t sidebarAccent=0xf4f4f5,sidebarAccentForeground=0x18181b,sidebarRing=0xd4d4d8;
    std::uint32_t canvas=0xffffff,canvasGrid=0xf4f4f5,canvasGuide=0xd4d4d8;
    std::uint32_t entity=0x2a9d8f,selectedEntity=0xe76e50;
    double radius=12.8;
    bool operator==(const ThemeTokens&) const=default;
};
inline constexpr ThemeTokens LightTheme{};
inline constexpr ThemeTokens DarkTheme=[] {
    ThemeTokens t;
    t.background=t.card=t.popover=t.canvas=0x09090b;t.foreground=t.cardForeground=t.popoverForeground=0xfafafa;
    t.primary=0xfafafa;t.primaryForeground=0x18181b;
    t.secondary=t.muted=t.accent=t.border=t.input=0x27272a;t.secondaryForeground=t.accentForeground=0xfafafa;
    t.mutedForeground=0xa1a1aa;t.ring=0xd4d4d8;t.destructive=0x7f1d1d;
    t.sidebar=t.statusbar=t.canvasGrid=0x18181b;t.sidebarForeground=0xa1a1aa;t.sidebarBorder=0x27272a;
    t.sidebarPrimary=0x3b82f6;t.sidebarPrimaryForeground=0xffffff;t.canvasGuide=0x3f3f46;
    t.sidebarAccent=0x27272a;t.sidebarAccentForeground=0xfafafa;t.sidebarRing=0x1d4ed8;
    return t;
}();
}
