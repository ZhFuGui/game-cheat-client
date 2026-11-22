import type { ContextFacts } from "./facts";
import { fault, type ContextMessage } from "./model";
import {triggerDetail} from "./trigger";

/** Only explicitly reported current conditions qualify; historical logs are not live faults. */
export function contextFaults(f: ContextFacts): ContextMessage[] {
  if (!f.connected && (f.ready || f.runtimeError))
    return [fault("runtime:disconnected", "原生运行时已断开", f.runtimeError, 100)];
  if (f.migrationRequired)
    return [fault("migration:required", "请选择旧配置来源", "选择后继续载入已有配置与记录。", 90, "warning")];
  if (!f.ready || !f.connected) return [];
  if (f.sessionEligible === false)
    return [fault("runtime:ineligible", "运行会话尚未就绪", "输出已停止，等待运行会话恢复。", 90, "warning")];
  const result: ContextMessage[] = [];
  if(f.rageEnabled&&f.rageBlock&&!['disabled','tracking','clicked','waiting-source','waiting-game-foreground','analysis-pending','no-visible-target','no-damage-target','targets-disabled','target-unavailable','reader-not-requested','map-unavailable','shot-pending','shot-ready','no-hit','inset-rejected','other-person-first','friendly-first','target-filtered','cover-occluded','shot-no-damage','physical-left-held','cooldown','capture-consumed','aim-not-current'].includes(f.rageBlock))
    result.push(fault(`rage:${f.rageBlock}`,"Rage正在等待当前条件",f.rageReason,35,"warning"));
  if(f.triggerEnabled&&f.triggerBlock&&!['ready','clicked','cooldown','no-hit','physical-left-held','friendly-first','target-filtered','targets-disabled','disabled','cover-pending','cover-occluded','reaction-pending','reaction-waiting','reaction-hidden','weapon-filtered','inset-rejected','sniper-inaccuracy-too-high'].includes(f.triggerBlock))
    result.push({...fault(`triggerbot:${f.triggerBlock}`,"自动扳机正在等待当前条件",undefined,35,"warning"),detail:triggerDetail(f)});
  if (f.extensionId)
    result.push(fault(`extension:${f.extensionId}:${f.extensionCode || f.extensionPhase}`,
      "当前游戏扩展正在等待", f.extensionMessage, 32, f.extensionPhase === "error" ? "error" : "warning"));
  if (f.graphicsError) result.push(fault("graphics:error", "ESP 绘制暂不可用", f.graphicsError, 60));
  if (f.reportError || f.reportReason === "error")
    result.push(fault("reports:error", "鼠标特征采集异常", f.reportError || "采集异常，已结束", 60));
  if (f.readerPhase === "error") result.push(fault("reader:error", "Reader 连接失败", f.readerMessage));
  if (f.serialPhase === "error") result.push(fault("serial:error", "串口检测失败", f.serialMessage));
  if (f.boxPhase === "error") result.push(fault("box:error", "鼠标盒子连接失败", f.boxMessage));
  if (f.ballisticsError) result.push(fault("ballistics:error", "弹道资料暂不可用", f.ballisticsError));
  if (f.readerPhase === "degraded")
    result.push(fault("reader:degraded", "Reader 数据尚不完整", f.readerMessage, 40, "warning"));
  if (f.serialPhase === "degraded")
    result.push(fault("serial:degraded", "请选择要连接的串口", f.serialMessage, 40, "warning"));
  if (f.boxPhase === "degraded")
    result.push(fault("box:degraded", "鼠标盒子尚未完全就绪", f.boxMessage, 40, "warning"));
  if (f.adapterNotice)
    result.push(fault("graphics:fallback", "正在使用回退绘制设备", f.adapterNotice, 30, "warning"));
  return result;
}
