import { useEffect, useState } from "react";

// Visibility and keyboard ownership stay mounted while the settings panel is hidden.
export function useEspPanel() {
  const [panel, setPanel] = useState(true);
  const [tab, setTab] = useState("display");
  useEffect(() => {
    const key = (e: KeyboardEvent) => {
      const editing =
        e.target instanceof Element &&
        !!e.target.closest(
          "input,textarea,[contenteditable=true],[role=combobox]",
        );
      if (e.key === "Insert" && !e.repeat && !editing) {
        setPanel((v) => !v);
        e.preventDefault();
      }
    };
    window.addEventListener("keydown", key);
    return () => window.removeEventListener("keydown", key);
  }, []);
  return { panel, tab, setTab, togglePanel: () => setPanel((value) => !value) };
}
