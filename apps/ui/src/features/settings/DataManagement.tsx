import { Link } from "react-router";
import { useTranslation } from "react-i18next";
import { Section } from "~/components/shared/Content";
import { useRuntimeStore } from "~/runtime/state";
export function DataManagement() {
  const { t } = useTranslation(), catalog = useRuntimeStore(s => s.workspace);
  return <>
    <Section title="保存位置" help={<> <p>{t("每台电脑按当前 Windows 用户解析本地应用数据目录，不使用固定用户名或项目路径。")}</p> </>}><code>%LOCALAPPDATA%\Mousebox\</code></Section>
    <Section title="配置范围" help={<> <p>{t("每个游戏保留一份配置；旧配置迁移前备份，原始文件和采样、模型均保留。")}</p> </>}><dl className="details-grid">
      <div><dt>{t("全局")}</dt><dd>{t("主题、语言、窗口、DPI、显卡与盒子连接。")}</dd></div>
      <div><dt>{t("游戏")}</dt><dd>{t("数据源、灵敏度、ESP、跟随参数、弹道、校准与模型启用关系。")}</dd></div>
      <div><dt>{t("公共资产")}</dt><dd>{t("原始采样和模型文件保留原路径与内容；不会跨游戏自动启用。")}</dd></div>
    </dl></Section>
    <Section title="当前游戏"><p>{catalog?.games.find(game => game.id === catalog.active?.gameId)?.name ?? "—"}</p><div className="actions"><Link to="/mouse-debug?section=learning">{t("管理采样与模型")}</Link></div></Section>
  </>;
}

