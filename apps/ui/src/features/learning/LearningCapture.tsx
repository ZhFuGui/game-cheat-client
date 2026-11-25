import { useTranslation } from "react-i18next";
import { Button } from "~/components/ui/button";
import { Input } from "~/components/ui/input";
import { SpeedCanvas } from "./SpeedCanvas";
import type { LearningSession } from "./useLearningSession";

export function LearningCapture({ session }: { session: LearningSession }) {
  const { t } = useTranslation();
  const { learning, busy, pending, recording, dpi, dpiError } = session;
  return (
    <>
      <div className="learning-controls">
        <div className="field dpi-field">
          <label htmlFor="capture-dpi">{t("采集鼠标 DPI")}</label>
          <Input
            id="capture-dpi"
            value={dpi}
            inputMode="numeric"
            disabled={busy}
            onChange={(e) => session.changeDpi(e.target.value)}
            onBlur={session.commitDpi}
            aria-invalid={dpiError}
          />
          {dpiError && (
            <small className="field-error">{t("请输入100至32000的整数")}</small>
          )}
        </div>
        <Button
          disabled={!learning || busy || pending}
          onClick={() => void session.start()}
        >
          {t("开始采集")}
        </Button>
        <Button
          variant="outline"
          disabled={!recording}
          onClick={() => void session.pause()}
        >
          {t("Esc 暂停")}
        </Button>
        <Button
          variant="ghost"
          disabled={busy || !learning?.trials}
          onClick={() => void session.replay()}
        >
          {t("回放最后一段")}
        </Button>
      </div>
      <SpeedCanvas />
      <div className="capture-summary">
        {learning?.status && <p>{learning.status}</p>}
        <span>
          {t("独立场次")} <strong>{learning?.sessions ?? "—"}</strong> ·{" "}
          {t("已保存段")} <strong>{learning?.trials ?? "—"}</strong> ·{" "}
          {t("样本")} <strong>{learning?.samples ?? "—"}</strong>
        </span>
      </div>
      <div className="actions sampling-actions">
        <Button
          variant="outline"
          disabled={busy || !learning?.trials}
          onClick={() => void session.save("samples")}
        >
          {t("导出采样")}
        </Button>
        <Button
          variant="outline"
          disabled={busy}
          onClick={() => void session.openSamples()}
        >
          {t("载入采样")}
        </Button>
        <Button
          variant="ghost"
          disabled={busy || !learning?.trials}
          onClick={() => session.requestClear()}
        >
          {t("清空采样")}
        </Button>
        <Button
          disabled={busy || !learning || pending}
          onClick={() => void session.fit()}
        >
          {t(learning?.training ? "正在拟合…" : "拟合速度与验证")}
        </Button>
      </div>
      {learning?.feedback && <p className="fit-feedback" role="status">{learning.feedback}</p>}
    </>
  );
}
