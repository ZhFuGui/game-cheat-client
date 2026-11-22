import { Suspense } from "react";
import { Navigate, useSearchParams } from "react-router";
import { getGameUiModule } from "~/games/registry";
import { useRuntimeStore } from "~/runtime/state";
import {FeatureSection} from "~/components/shared/FeatureSection";
import {SettingToggle} from "~/components/shared/SettingField";
import {useTranslation} from "react-i18next";

export default function Aimbot() {
  const {t}=useTranslation();
  const params = useSearchParams()[0];
  const gameId = useRuntimeStore(s => s.workspace?.active?.gameId);
  const module = getGameUiModule(gameId);
  const Settings = module?.AimSettings;
  if (params.get("algorithm") === "1")
    return <Navigate to="/game-settings?section=aimbot-algorithm" replace />;
  if (!Settings) return null;
  return (
    <div className="page aimbot-page">
      <div className="aimbot-settings aim-section-stack">
        <FeatureSection title="功能">
          <div className="aim-fields">
            <SettingToggle setting="aimEnabled" label="启用 Aimbot"
              help={t("启用前先关闭其他已开启的 Aim 功能。启用后仍需按住所设侧键进行瞄准；关闭时停止跟随并清除本次锁定。")}/>
          </div>
        </FeatureSection>
        <Suspense fallback={null}><Settings /></Suspense>
      </div>
    </div>
  );
}
