import { useTranslation } from "react-i18next";
import { Section } from "~/components/shared/Content";
import { Button } from "~/components/ui/button";
import { Switch } from "~/components/ui/switch";
import type { LearningSession } from "./useLearningSession";

export function LearningModel({ session }: { session: LearningSession }) {
  const { t } = useTranslation();
  const { learning, busy } = session;
  return (
    <Section title="速度模型" help={<> <p>{t(
          "轻微动作变化仅在启用速度模型后参与主动接近；接近目标及快速纠偏时不叠加。",
        )}</p><p>{t(
          "学习模型仅限制主动接近的速度、加速和制动；锁定后的快速纠偏不使用学习上限。",
        )}</p><p>{t("载入或拟合模型不会自动启用。")}</p> </>}>
      <p className="model-description">
        {learning?.model || t("尚无速度模型。载入或拟合后可手动启用。")}
      </p>
      <div className="actions model-actions">
        <Button
          variant="outline"
          disabled={busy || !learning?.canApply}
          onClick={() => void session.save("model")}
        >
          {t("导出模型")}
        </Button>
        <Button
          variant="outline"
          disabled={busy}
          onClick={() => void session.openModel()}
        >
          {t("载入模型")}
        </Button>
        <Button
          variant="destructive"
          disabled={busy || (!learning?.modelPath && !learning?.canApply)}
          onClick={() => void session.requestDelete()}
        >
          {t("删除当前模型")}
        </Button>
        <Button
          className="model-activate"
          disabled={busy || !learning?.canApply}
          onClick={() => void session.toggleActive()}
        >
          {t(learning?.active ? "恢复默认速度" : "使用速度模型")}
        </Button>
      </div>
      <label className="toggle-row">
        <span>{t("轻微动作变化 · 接近目标自动归零")}</span>
        <Switch
          checked={learning?.variation ?? false}
          disabled={busy || !learning?.active}
          onCheckedChange={(variation) => void session.setVariation(variation)}
        />
      </label>
    </Section>
  );
}
