import { useEffect, useMemo, useState } from "react";
import { useLocation } from "react-router";
import { useShallow } from "zustand/react/shallow";
import { useRuntimeStore } from "~/runtime/state";
import { selectContextFacts } from "./facts";
import { buildContextMessages } from "./messages";
import { contextPage, type ContextPage } from "./model";
import { CONTEXT_ROTATE_MS, contextCandidates, contextIndex, nextContextCursor, type ContextCursor } from "./scheduler";

export function usePageContext() {
  const location = useLocation();
  const page = contextPage(location.pathname, location.search);
  const facts = useRuntimeStore(useShallow(s => selectContextFacts(s, page)));
  const candidates = useMemo(() => contextCandidates(buildContextMessages(page, facts)), [page, facts]);
  const signature = `${facts.workspaceEpoch ?? "global"}:` + candidates.map(m => m.id).join("|");
  const [cursors, setCursors] = useState<Partial<Record<ContextPage, ContextCursor>>>({});
  const count = candidates.length;
  const index = contextIndex(cursors[page], signature, count);
  // Counters, translated text and repeated retries never restart the reading interval.
  // No timer is needed for a pinned task or a single fault.
  useEffect(() => {
    if (count <= 1) return;
    const timer = window.setInterval(() => {
      if (document.hidden) return;
      setCursors(current => ({
        ...current, [page]: nextContextCursor(current[page], signature, count),
      }));
    }, CONTEXT_ROTATE_MS);
    return () => window.clearInterval(timer);
  }, [page, signature, count]);
  return { page, active: candidates[index] };
}
