import {readerHref} from "~/navigation/readerLocation";
import {aimHref,legacyAimHref} from "~/navigation/aimLocation";
import {pageHref} from "~/framework/pages/model";
import {visualsPages} from "~/navigation/pageGroups";
const pages = new Set([
  "overview",
  "reader",
  "dashboard",
  "esp",
  "visuals",
  "aimbot",
  "aim",
  "triggerbot",
  "rage",
  "recoil",
  "learning",
  "mouse-debug",
  "logs",
  "settings",
  "game-settings",
]);
const sections = new Set([
  "general",
  "firmware",
  "input",
  "calibration",
  "rendering",
  "aimbot-algorithm",
  "aimbot-mouse",
  "input-device",
  "data",
]);
export function startupRoute(search: string): string {
  const params = new URLSearchParams(search),
    page = params.get("page") || "overview";
  if (!pages.has(page)) return "/";
  if (page === "esp") return visualsPages.route;
  if (page === "visuals") return pageHref(visualsPages, params.get("section") ?? "");
  if(page==="aim")return aimHref(params.get("section"));
  if(page==="aimbot"||page==="triggerbot"||page==="rage")return legacyAimHref(`/${page}`,search);
  if(page==="overview"||page==="reader"||page==="dashboard")return readerHref(page==="dashboard"?"dashboard":params.get("section")??"",params.get("view"));
  if(page==='learning')return '/mouse-debug?section=learning';
  if(page==='mouse-debug')return '/mouse-debug?section='+(params.get('section')==='characteristics'?'characteristics':'learning');
  if (page === "settings" || page === "game-settings") {
    const section = params.get("section");
    return (
      `/${page}` +
      (section && sections.has(section) ? `?section=${section}` : "")
    );
  }
  return `/${page}`;
}
