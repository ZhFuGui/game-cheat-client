import { useTranslation } from "react-i18next";
import { Section } from "~/components/shared/Content";
import { SettingField } from "~/components/shared/SettingField";

export function DataSettings() {
  const { t } = useTranslation();
  return <>
    <Section title="CS2 数据设置" help={t("使用已核验的 Reader 数据；未知模型、缺失数据或不匹配的姿态不补默认值。") }>
      <SettingField setting="cs2MaxAgeMs" label="完整快照过期上限" suffix="ms" />
      <SettingField setting="cs2MetersPerUnit" label="每原生单位对应的米数" suffix="m"  help={<> <p>{t("比例为 0 表示未设置，不显示米制距离。显式设置后，距离从本地角色眼部到人物实体位置计算。")}</p> </>}/>
    </Section>
  </>;
}
