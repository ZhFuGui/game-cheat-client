import { useSearchParams, Navigate } from "react-router";
import { useTranslation } from "react-i18next";
import { AppIcon } from "~/components/icons";
import { General } from "~/features/settings/General";
import { Firmware } from "~/features/settings/Firmware";
import { InputDevice } from "~/features/settings/InputDevice";
import { Rendering } from "~/features/settings/Rendering";
import { DataManagement } from "~/features/settings/DataManagement";
const panes = [
  { id: "general", title: "常规", icon: "tuning", component: General },
  { id: "firmware", title: "固件", icon: "firmware", component: Firmware },
  { id: "input-device", title: "输入", icon: "mouseDevice", component: InputDevice },
  { id: "rendering", title: "绘制", icon: "rendering", component: Rendering },
  { id: "data", title: "数据", icon: "storage", component: DataManagement },
] as const;
const legacyGameSections = new Set(["input", "calibration", "aimbot-algorithm", "aimbot-mouse"]);
export default function Settings() {
  const [params, setParams] = useSearchParams(), { t } = useTranslation();
  const requested = params.get("section");
  const pane = panes.find(p => p.id === requested) ?? panes[0], Pane = pane.component;
  if (requested && legacyGameSections.has(requested)) return <Navigate to={`/game-settings?section=${requested}`} replace />;
  return <div className="settings-page">
    <aside className="settings-navigation"><nav aria-label={t("设置分类")}>{panes.map(({ id, title, icon }) => <button key={id} data-section={id} className={id === pane.id ? "active" : ""} aria-current={id === pane.id ? "page" : undefined} onClick={() => setParams({ section: id })}><AppIcon name={icon} size={16} />{t(title)}</button>)}</nav></aside>
    <div className="settings-content global-settings-content" aria-label={t(pane.title)}><Pane /></div>
  </div>;
}

