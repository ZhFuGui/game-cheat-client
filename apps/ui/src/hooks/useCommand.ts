import { useCallback, useState } from "react";
import { toast } from "sonner";
import { useTranslation } from "react-i18next";
import type {
  CommandInput,
  CommandName,
  CommandResult,
} from "@mousebox/shared";
import { isWorkspaceCommand, sameWorkspaceContext } from "@mousebox/shared";
import { useRuntimeStore } from "~/runtime/state";
import { invoke } from "~/lib/desktopApi";
import { displayText } from "~/i18n/displayText";
export function useCommand() {
  const [pending, setPending] = useState(0);
  const { t, i18n } = useTranslation();
  const context = useRuntimeStore(s => s.workspace?.active);
  const run = useCallback(
    async <K extends CommandName>(
      name: K,
      args: CommandInput<K>,
    ): Promise<CommandResult<K> | undefined> => {
      setPending((n) => n + 1);
      try {
        if (context && isWorkspaceCommand(name, args) && !sameWorkspaceContext(context, useRuntimeStore.getState().workspace?.active)) return undefined;
        const result = await invoke(name, args, context);
        if (name.startsWith("workspace.")) {
          const catalog = result as CommandResult<"workspace.read">;
          const current = useRuntimeStore.getState().workspace?.active;
          // A late catalog reply must not undo a more recent switch event.
          if (context && !sameWorkspaceContext(context, current) && !sameWorkspaceContext(catalog.active, current)) return undefined;
          useRuntimeStore.getState().applyWorkspace(catalog);
          return result;
        }
        const current = useRuntimeStore.getState().workspace?.active;
        if (context && !sameWorkspaceContext(context, current)) return undefined;
        return result;
      } catch (error) {
        if (context && !sameWorkspaceContext(context, useRuntimeStore.getState().workspace?.active)) return undefined;
        toast.error(t("操作失败"), {
          description: displayText(
            error instanceof Error ? error.message : String(error),
            i18n.language === "en" ? "en" : "zh",
          ),
        });
        return undefined;
      } finally {
        setPending((n) => n - 1);
      }
    },
    [t, i18n, context],
  );
  return { run, pending: pending > 0 };
}
