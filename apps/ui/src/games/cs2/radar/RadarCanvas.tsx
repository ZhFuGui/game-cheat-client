import { useEffect, useMemo, useRef, useState, type KeyboardEvent, type PointerEvent } from "react";
import { useTranslation } from "react-i18next";
import { drawRadar } from "./drawRadar";
import { radarImage, type RadarFloor, type RadarMap } from "./maps";
import { radarCamera, type Point, type RadarMarker } from "./projection";

interface RadarCanvasProps {
  map: RadarMap;
  floor: RadarFloor;
  zoom: number;
  center: Point;
  following?: RadarMarker;
  names: boolean;
  markers: readonly RadarMarker[];
  onPan(dx: number, dy: number, extent: number): void;
  onZoom(delta: number): void;
}

export function RadarCanvas({ map, floor, zoom, center, following, names, markers, onPan, onZoom }: RadarCanvasProps) {
  const { t } = useTranslation();
  const canvas = useRef<HTMLCanvasElement>(null);
  const drag = useRef<(Point & { pointer: number }) | null>(null);
  const [dragging, setDragging] = useState(false);
  const [size, setSize] = useState({ width: 0, height: 0 });
  const [assets, setAssets] = useState<Partial<Record<RadarFloor, HTMLImageElement | null>>>({});
  const image = assets[floor];
  const camera = useMemo(() => radarCamera(size.width, size.height, zoom, center, following),
    [size, zoom, center, following]);

  useEffect(() => {
    const floors: RadarFloor[] = map.mainFloorMinZ === undefined ? ["main"] : ["main", "lower"];
    const loading = floors.map(level => {
      const texture = new Image();
      texture.onload = () => setAssets(previous => ({ ...previous, [level]: texture }));
      texture.onerror = () => setAssets(previous => ({ ...previous, [level]: null }));
      texture.src = radarImage(map, level);
      return texture;
    });
    return () => { for (const texture of loading) texture.onload = texture.onerror = null; };
  }, [map]);

  useEffect(() => {
    const element = canvas.current;
    if (!element) return;
    const measure = () => {
      const { clientWidth: width, clientHeight: height } = element;
      setSize(previous => previous.width === width && previous.height === height ? previous : { width, height });
    };
    const resize = new ResizeObserver(measure);
    resize.observe(element);
    measure();
    return () => resize.disconnect();
  }, []);

  useEffect(() => {
    const render = window.requestAnimationFrame(() => {
      if (canvas.current) drawRadar(canvas.current, image ?? undefined, camera, markers, floor, names);
    });
    return () => window.cancelAnimationFrame(render);
  }, [image, camera, markers, floor, names]);

  useEffect(() => {
    const element = canvas.current;
    const wheel = (event: WheelEvent) => {
      if (!event.deltaY) return;
      event.preventDefault();
      onZoom(event.deltaY < 0 ? 0.25 : -0.25);
    };
    element?.addEventListener("wheel", wheel, { passive: false });
    return () => element?.removeEventListener("wheel", wheel);
  }, [onZoom]);

  function begin(event: PointerEvent<HTMLCanvasElement>) {
    if (event.button !== 0 || following || drag.current) return;
    event.currentTarget.setPointerCapture(event.pointerId);
    drag.current = { x: event.clientX, y: event.clientY, pointer: event.pointerId };
    setDragging(true);
  }
  function move(event: PointerEvent<HTMLCanvasElement>) {
    const previous = drag.current;
    if (previous?.pointer !== event.pointerId || following) return;
    drag.current = { x: event.clientX, y: event.clientY, pointer: event.pointerId };
    onPan(event.clientX - previous.x, event.clientY - previous.y, camera.extent);
  }
  function end(event: PointerEvent<HTMLCanvasElement>) {
    if (drag.current?.pointer !== event.pointerId) return;
    drag.current = null;
    setDragging(false);
  }
  function keyboard(event: KeyboardEvent<HTMLCanvasElement>) {
    if (following) return;
    const step = 36;
    switch (event.key) {
      case "ArrowLeft": onPan(step, 0, camera.extent); break;
      case "ArrowRight": onPan(-step, 0, camera.extent); break;
      case "ArrowUp": onPan(0, step, camera.extent); break;
      case "ArrowDown": onPan(0, -step, camera.extent); break;
      default: return;
    }
    event.preventDefault();
  }
  return <>
    <canvas ref={canvas} className="radar-canvas" aria-label={t("CS2 雷达地图")} tabIndex={0}
      data-map-ready={!!image} data-players={markers.length} data-dragging={dragging} data-following={!!following}
      onPointerDown={begin} onPointerMove={move} onPointerUp={end} onPointerCancel={end} onLostPointerCapture={end}
      onKeyDown={keyboard} />
    {!image && <div className="radar-map-status" role="status">
      {t(image === null ? "雷达底图加载失败" : "正在加载雷达底图")}
    </div>}
  </>;
}
