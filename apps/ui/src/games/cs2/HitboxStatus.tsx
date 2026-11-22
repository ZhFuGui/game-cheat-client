import { useTranslation } from "react-i18next";
import { Hint } from "~/components/shared/Content";
import { useRuntimeStore } from "~/runtime/state";

const messages = {
  disabled: "命中体显示已关闭",
  "waiting-data": "等待当前 CS2 数据",
  unsupported: "当前人物没有匹配的已核验命中体资源",
  "waiting-pose": "等待本批有效的骨骼世界姿态和命中体资格",
  partial: "部分命中体姿态缺失，仅显示本批可用部位",
  ready: "命中体资源与当前骨骼姿态已匹配",
};

export function HitboxStatus() {
  const { t } = useTranslation();
  const mode = useRuntimeStore(store => store.values?.cs2HitboxDisplay ?? "off");
  const state = useRuntimeStore(store => store.state.cs2Hitboxes);
  if (mode === "off") return null;
  return <Hint>{t(state ? messages[state.phase] : messages["waiting-data"])}</Hint>;
}
