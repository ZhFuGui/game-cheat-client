import { useEffect, useLayoutEffect } from "react";
import i18n from "~/i18n";
import { useRuntimeStore } from "~/runtime/state";
import { invoke } from "~/lib/desktopApi";
import { getThemePreset } from "~/theme/presets";
export function useTheme() {
  const mode = useRuntimeStore((s) => s.preferences.colorMode),
    preset = useRuntimeStore((s) => getThemePreset(s.preferences.themePreset)),
    language = useRuntimeStore((s) => s.preferences.language),
    ready = useRuntimeStore((s) => s.ready),
    catalog = useRuntimeStore((s) => s.workspace),
    context = catalog?.active;
  useLayoutEffect(() => {
    const root = document.documentElement;
    root.dataset.themePreset = preset.id;
    root.classList.toggle("dark", mode === "dark");
    root.style.colorScheme = mode;
  }, [mode, preset]);
  useEffect(() => {
    if (!ready || (catalog && !context)) return;
    const css = getComputedStyle(document.documentElement);
    const get = (key: string) => css.getPropertyValue(key).trim();
    void invoke("surface.palette", {
      canvas: get("--canvas"),
      grid: get("--canvas-grid"),
      guide: get("--canvas-guide"),
      text: get("--canvas-text"),
      entity: get("--entity"),
      selected: get("--selected"),
      language,
    }, context).catch(() => {});
  }, [mode, language, ready, catalog, context]);
  useEffect(() => {
    void i18n.changeLanguage(language);
    document.documentElement.lang = language === "zh" ? "zh-CN" : "en";
  }, [language]);
}
