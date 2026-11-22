import {RecoilScheme} from "~/features/aim/common/RecoilScheme";
import {useTranslation} from "react-i18next";

export function PubgRecoilScheme() {
  const {t} = useTranslation();
  return <RecoilScheme name="原生发射参考"
    help={<p>{t("使用 Reader 提供并通过当前资格校验的原生发射方向，后坐相关量随该参考求值。")}</p>}
    factors={[{title: "原生发射方向",
      detail: "使用当前已核验的原生发射参考及后坐字段，完整来源与时间资格由原生运行时核验。"}]}/>;
}
