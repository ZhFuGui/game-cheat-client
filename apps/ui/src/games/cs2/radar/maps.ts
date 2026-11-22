import type { Cs2RadarFrame } from "@mousebox/shared";

export type RadarFloor = "main" | "lower";

export interface RadarMap {
  id: string;
  title: string;
  posX: number;
  posY: number;
  scale: number;
  mainFloorMinZ?: number;
}

// These values calibrate the supplied overviews on a 1024 px grid, including the 2048 px Dust II texture.
const maps = new Map<string, RadarMap>([
  { id: "de_dust2", title: "Dust II", posX: -2476, posY: 3232, scale: 4.4 },
  { id: "de_mirage", title: "Mirage", posX: -3230, posY: 1713, scale: 5 },
  { id: "de_inferno", title: "Inferno", posX: -2087, posY: 3870, scale: 4.9 },
  { id: "de_nuke", title: "Nuke", posX: -3453, posY: 2887, scale: 7, mainFloorMinZ: -495 },
  { id: "de_overpass", title: "Overpass", posX: -4831, posY: 1781, scale: 5.2 },
  { id: "de_vertigo", title: "Vertigo", posX: -3168, posY: 1762, scale: 4, mainFloorMinZ: 11700 },
  { id: "de_ancient", title: "Ancient", posX: -2953, posY: 2164, scale: 5 },
  { id: "de_anubis", title: "Anubis", posX: -2796, posY: 3328, scale: 5.22 },
].map(map => [map.id, map]));

export function getRadarMap(identity: Cs2RadarFrame["map"] | undefined): RadarMap | undefined {
  if (!identity?.available || !identity.name) return;
  const map = maps.get(identity.name.toLowerCase());
  if (map && identity.resourcePath?.replaceAll("\\", "/").toLowerCase() === `maps/${map.id}.vpk`)
    return map;
}

// Valve's overviews put the same boundary in the lower maximum and the main minimum.
export function floorAtHeight(map: RadarMap, z: number): RadarFloor {
  return map.mainFloorMinZ !== undefined && z < map.mainFloorMinZ ? "lower" : "main";
}

export function radarImage(map: RadarMap, floor: RadarFloor): string {
  const name = floor === "lower" && map.mainFloorMinZ !== undefined ? `${map.id}_lower` : map.id;
  return `${import.meta.env.BASE_URL}radar/cs2/${name}.png`;
}
