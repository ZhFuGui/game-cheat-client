import manifest from "./manifest.json";

export type Point = readonly [number, number];
export type RadarMap = typeof manifest.maps[number];

/** Match only a confirmed identity; the caller never selects a fallback map. */
export function findMap(identity: string | null | undefined): RadarMap | undefined {
  const value = identity?.replaceAll("\\", "/").toLowerCase();
  if (!value) return;
  return manifest.maps.find(map => map.mapUrl.toLowerCase() === value ||
    map.mapUrl.split("/").at(-1)?.toLowerCase() === value || map.name.toLowerCase() === value || map.id === value);
}

export function mapPoint(map: RadarMap, position: readonly [number, number, number]): Point | null {
  const x = position[1] * map.xMultiplier + map.xScalarToAdd;
  const y = position[0] * map.yMultiplier + map.yScalarToAdd;
  return Number.isFinite(x) && Number.isFinite(y) ? [x,y] : null;
}

export function imageUrl(map: RadarMap): string {
  return `${import.meta.env.BASE_URL}radar/valorant/${map.image}`;
}
