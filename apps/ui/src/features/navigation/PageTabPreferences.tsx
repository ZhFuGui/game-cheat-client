import { useCallback, useEffect, useRef, useState, type ReactNode } from "react";
import { toast } from "sonner";
import { useTranslation } from "react-i18next";
import { PageOrderProvider } from "~/framework/pages/PageOrder";
import { invoke } from "~/lib/desktopApi";
import { useRuntimeStore } from "~/runtime/state";
import { useSurfaceStore } from "~/stores/surfaceStore";

/** Global UI preferences, independent of game sessions. One save per completed reorder. */
export function PageTabPreferences({ children }: { children: ReactNode }) {
  const stored = useRuntimeStore(state => state.preferences.pageTabOrder);
  const [drafts, setDrafts] = useState<Record<string, string[]>>({});
  const revisions = useRef(new Map<string, number>()), confirmed = useRef(new Map<string, number>()), mounted = useRef(true);
  const { t } = useTranslation();
  useEffect(() => { mounted.current = true; return () => { mounted.current = false; }; }, []);
  const reorder = useCallback((scope: string, ids: string[]) => {
    const revision = (revisions.current.get(scope) ?? 0) + 1;
    revisions.current.set(scope, revision);
    setDrafts(current => ({ ...current, [scope]: ids }));
    void invoke("preferences.set", { pageTabOrder: { [scope]: ids } }).then(result => {
      if ((confirmed.current.get(scope) ?? 0) >= revision) return;
      confirmed.current.set(scope, revision);
      useRuntimeStore.setState(state => ({ preferences: { ...state.preferences,
        pageTabOrder: { ...state.preferences.pageTabOrder, [scope]: result.pageTabOrder?.[scope] ?? ids } } }));
    }).catch(error => {
      if (revisions.current.get(scope) === revision)
        toast.error(t("标签顺序保存失败"), { description: error instanceof Error ? error.message : String(error) });
    }).finally(() => {
      if (!mounted.current || revisions.current.get(scope) !== revision) return;
      setDrafts(current => { const next = { ...current }; delete next[scope]; return next; });
    });
  }, [t]);
  return <PageOrderProvider orders={{ ...stored, ...drafts }} onReorder={reorder}
    onDragChange={useSurfaceStore.getState().setOccluded}>{children}</PageOrderProvider>;
}
