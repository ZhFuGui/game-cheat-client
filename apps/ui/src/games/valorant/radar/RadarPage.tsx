import {useEffect,useMemo,useRef,useState} from "react";
import {useTranslation} from "react-i18next";
import type {ValorantRadarFrame,ValorantRadarPlayer} from "@mousebox/shared";
import {AppIcon} from "~/components/icons";
import {Button} from "~/components/ui/button";
import {HelpTooltip} from "~/components/shared/HelpTooltip";
import {MapCanvas,type MapMarker} from "~/features/radar/MapCanvas";
import {RadarWaiting} from "~/features/radar/RadarWaiting";
import {findMap,imageUrl} from "./maps";
import {positionBounds,projectPoint} from "./projection";
import {useRadarFrame} from "./useRadarFrame";

const labels={unknown:"未分类",self:"本地",ally:"友方",opponent:"敌方"} as const;
const colors={unknown:"#ccd4df",self:"#67e8a9",ally:"#76c3ff",opponent:"#ff8787"};

function Roster({players,title,side}:{players:readonly ValorantRadarPlayer[];title:string;side:"leading"|"trailing"}){
  const {t}=useTranslation();
  return <section className={`radar-roster valorant-roster-${side}`} aria-label={t(title)}>
    <div className="radar-roster-header"><strong>{t(title)}</strong><span>{players.length}</span></div>
    <ul>{players.map(player=><li key={player.id}>
      <div className="radar-player-title"><strong>#{player.id}</strong><span style={{color:"color-mix(in srgb, "+colors[player.relation]+" 35%, var(--foreground))"}}>{t(labels[player.relation])}</span></div>
      <div className="radar-player-stats"><span>{t("队伍")}: {player.team?.key??"—"}</span><span>{player.position?t("位置已知"):t("位置缺失")}</span></div>
    </li>)}</ul>
  </section>;
}

function RadarView({frame}:{frame:ValorantRadarFrame}){
  const {t}=useTranslation();
  const map=findMap(frame.map?.packagePath);
  const [names,setNames]=useState(false),[following,setFollowing]=useState(false);
  const [bounds,setBounds]=useState(()=>positionBounds(frame.players));
  const initialized=useRef(frame.players.some(player=>player.position!==null));
  useEffect(()=>{
    if(!initialized.current&&frame.players.some(player=>player.position!==null)){
      initialized.current=true;setBounds(positionBounds(frame.players));
    }
  },[frame.players]);
  const localId=frame.local.state==="resolved"?frame.local.entityId:null;
  const local=frame.players.find(player=>player.id===localId);
  const localPoint=local?.position?projectPoint(map,bounds,local.position):null;
  const markers=useMemo(()=>{
    const result:MapMarker[]=[];
    for(const player of frame.players){
      if(!player.position)continue;
      const point=projectPoint(map,bounds,player.position);if(!point)continue;
      result.push({key:player.id,x:point[0],y:point[1],color:colors[player.relation],local:player.id===localId,
        label:names?`#${player.id}`:undefined});
    }
    if(frame.camera){
      const point=projectPoint(map,bounds,frame.camera);
      if(point)result.push({key:"camera",x:point[0],y:point[1],color:"#ffffff",label:t("相机")});
    }
    return result;
  },[frame.players,frame.camera,map,bounds,localId,names,t]);
  const allies=frame.players.filter(player=>player.relation==="self"||player.relation==="ally");
  const others=frame.players.filter(player=>player.relation!=="self"&&player.relation!=="ally");
  const title=map?.name||frame.map?.worldName||t("等待当前地图");
  return <div className="radar-page valorant-radar-page">
    <div className="radar-toolbar">
      <div className="radar-map-name"><AppIcon name="radar" size={16}/><strong>{title}</strong>
        {!map&&<span className="hint">{t("无底图")}</span>}
        <HelpTooltip label="VALORANT Radar">{t("地图来自当前 Reader 上下文。已核验的本地与队伍关系用于着色，未知关系保持未分类。无底图时仅显示相对位置，不推断墙体和楼层。")}</HelpTooltip>
      </div>
      <div className="radar-controls">
        <Button variant={following?"secondary":"outline"} size="sm" disabled={!localPoint&&!following}
          aria-pressed={following} onClick={()=>setFollowing(value=>!value)}>{t("跟随本地")}</Button>
        <Button variant={names?"secondary":"outline"} size="sm" aria-pressed={names} onClick={()=>setNames(value=>!value)}>{t("显示编号")}</Button>
        <Button variant="outline" size="sm" onClick={()=>{setBounds(positionBounds(frame.players));setFollowing(false);}}>{t("适应人物范围")}</Button>
        <HelpTooltip label="雷达操作">{t("拖动移动地图，滚轮或加减键缩放，Home 恢复视图。")}</HelpTooltip>
      </div>
    </div>
    <div className="radar-layout valorant-radar-layout">
      <div className="radar-map" data-map={map?.id??"untextured"}>
        <MapCanvas imageUrl={map?imageUrl(map):undefined} markers={markers} centerOn={following&&localPoint?localPoint:undefined}/>
        {!frame.players.some(player=>player.position!==null)&&<div className="radar-map-status" role="status">{t("等待人物位置")}</div>}
      </div>
      <Roster title="本地与友方" side="leading" players={allies}/>
      <Roster title="其他人物" side="trailing" players={others}/>
    </div>
  </div>;
}

export default function RadarPage(){
  const {frame,failed}=useRadarFrame();
  if(!frame)return <div className="radar-page"><div className="radar-map">
    <RadarWaiting connection message={failed?"暂时无法读取 Radar 数据":"等待 VALORANT Reader 数据"}/>
  </div></div>;
  return <RadarView key={`${frame.scope}:${frame.map?.packagePath??""}`} frame={frame}/>;
}
