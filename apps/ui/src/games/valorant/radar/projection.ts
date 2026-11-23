import type {ValorantRadarPlayer} from "@mousebox/shared";
import {mapPoint,type RadarMap,type Point} from "./maps";

export interface Bounds {x:number;y:number;extent:number;}
/** Untextured view uses one explicitly relative coordinate scale. It does not infer walls or floors. */
export function positionBounds(players:readonly ValorantRadarPlayer[]):Bounds{
  const values=players.flatMap(player=>player.position?[player.position]:[]);
  if(!values.length)return{x:0,y:0,extent:1000};
  const xs=values.map(point=>point[1]),ys=values.map(point=>-point[0]);
  const minX=Math.min(...xs),maxX=Math.max(...xs),minY=Math.min(...ys),maxY=Math.max(...ys);
  const extent=Math.max(maxX-minX,maxY-minY,1000)*1.3;
  return{x:(minX+maxX-extent)/2,y:(minY+maxY-extent)/2,extent};
}
export function projectPoint(map:RadarMap|undefined,bounds:Bounds,point:readonly[number,number,number]):Point|null{
  if(map)return mapPoint(map,point);
  const x=(point[1]-bounds.x)/bounds.extent,y=(-point[0]-bounds.y)/bounds.extent;
  return Number.isFinite(x)&&Number.isFinite(y)?[x,y]:null;
}
