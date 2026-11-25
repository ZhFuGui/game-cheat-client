import {resolvePageGroup,type PageGroup} from "~/framework/pages/model";
import {readerPages,mousePages,recoilPages,aimPages,visualsPages} from "~/navigation/pageGroups";
export type ContextPage =
  | "reader" | "dashboard" | "visuals-general" | "esp" | "radar" | "aim-general" | "aimbot" | "triggerbot" | "rage" | "recoil" | "learning" | "reports" | "logs"
  | "general" | "firmware" | "input" | "algorithm" | "movement"
  | "calibration" | "rendering" | "input-device" | "data" | "cs2-source" | "valorant-display" | "unknown";
export type ContextTone = "error" | "warning" | "success" | "info" | "quiet";
export interface ContextCopy {
  key: string;
  values?: Record<string, string | number>;
}
export interface ContextMessage {
  /** Condition identities exclude counters, timestamps and translated text. */
  id: string;
  kind: "normal" | "activity" | "fault";
  priority: number;
  tone: ContextTone;
  text: ContextCopy;
  detail?: ContextCopy;
}
export function copy(key: string, values?: ContextCopy["values"]): ContextCopy {
  return values ? { key, values } : { key };
}
export function message(id: string, text: string, detail?: ContextCopy, tone: ContextTone = "quiet"): ContextMessage {
  return { id, kind: "normal", priority: 0, tone, text: copy(text), detail };
}
export function activity(id: string, text: string, detail?: ContextCopy): ContextMessage {
  return { ...message(id, text, detail, "info"), kind: "activity", priority: 10 };
}
export function fault(id: string, text: string, detail: string | undefined, priority = 50, tone: ContextTone = "error"): ContextMessage {
  return { ...message(id, text, detail ? copy(detail) : undefined, tone), kind: "fault", priority };
}
const settingsPages: Record<string, ContextPage> = {
  general: "general", firmware: "firmware", input: "input",
  "aimbot-algorithm": "algorithm", "aimbot-mouse": "movement",
  calibration: "calibration", rendering: "rendering",
  "input-device": "input-device", data: "data",
  "cs2-source": "cs2-source", "valorant-display": "valorant-display",
};
export function contextPage(path: string, search: string,visuals:PageGroup=visualsPages,aim:PageGroup=aimPages,gameSettingsSection?:string): ContextPage {
  const section = new URLSearchParams(search).get("section");
  switch (path) {
    case "/": return resolvePageGroup(readerPages,section).context as ContextPage;
    case "/dashboard": return "dashboard";
    case "/esp": return "esp";
    case "/visuals": return resolvePageGroup(visuals,section).context as ContextPage;
    case "/aim": return resolvePageGroup(aim,section).context as ContextPage;
    case "/aimbot": return "aimbot";
    case "/triggerbot": return "triggerbot";
    case "/rage": return "rage";
    case "/recoil": return resolvePageGroup(recoilPages,section).context as ContextPage;
    case "/learning": return "learning";
    case "/mouse-debug": return resolvePageGroup(mousePages,section).context as ContextPage;
    case "/logs": return "logs";
    case "/settings": return settingsPages[section ?? "general"] ?? "general";
    case "/game-settings": return settingsPages[gameSettingsSection ?? section ?? "input"] ?? "input";
    default: return "unknown";
  }
}
