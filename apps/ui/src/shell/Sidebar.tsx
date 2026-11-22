import { NavLink } from "react-router";
import { AppIcon } from "~/components/icons";
import { useTranslation } from "react-i18next";
import { useRuntimeStore } from "~/runtime/state";
import { GameSelector } from "~/games/GameSelector";
import { SidebarResizeHandle } from "~/features/navigation/SidebarResizeHandle";
import {NavigationHelp} from "~/features/navigation/NavigationHelp";
import {getGameUiModule} from "~/games/registry";
import { visualsPages } from "~/navigation/pageGroups";
const main = [
  { to: "/", label: "READER", icon: "connection" },
  { to: visualsPages.route, label: visualsPages.label, icon: "esp" },
  { to: "/aim", label: "Aim", icon: "aimbot" },
  { to: "/recoil", label: "Recoil", icon: "recoil" },
  { to: "/game-settings", label: "游戏设置", icon: "tuning" },
] as const;
const tools = [
  { to: "/mouse-debug", label: "鼠标移动调试", icon: "mouseDevice" },
  { to: "/logs", label: "运行日志", icon: "logs" },
  { to: "/settings", label: "设置", icon: "settings" },
] as const;
export function Sidebar() {
  const { t } = useTranslation(), version = useRuntimeStore(s => s.version);
  const catalog = useRuntimeStore(s => s.workspace);
  const help=getGameUiModule(catalog?.active?.gameId)?.navigationHelp;
  const capabilities = catalog?.games.find(g => g.id === catalog.active?.gameId)?.capabilities ?? [];
  return <aside className="sidebar" id="primary-sidebar">
    <header className="sidebar-header drag-region">
      <GameSelector />
      <span className="sidebar-version">{version ? `v${version}` : ""}</span>
    </header>
    <nav aria-label={t("主导航")}>
      <div className="navigation-group">{main.filter(item => item.to !== visualsPages.route || capabilities.includes("esp")).filter(item => item.to !== "/aim" || capabilities.some(capability=>["aim","triggerbot","rage"].includes(capability))).filter(item => item.to !== "/recoil" || capabilities.includes("recoil")).map(({ to, label, icon }) => <div key={to} className="nav-entry"><NavLink to={to} end={to === "/"} title={t(label)} aria-label={t(label)} className={({ isActive }) => `nav-item ${help?.[to]?"has-help":""} ${isActive ? "active" : ""}`}><AppIcon name={icon} size={18} /><span>{t(label)}</span></NavLink>{help?.[to]&&<NavigationHelp page={label} content={help[to]}/>}</div>)}</div>
      <div className="navigation-bottom">{tools.map(({ to, label, icon }) => <NavLink key={to} to={to} title={t(label)} aria-label={t(label)} className={({ isActive }) => `nav-item ${isActive ? "active" : ""}`}><AppIcon name={icon} size={18} /><span>{t(label)}</span></NavLink>)}</div>
    </nav>
    <SidebarResizeHandle />
  </aside>;
}

