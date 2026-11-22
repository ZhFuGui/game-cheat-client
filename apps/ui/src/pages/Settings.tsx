import { useSearchParams, Navigate } from "react-router";
import { useTranslation } from "react-i18next";
import { AppIcon } from "~/components/icons";
import { General } from "~/features/settings/General";
import { Firmware } from "~/features/settings/Firmware";
import { InputDevice } from "~/features/settings/InputDevice";
import { Rendering } from "~/features/settings/Rendering";
import { DataManagement } from "~/features/settings/DataManagement";
const panes = [
  { id: "general", title: "常规", description: "主题、语言与窗口偏好，在所有游戏间共享。", icon: "tuning", component: General },
  { id: "firmware", title: "固件", description: "管理串口、盒子与设备能力，独立于当前游戏。", icon: "firmware", component: Firmware },
  { id: "input-device", title: "输入设备", description: "DPI 与本机鼠标诊断；游戏灵敏度在游戏设置中管理。", icon: "mouseDevice", component: InputDevice },
  { id: "rendering", title: "显卡与绘制", description: "共享绘制设备，按数据到达更新，不设软件 FPS 上限。", icon: "rendering", component: Rendering },
  { id: "data", title: "数据管理", description: "查看游戏配置、公共资产与迁移数据的保存范围。", icon: "storage", component: DataManagement },
] as const;
const legacyGameSections = new Set(["input", "calibration", "aimbot-algorithm", "aimbot-mouse"]);
export default function Settings() {
  const [params, setParams] = useSearchParams(), { t } = useTranslation();
  const requested = params.get("section");
  const pane = panes.find(p => p.id === requested) ?? panes[0], Pane = pane.component;
  if (requested && legacyGameSections.has(requested)) return <Navigate to={`/game-settings?section=${requested}`} replace />;
  return <div className="settings-page">
    <aside className="settings-navigation"><span>{t("设置")}</span><nav aria-label={t("设置分类")}>{panes.map(({ id, title, icon }) => <button key={id} data-section={id} className={id === pane.id ? "active" : ""} aria-current={id === pane.id ? "page" : undefined} onClick={() => setParams({ section: id })}><AppIcon name={icon} size={16} />{t(title)}</button>)}</nav></aside>
    <div className="settings-content" aria-label={t(pane.title)}><Pane /></div>
  </div>;
}

