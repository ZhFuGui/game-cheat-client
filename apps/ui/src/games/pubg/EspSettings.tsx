import { useTranslation } from "react-i18next";
import {
  SettingChoice,
  SettingField,
  SettingToggle,
} from "~/components/shared/SettingField";
import { Button } from "~/components/ui/button";
import { Hint } from "~/components/shared/Content";
import { useCommand } from "~/hooks/useCommand";
import { useRuntimeStore } from "~/runtime/state";
import type { GameEspSettingsProps } from "../contracts";

export function EspSettings({
  tab,
  setTab,
}: GameEspSettingsProps) {
  const { t } = useTranslation();
  const { run } = useCommand();
  const marker = useRuntimeStore((state) => state.espValues?.marker);
  const units = useRuntimeStore((state) => state.state.espUnits);
  return (
    <aside className="esp-settings" aria-label={t("ESP 配置")}>
      <div className="tabs" role="group" aria-label={t("视图")}>
        <button
          className={tab === "display" ? "active" : ""}
          aria-pressed={tab === "display"}
          onClick={() => setTab("display")}
        >
          {t("显示")}
        </button>
        <button
          className={tab === "projection" ? "active" : ""}
          aria-pressed={tab === "projection"}
          onClick={() => setTab("projection")}
        >
          {t("投影")}
        </button>
      </div>
      <div className="esp-settings-scroll">
        {tab === "display" ? (
          <>
            <SettingChoice
              esp
              setting="marker"
              label="标记样式"
              options={[
                { label: "定位角标", value: "corners" },
                { label: "定位点", value: "point" },
                { label: "高度估算框", value: "estimated" },
              ]}
            />
            <h3>{t("叠加信息")}</h3>
            {(
              [
                "names",
                "distance",
                "bones",
                "tracers",
                "crosshair",
                "aimRange",
                "targetHighlight",
              ] as const
            ).map((setting, index) => (
              <SettingToggle
                key={setting}
                esp
                setting={setting}
                label={
                  [
                    "名称",
                    "距离",
                    "骨骼",
                    "射线",
                    "中心准星",
                    "瞄准范围",
                    "目标高亮",
                  ][index]
                }
              />
            ))}
            <hr />
            <h3>{t("人物筛选")}</h3>
            <SettingField
              esp
              setting="maximumDistanceMeters"
              label="显示距离上限"
              suffix="m"
             help={<> <p>{t("0 表示不限；启用限制时，隐藏无法确认距离的人物。")}</p><p>{t(
                marker === "estimated"
                  ? "高度估算框，不是 Reader 人体边界"
                  : "定位标记，不是人体边界",
              )}</p> </>}/>
            {(["teammates", "opponents", "bots", "self"] as const).map(
              (setting, index) => (
                <SettingToggle
                  key={setting}
                  esp
                  setting={setting}
                  label={["队友", "其他玩家", "人机", "自己"][index]}
                />
              ),
            )}
          </>
        ) : (
          <>
            <SettingChoice
              esp
              setting="aspect"
              label="投影视口"
              options={["16:9", "16:10", "21:9", "4:3", "viewport"].map(
                (value) => ({
                  value,
                  label: value === "viewport" ? "填满区域" : value,
                }),
              )}
            />
            <SettingChoice
              esp
              setting="fovAxis"
              label="FOV 解释"
              options={[
                { label: "水平 FOV", value: "horizontal" },
                { label: "垂直 FOV", value: "vertical" },
              ]}
             help={<> <p>{t("高度框以实体坐标为中心估算，不代表真实身高、姿态或胶囊体。")}</p><p>{t(
                "宽高比与 FOV 方向为手动假设，Reader 未提供游戏视口或最终投影矩阵。",
              )}</p> </>}/>
            {marker === "estimated" && (
              <SettingField
                esp
                setting="estimatedHeightMeters"
                label="估算高度"
                suffix="m"
              />
            )}
            <Hint>{units || "—"}</Hint>
          </>
        )}
      </div>
      <div className="esp-panel-footer">
        <Button
          variant="outline"
          className="full-width"
          onClick={() => void run("esp.reset", {})}
        >
          {t("恢复默认")}
        </Button>
      </div>
    </aside>
  );
}
