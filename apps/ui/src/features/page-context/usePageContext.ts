import { useEffect, useMemo, useState } from "react";
import { useLocation } from "react-router";
import { useShallow } from "zustand/react/shallow";
import { useRuntimeStore } from "~/runtime/state";
import { availableVisualsPages,availableAimPages } from "~/navigation/pageGroups";
import {getGameUiModule, resolveGameSettings} from "~/games/registry";
import { selectContextFacts } from "./facts";
import { buildContextMessages } from "./messages";
import { contextPage, type ContextPage } from "./model";
import { CONTEXT_ROTATE_MS, contextCandidates, contextIndex, nextContextCursor, type ContextCursor } from "./scheduler";

export function usePageContext() {
  const location = useLocation();
  const capabilities=useRuntimeStore(s=>s.workspace?.games.find(game=>game.id===s.workspace?.active?.gameId)?.capabilities);
  const game=useRuntimeStore(s=>s.workspace?.active?.gameId),module=getGameUiModule(game);
  const visuals=useMemo(()=>availableVisualsPages(!!capabilities?.includes("esp"),!!capabilities?.includes("radar")),[capabilities]);
  const aim=useMemo(()=>availableAimPages(!!module?.AimSettings,!!module?.Triggerbot,!!module?.Rage&&!!capabilities?.includes("rage")),[module,capabilities]);
  const gameSettingsSection = location.pathname === "/game-settings"
    ? resolveGameSettings(game, capabilities, new URLSearchParams(location.search).get("section")).selected?.id
    : undefined;
  const page = contextPage(location.pathname, location.search,visuals,aim,gameSettingsSection);
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
