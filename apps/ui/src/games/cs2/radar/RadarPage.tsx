import { useCallback, useEffect, useMemo, useState } from "react";
import { Link } from "react-router";
import { useTranslation } from "react-i18next";
import type { Cs2RadarFrame, Cs2RadarPlayer } from "@mousebox/shared";
import { AppIcon } from "~/components/icons";
import { Button } from "~/components/ui/button";
import { HelpTooltip } from "~/components/shared/HelpTooltip";
import { RadarCanvas } from "./RadarCanvas";
import { RadarRoster } from "./RadarRoster";
import { floorAtHeight, getRadarMap, type RadarFloor, type RadarMap } from "./maps";
import { mapCenter, panCenter, radarMarkers } from "./projection";
import { useRadarFrame } from "./useRadarFrame";

const emptyRoster: readonly Cs2RadarPlayer[] = [];
const floorLabels = { main: "主层", lower: "下层" } as const;

function RadarView({ frame, map, failed }: { frame: Cs2RadarFrame | null; map?: RadarMap; failed: boolean }) {
  const { t } = useTranslation();
  const [zoom, setZoom] = useState(1);
  const [center, setCenter] = useState(mapCenter);
  const [following, setFollowing] = useState(false);
  const [names, setNames] = useState(true);
  const [floorMode, setFloorMode] = useState<"auto" | RadarFloor>("auto");
  const [lastLocalFloor, setLastLocalFloor] = useState<RadarFloor>("main");
  const players = frame?.players ?? emptyRoster;
  const markers = useMemo(() => map ? radarMarkers(map, players) : [], [map, players]);
  const local = markers.find(marker => marker.player.local);
  const localHeight = players.find(player => player.local)?.position?.[2];
  const observedFloor = map && localHeight !== undefined ? floorAtHeight(map, localHeight) : undefined;
  const floor = floorMode === "auto" ? observedFloor ?? lastLocalFloor : floorMode;

  useEffect(() => {
    if (observedFloor) setLastLocalFloor(observedFloor);
  }, [observedFloor]);

  const pan = useCallback((dx: number, dy: number, extent: number) => {
    setCenter(point => panCenter(point, dx, dy, extent));
  }, []);
  const changeZoom = useCallback((delta: number) => {
    setZoom(value => Math.max(0.5, Math.min(5, value + delta)));
  }, []);
  const reset = () => { setZoom(1); setCenter(mapCenter); setFollowing(false); };
  const title = map?.title ?? frame?.map?.name ?? t("雷达地图");
  const waiting = failed ? "暂时无法读取 Radar 数据" : !frame ? "等待 CS2 Reader 数据" :
    frame.map?.available ? "当前地图缺少雷达校准" : "等待当前地图";
  return <div className="radar-page">
    <div className="radar-toolbar">
      <div className="radar-map-name"><AppIcon name="radar" size={16} /><strong>{title}</strong></div>
      {map && <div className="radar-controls">
        {map.mainFloorMinZ !== undefined && <div className="radar-control-group" role="group" aria-label={t("底图楼层")}>
          <Button variant={floorMode === "auto" ? "secondary" : "outline"} size="sm"
            aria-label={t("自动切层")} aria-pressed={floorMode === "auto"} onClick={() => setFloorMode("auto")}>
            {floorMode === "auto" ? t("自动 · {{floor}}", { floor: t(floorLabels[floor]) }) : t("自动")}
          </Button>
          {(["main", "lower"] as const).map(level => <Button key={level}
            variant={floorMode === level ? "secondary" : "outline"} size="sm" aria-pressed={floorMode === level}
            onClick={() => setFloorMode(level)}>{t(floorLabels[level])}</Button>)}
          <HelpTooltip label="底图楼层">{t("自动按本地玩家高度切换。实心圆在当前层，菱形内的 ↑ 或 ↓ 表示上方或下方楼层。重叠时错开标记，细线指向实际位置。")}</HelpTooltip>
        </div>}
        <Button variant={following ? "secondary" : "outline"} size="sm" aria-pressed={following}
          disabled={!local && !following} onClick={() => setFollowing(value => !value)}>{t("跟随本地")}</Button>
        <Button variant={names ? "secondary" : "outline"} size="sm" aria-pressed={names}
          onClick={() => setNames(value => !value)}>{t("显示名称")}</Button>
        <div className="radar-control-group radar-zoom" role="group" aria-label={t("雷达缩放")}>
          <Button variant="outline" size="sm" aria-label={t("缩小雷达")} disabled={zoom <= 0.5} onClick={() => changeZoom(-0.25)}>−</Button>
          <span>{Math.round(zoom * 100)}%</span>
          <Button variant="outline" size="sm" aria-label={t("放大雷达")} disabled={zoom >= 5} onClick={() => changeZoom(0.25)}>+</Button>
        </div>
        <Button variant="outline" size="sm" onClick={reset}>{t("重置视图")}</Button>
      </div>}
    </div>
    <div className="radar-layout">
      <div className="radar-map" data-map={map?.id} data-map-layer={map ? floor : undefined}>
        {map ? <RadarCanvas map={map} floor={floor} zoom={zoom} center={center}
          following={following ? local : undefined} names={names} markers={markers} onPan={pan} onZoom={changeZoom} /> :
          <div className="radar-map-status" role="status">
            <AppIcon name="radar" size={28} />
            <span>{t(waiting)}</span>
            {!frame && !failed && <Link to="/">{t("Reader 连接")}</Link>}
          </div>}
      </div>
      <RadarRoster team={3} players={players} />
      <RadarRoster team={2} players={players} />
    </div>
  </div>;
}

export default function RadarPage() {
  const { frame, failed } = useRadarFrame();
  const map = getRadarMap(frame?.map);
  const scope = frame ? `${frame.scope}:${frame.map?.generation}:${frame.map?.name}` : "waiting";
  return <RadarView key={scope} frame={frame} map={map} failed={failed} />;
}
