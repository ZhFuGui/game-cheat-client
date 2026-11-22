import type {ContextFacts} from "./facts";
import {copy} from "./model";
export function triggerDetail(f:ContextFacts){
  const count=f.triggerClicks??0;
  if(!f.triggerEnabled)return copy("已提交点击请求 {{count}}",{count});
  const values={reason:f.triggerReason??"",count};
  return f.triggerTarget==null?copy("{{reason}} · 已提交点击请求 {{count}}",values):
    copy("{{reason}} · 已提交点击请求 {{count}} · 目标 {{target}} / Hitbox {{hitbox}}",
      {...values,target:f.triggerTarget,hitbox:f.triggerHitbox??"—"});
}
