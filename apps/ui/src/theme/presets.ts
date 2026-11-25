export const themePresets = [
  { id: "default", label: "默认" },
  { id: "warm-gray", label: "暖灰" },
] as const;

export function getThemePreset(id: string | undefined) {
  return themePresets.find(preset => preset.id === id) ?? themePresets[0];
}
