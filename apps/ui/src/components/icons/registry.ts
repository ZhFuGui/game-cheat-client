import {
  ArrowDown, ArrowUp, Braces, ChartNoAxesCombined, Check, ChevronDown, ChevronRight,
  Cpu, Crosshair, Database, Eye, Gamepad2, Info, Minus, Monitor, MoveVertical,
  PanelLeftClose, PanelLeftOpen, Radar, Search, Settings, SlidersHorizontal, Square, X,
} from "lucide-react";
import Link2Regular from "@mingcute/react/core-regular/link-2";
import Document2Regular from "@mingcute/react/core-regular/document-2";
import MouseRegular from "@mingcute/react/core-regular/mouse";
import DashboardRegular from "@mingcute/react/core-regular/dashboard";
import ChartBarRegular from "@mingcute/react/core-regular/chart-bar";
import Cursor3Regular from "@mingcute/react/core-regular/cursor-3";

/** Meanings belong here; consumers never depend on an icon vendor or artwork name. */
export const iconDefinitions = {
  connection: { glyph: Link2Regular, provider: "mingcute" },
  logs: { glyph: Document2Regular, provider: "mingcute" },
  mouseDevice: { glyph: MouseRegular, provider: "mingcute" },
  speedLearning: { glyph: DashboardRegular, provider: "mingcute" },
  dashboard: { glyph: ChartBarRegular, provider: "mingcute" },
  triggerbot: { glyph: Cursor3Regular, provider: "mingcute" },
  esp: { glyph: Eye, provider: "lucide" },
  radar: { glyph: Radar, provider: "lucide" },
  aimbot: { glyph: Crosshair, provider: "lucide" },
  recoil: { glyph: MoveVertical, provider: "lucide" },
  tuning: { glyph: SlidersHorizontal, provider: "lucide" },
  settings: { glyph: Settings, provider: "lucide" },
  game: { glyph: Gamepad2, provider: "lucide" },
  firmware: { glyph: Cpu, provider: "lucide" },
  rendering: { glyph: Monitor, provider: "lucide" },
  storage: { glyph: Database, provider: "lucide" },
  summary: { glyph: ChartNoAxesCombined, provider: "lucide" },
  source: { glyph: Braces, provider: "lucide" },
  search: { glyph: Search, provider: "lucide" },
  help: { glyph: Info, provider: "lucide" },
  chevronDown: { glyph: ChevronDown, provider: "lucide" },
  chevronRight: { glyph: ChevronRight, provider: "lucide" },
  check: { glyph: Check, provider: "lucide" },
  upload: { glyph: ArrowUp, provider: "lucide" },
  download: { glyph: ArrowDown, provider: "lucide" },
  hidePanel: { glyph: PanelLeftClose, provider: "lucide" },
  showPanel: { glyph: PanelLeftOpen, provider: "lucide" },
  minimize: { glyph: Minus, provider: "lucide" },
  maximize: { glyph: Square, provider: "lucide" },
  close: { glyph: X, provider: "lucide" },
} as const;

export type AppIconName = keyof typeof iconDefinitions;
