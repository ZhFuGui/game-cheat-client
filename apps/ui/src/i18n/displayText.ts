import { english } from "./index";
const escape = (text: string) => text.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
interface Rule {
  pattern: RegExp;
  slots: string[];
  replacement: string;
  size: number;
}
function rules(reverse: boolean): Rule[] {
  return Object.entries(english)
    .map(([zh, en]) => {
      const source = reverse ? en : zh,
        replacement = reverse ? zh : en,
        slots: string[] = [];
      let offset = 0,
        pattern = "^",
        literalSize = source.length;
      for (const match of source.matchAll(/%([1-9][0-9]*)/g)) {
        pattern += escape(source.slice(offset, match.index)) + "(.*?)";
        offset = match.index! + match[0].length;
        slots.push(match[0]);
        literalSize -= match[0].length;
      }
      return {
        pattern: new RegExp(pattern + escape(source.slice(offset)) + "$", "s"),
        slots,
        replacement,
        size: literalSize,
      };
    })
    .sort((a, b) => b.size - a.size);
}
const forward = rules(false),
  backward = rules(true);
const cache = { zh: new Map<string, string>(), en: new Map<string, string>() };
export function displayText(
  text: string,
  language: "zh" | "en",
  depth = 0,
): string {
  if (depth >= 8 || !text) return text;
  const remembered = cache[language].get(text);
  if (remembered !== undefined) return remembered;
  const remember = (result: string) => {
    if (cache[language].size >= 2048)
      cache[language].delete(cache[language].keys().next().value!);
    cache[language].set(text, result);
    return result;
  };
  if (language === "en" && english[text]) return english[text];
  for (const rule of language === "en" ? forward : backward) {
    const match = text.match(rule.pattern);
    if (!match) continue;
    let output = rule.replacement;
    for (let index = rule.slots.length - 1; index >= 0; index--)
      output = output
        .split(rule.slots[index])
        .join(displayText(match[index + 1], language, depth + 1));
    return remember(output);
  }
  // Native summaries combine independently translated facts and status codes.
  // Keep the original separators, paths and numeric values intact.
  if (text.includes("\n"))
    return remember(
      text
        .split("\n")
        .map((part) => displayText(part, language, depth + 1))
        .join("\n"),
    );
  if (text.includes(" · "))
    return remember(
      text
        .split(" · ")
        .map((part) => displayText(part, language, depth + 1).trim())
        .join(" · "),
    );
  return remember(text);
}
export function localizeDisplay<T>(value: T, language: "zh" | "en"): T {
  if (typeof value === "string") return displayText(value, language) as T;
  if (Array.isArray(value))
    return value.map((v) => localizeDisplay(v, language)) as T;
  if (value && typeof value === "object")
    return Object.fromEntries(
      Object.entries(value).map(([key, v]) => [
        key,
        // Phase values are typed machine codes; only their message is localized.
        key === "phase" ? v : localizeDisplay(v, language),
      ]),
    ) as T;
  return value;
}
