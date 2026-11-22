import { useTranslation } from "react-i18next";
import { Button } from "~/components/ui/button";
import { Dialog } from "~/components/ui/dialog";
import type { LearningSession } from "./useLearningSession";

export function LearningConfirmation({
  session,
}: {
  session: LearningSession;
}) {
  const { t } = useTranslation();
  const { confirmation, deletion, pending } = session;
  return (
    <Dialog
      open={confirmation !== null}
      onOpenChange={(v) => {
        if (!v) session.dismissConfirmation();
      }}
      title={t(confirmation === "delete" ? "删除当前模型" : "清空采样")}
    >
      <p>
        {t(
          confirmation === "delete"
            ? "删除后当前模型会停用，原始采样保留。"
            : "清空当前采集的场次与片段，已导出的文件保留。",
        )}
      </p>
      {confirmation === "delete" && (
        <code className="delete-path">{deletion?.path}</code>
      )}
      <div className="dialog-actions">
        <Button variant="outline" onClick={session.dismissConfirmation}>
          {t("取消")}
        </Button>
        <Button
          variant="destructive"
          disabled={pending}
          onClick={() => void session.confirm()}
        >
          {t("确认")}
        </Button>
      </div>
    </Dialog>
  );
}
