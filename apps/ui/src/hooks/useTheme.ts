import { useEffect } from "react";
import i18n from "~/i18n";
import { useRuntimeStore } from "~/runtime/state";
import { invoke } from "~/lib/desktopApi";
export function useTheme() {
  const mode = useRuntimeStore((s) => s.preferences.colorMode),
    language = useRuntimeStore((s) => s.preferences.language),
    ready = useRuntimeStore((s) => s.ready),
    catalog = useRuntimeStore((s) => s.workspace),
    context = catalog?.active;
  useEffect(() => {
    const apply = () => {
      const dark = mode === "dark";
      document.documentElement.classList.toggle("dark", dark);
      document.documentElement.style.colorScheme = dark ? "dark" : "light";
      if (ready && (!catalog || context)) {
        const css = getComputedStyle(document.documentElement);
        const get = (key: string) => css.getPropertyValue(key).trim();
        void invoke("surface.palette", {
          canvas: get("--canvas"),
          grid: get("--canvas-grid"),
          guide: get("--canvas-guide"),
          text: get("--muted-foreground"),
          entity: get("--entity"),
          selected: get("--selected"),
          language,
        }, context).catch(() => {});
      }
    };
    apply();
  }, [mode, language, ready, catalog, context]);
  useEffect(() => {
    void i18n.changeLanguage(language);
    document.documentElement.lang = language === "zh" ? "zh-CN" : "en";
  }, [language]);
}
