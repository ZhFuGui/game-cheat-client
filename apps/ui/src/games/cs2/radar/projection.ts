import type { Cs2RadarPlayer } from "@mousebox/shared";
import { floorAtHeight, type RadarFloor, type RadarMap } from "./maps";

export interface Point { x: number; y: number }
export interface RadarMarker { player: Cs2RadarPlayer; point: Point; floor: RadarFloor }
export interface RadarCamera {
  center: Point;
  rotation: number;
  extent: number;
  width: number;
  height: number;
}
export interface RadarMarkerPlacement {
  player: Cs2RadarPlayer;
  anchor: Point;
  position: Point;
  relativeFloor: "current" | "above" | "below";
}

const calibrationPixels = 1024;
const radians = Math.PI / 180;
export const mapCenter: Point = { x: 0.5, y: 0.5 };

export function projectToMap(map: RadarMap, [x, y]: readonly [number, number, number]): Point {
  const extent = map.scale * calibrationPixels;
  return { x: (x - map.posX) / extent, y: (map.posY - y) / extent };
}

export function radarMarkers(map: RadarMap, players: readonly Cs2RadarPlayer[]): RadarMarker[] {
  return players.flatMap(player => {
    if (player.alive !== true || !player.position) return [];
    const point = projectToMap(map, player.position);
    return point.x >= 0 && point.x <= 1 && point.y >= 0 && point.y <= 1
      ? [{ player, point, floor: floorAtHeight(map, player.position[2]) }] : [];
  });
}

export function radarCamera(width: number, height: number, zoom: number, center: Point,
                            following?: RadarMarker): RadarCamera {
  return {
    width, height,
    extent: Math.max(1, Math.min(width, height) - 24) * zoom,
    center: following?.point ?? center,
    rotation: following?.player.yaw == null ? 0 : (following.player.yaw - 90) * radians,
  };
}

export function screenPoint(camera: RadarCamera, point: Point): Point {
  const dx = (point.x - camera.center.x) * camera.extent;
  const dy = (point.y - camera.center.y) * camera.extent;
  const cos = Math.cos(camera.rotation), sin = Math.sin(camera.rotation);
  return { x: camera.width / 2 + dx * cos - dy * sin, y: camera.height / 2 + dx * sin + dy * cos };
}

export function screenHeading(camera: RadarCamera, yaw: number): number {
  return camera.rotation - yaw * radians;
}

const markerSpacing = 22;
const markerOffsets: readonly Point[] = [
  {x:0,y:-26}, {x:26,y:0}, {x:-26,y:0}, {x:0,y:26},
  {x:26,y:-26}, {x:-26,y:-26}, {x:26,y:26}, {x:-26,y:26},
  {x:0,y:-52}, {x:52,y:0}, {x:-52,y:0}, {x:0,y:52},
];

export function layoutRadarMarkers(camera: RadarCamera, markers: readonly RadarMarker[], floor: RadarFloor): RadarMarkerPlacement[] {
  const current: RadarMarkerPlacement[] = [];
  const other: RadarMarkerPlacement[] = [];
  for (const marker of markers) {
    const anchor = screenPoint(camera, marker.point);
    if (anchor.x < -24 || anchor.y < -24 || anchor.x > camera.width + 24 || anchor.y > camera.height + 24)
      continue;
    const relativeFloor = marker.floor === floor ? "current" : marker.floor === "main" ? "above" : "below";
    const placement: RadarMarkerPlacement = { player: marker.player, anchor, position: anchor, relativeFloor };
    (relativeFloor === "current" ? current : other).push(placement);
  }

  const occupied = current.map(marker => marker.position);
  const clear = (point: Point) => occupied.every(marker =>
    (point.x - marker.x) ** 2 + (point.y - marker.y) ** 2 >= markerSpacing ** 2);
  for (const placement of other) {
    const { anchor } = placement;
    if (!clear(anchor)) {
      for (const offset of markerOffsets) {
        const candidate = { x: anchor.x + offset.x, y: anchor.y + offset.y };
        if (candidate.x >= 12 && candidate.y >= 12 && candidate.x <= camera.width - 12 &&
            candidate.y <= camera.height - 12 && clear(candidate)) {
          placement.position = candidate;
          break;
        }
      }
    }
    occupied.push(placement.position);
  }
  return [...current, ...other];
}

export function panCenter(center: Point, dx: number, dy: number, extent: number): Point {
  return { x: Math.max(0, Math.min(1, center.x - dx / extent)),
           y: Math.max(0, Math.min(1, center.y - dy / extent)) };
}
