import { useTranslation } from "react-i18next";
import { useRuntimeStore } from "~/runtime/state";
import { Hint } from "~/components/shared/Content";

const labels = {
  disabled: "扩展未启用", waiting: "扩展等待就绪条件", loading: "扩展正在加载",
  ready: "扩展已就绪", degraded: "扩展部分结果未确定", error: "扩展发生错误", paused: "扩展已暂停",
};
export function ExtensionStatus({ id }: { id: string }) {
  const { t } = useTranslation();
  const state = useRuntimeStore(store => store.state.extensions?.find(entry => entry.id === id));
  return <div role="status" data-extension-id={id} data-extension-phase={state?.phase ?? "unknown"}>
    <Hint>{t(state ? labels[state.phase] : "等待扩展状态")}{state?.message ? ` · ${t(state.message)}` : ""}</Hint>
  </div>;
}
