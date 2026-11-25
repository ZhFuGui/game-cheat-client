import { Suspense,useState } from "react";
import { Link } from "react-router";
import { useTranslation } from "react-i18next";
import { AppIcon } from "~/components/icons";
import { Button } from "~/components/ui/button";
import { EspStatus } from "~/features/esp/EspStatus";
import { useEspPanel } from "~/features/esp/useEspPanel";
import { useNativeSurface } from "~/hooks/useNativeSurface";
import { useRuntimeStore } from "~/runtime/state";
import { getGameUiModule } from "~/games/registry";
import { EspOutputSettings,EspOverlayPlaceholder } from "~/features/esp/EspOutputSettings";

export default function Esp() {
  const { t } = useTranslation();
  const [outputOpen,setOutputOpen]=useState(false);
  const preferredPlacement=useRuntimeStore(s=>s.preferences.espPlacement??"embedded");
  const { panel, tab, setTab, togglePanel } = useEspPanel();
  const ref = useNativeSurface("esp");
  const sequences = useRuntimeStore((state) => state.state.espSequences);
  const fov = useRuntimeStore((state) => state.state.espFov);
  const gameId = useRuntimeStore(state => state.workspace?.active?.gameId);
  const game = getGameUiModule(gameId);
  const Settings = game?.EspSettings;
  const outputEnabled=game?.externalEsp===true;
  const placement=outputEnabled?preferredPlacement:"embedded";
  return (
    <div className="esp-page">
      <div className="esp-toolbar">
        <div className="esp-toolbar-actions">
          <Button variant="outline" size="sm" aria-pressed={panel} onClick={togglePanel}>
            <AppIcon name={panel ? "hidePanel" : "showPanel"} size={15} />
            {t("设置")}
            <kbd>Insert</kbd>
          </Button>
          {outputEnabled&&<Button variant="outline" size="sm" data-esp-output-toggle aria-expanded={outputOpen} onClick={()=>setOutputOpen(v=>!v)}>
            <AppIcon name="rendering" size={15} />{t("输出")}
          </Button>}
        </div>
        <EspStatus />
      </div>
      {outputEnabled&&outputOpen&&<EspOutputSettings/>}
      <div className="esp-body">
        <Suspense fallback={null}>{panel && Settings && <Settings tab={tab} setTab={setTab} />}</Suspense>
        <div
          ref={ref}
          className="native-surface"
          aria-label={t("DirectX ESP 画布")}
          data-native-surface="esp"
        >{placement==="overlay"&&<EspOverlayPlaceholder/>}</div>
      </div>
      <div className="esp-footer">
        {sequences && <code>{sequences}</code>}
        {fov && <span>{fov}</span>}
        <Link to="/"><AppIcon name="connection" size={14} />{t("Reader 连接")}</Link>
      </div>
    </div>
  );
}
