import { useEffect, useRef, useState } from "react";
import type { LogEntry } from "@mousebox/shared";

const ROW_HEIGHT = 38;
const OVERSCAN = 8;

function displayTime(value: string) {
  const parts = value.match(/^\d{4}-(\d{2}-\d{2})T(\d{2}:\d{2}:\d{2}(?:\.\d{1,3})?)/);
  return parts ? `${parts[1]} ${parts[2]}` : value;
}

export function LogList({ entries }: { entries: readonly LogEntry[] }) {
  const ref = useRef<HTMLDivElement>(null);
  const [scroll, setScroll] = useState(0);
  const [height, setHeight] = useState(500);
  useEffect(() => {
    const list = ref.current;
    if (!list) return;
    const measure = () => {
      if (list.clientHeight > 0) setHeight(list.clientHeight);
    };
    measure();
    const observer = new ResizeObserver(measure);
    observer.observe(list);
    return () => observer.disconnect();
  }, []);

  const totalHeight = entries.length * ROW_HEIGHT;
  const scrollTop = Math.min(scroll, Math.max(0, totalHeight - height));
  const start = Math.max(0, Math.floor(scrollTop / ROW_HEIGHT) - OVERSCAN);
  const end = Math.min(entries.length, Math.ceil((scrollTop + height) / ROW_HEIGHT) + OVERSCAN);

  return (
    <div ref={ref} className="log-list" onScroll={event => setScroll(event.currentTarget.scrollTop)} role="log" aria-live="off">
      <div className="log-spacer" style={{ height: totalHeight }}>
        <div className="log-window" style={{ top: start * ROW_HEIGHT }}>
          {entries.slice(start, end).map(entry => (
            <div key={entry.id} className={`log-row log-${entry.level}`} style={{ height: ROW_HEIGHT }}>
              <time dateTime={entry.time} title={entry.time}>{displayTime(entry.time)}</time>
              <span className="log-level">{entry.level}</span>
              {entry.workspace && <span className="log-workspace" title={entry.workspace.workspaceId}>{entry.workspace.gameId.toUpperCase()}</span>}
              <span className="log-message" title={entry.message}>{entry.message}</span>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}
