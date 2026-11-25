import { Link } from "react-router";
import { useTranslation } from "react-i18next";
import { Section } from "~/components/shared/Content";
import { useRuntimeStore } from "~/runtime/state";
export function DataManagement() {
  const { t } = useTranslation(), catalog = useRuntimeStore(s => s.workspace);
  const game = catalog?.games.find(entry => entry.id === catalog.active?.gameId);
  return <Section title="存储" help={<>
    <p>{t("主题、窗口和设备设置全局共享。数据源、灵敏度、绘制、校准和模型启用状态按游戏保存。")}</p>
    <p>{t("每个游戏保留一份配置；旧配置迁移前备份，原始文件和采样、模型均保留。")}</p>
    <p>{t("原始采样和模型文件保留原路径与内容；不会跨游戏自动启用。")}</p>
  </>}>
    <dl className="settings-facts">
      <div><dt>{t("保存位置")}</dt><dd><code>%LOCALAPPDATA%\Mousebox\</code></dd></div>
      {game && <div><dt>{t("当前游戏")}</dt><dd>{game.name}</dd></div>}
    </dl>
    <div className="actions"><Link to="/mouse-debug?section=learning">{t("采样与模型")}</Link></div>
  </Section>;
}

