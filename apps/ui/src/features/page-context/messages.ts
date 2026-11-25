import type { ContextFacts } from "./facts";
import { contextFaults } from "./faults";
import {triggerDetail} from "./trigger";
import { activity, copy, message, type ContextMessage, type ContextPage } from "./model";

const metric = (v: unknown, digits = 1) =>
  typeof v === "number" ? v.toFixed(digits) : v == null || v === "" ? "—" : String(v);

function learningMessages(f: ContextFacts): ContextMessage[] {
  const count = copy("{{sessions}} 个独立场次 · {{trials}} 段 · {{samples}} 个样本", {
    sessions: f.sessions ?? 0, trials: f.trials ?? 0, samples: f.samples ?? 0,
  });
  if (f.training) return [activity("learning:training", "正在拟合并验证速度模型", count)];
  if (f.recording) return [activity("learning:recording", "正在采集鼠标速度", count)];
  if (f.ioBusy) return [activity("learning:io", "正在读写采样或模型文件")];
  if (f.replaying) return [activity("learning:replaying", "正在回放采样轨迹")];
  return [
    message("learning:samples", "速度采样记录", count),
    message("learning:model", f.modelActive ? "速度模型已启用" : f.canApply ? "速度模型可手动启用" : "等待载入或拟合速度模型",
      f.learningFeedback ? copy(f.learningFeedback) : undefined, f.modelActive ? "success" : "quiet"),
  ];
}
function reportMessages(f: ContextFacts): ContextMessage[] {
  const count = copy("{{reports}} 条报文 · {{seconds}} s", {
    reports: f.reports ?? 0, seconds: metric((f.elapsedMs ?? 0) / 1000),
  });
  if (f.recording) return [activity("reports:recording", "正在采集原始鼠标报文", count)];
  if (f.analyzing) return [activity("reports:analyzing", "正在统计原始鼠标报文", count)];
  if (f.reportsBusy) return [activity("reports:io", "正在保存或导出鼠标报文", count)];
  if (f.hasResult) return [message("reports:result", "本次鼠标特征采集已结束", count, "success")];
  return [message("reports:ready", "鼠标特征采集已准备好", copy("在活动区域自由移动，按 Esc 或结束按钮停止。"))];
}
function pageMessages(page: ContextPage, f: ContextFacts): ContextMessage[] {
  switch (page) {
    case "input-device": return f.testArmed ? [activity("calibration:test", "F8 桌面位移测试已开启", copy("短按 F8 测试，松开停止。"))] : [message("settings:device-input", "本机输入设备与原始鼠标调试")];
    case "data":
    case "general": return [];
    case "cs2-source": return [message("game-settings:source", "CS2 数据")];
    case "valorant-display": return [message("game-settings:display", "窗口与单位")];
    case "reader":
      return [
        message("reader:summary", f.readerConnected ? "Reader 已连接" : f.readerRunning ? "正在连接 Reader" : "Reader 未连接",
          f.readerRunning && !f.readerConnected && f.readerMessage ? copy(f.readerMessage) : undefined, f.readerConnected ? "success" : "info"),
        message("reader:source", "订阅",
          copy("{{host}}:{{port}} · {{hz}} Hz", { host: f.host ?? "—", port: f.port ?? "—", hz: f.hz ?? "—" })),
      ];
    case "dashboard":
      return [message("dashboard:data", f.readerConnected ? "当前游戏数据" : "等待数据",
        f.readerConnected && f.characters != null && f.characters !== "" ? copy("{{characters}} 人", { characters: metric(f.characters, 0) }) : undefined, f.readerConnected ? "success" : "info")];
    case "visuals-general": return [message("visuals:general", "Visuals 常规")];
    case "radar": return [message("visuals:radar", "Radar")];
    case "esp":
      return [
        message("esp:scene", f.readerConnected ? "ESP 场景随数据更新" : "ESP 等待 Reader 数据",
          f.espVisible ? copy(f.espVisible) : undefined, f.readerConnected ? "success" : "info"),
        message("esp:device", f.adapterReady ? "当前 ESP 绘制设备" : "正在准备 ESP 绘制设备",
          f.adapter ? copy(f.adapter) : undefined),
      ];
    case "aim-general": return [message("aim:general", "Aim 常规")];
    case "aimbot":
      if(f.aimEnabled===false)return [message("aimbot:disabled","Aimbot已关闭")];
      if(f.recoilOnly)return [message("aimbot:recoil-only", "正在补偿后坐，瞄准方向由鼠标控制", undefined, "success")];
      if (f.canAim === false && f.control)
        return [activity("aimbot:waiting", "侧键跟随正在等待就绪条件", copy(f.control))];
      return [
        message("aimbot:control", f.canAim ? "侧键跟随已就绪" : f.aimArmed ? "侧键跟随正在等待当前数据" : "侧键跟随等待链路就绪",
          f.control ? copy(f.control) : undefined, f.canAim ? "success" : "info"),
        message("aimbot:target", f.target != null ? "当前候选或锁定目标" : "等待符合条件的目标",
          f.target != null ? f.distance == null ? copy("目标 {{target}}", {target:String(f.target)}) : copy("目标 {{target}} · 距离 {{distance}} m", { target: String(f.target), distance: metric(f.distance) }) : undefined),
        ...(f.ballistics === undefined ? [] : [message("aimbot:ballistics", "当前弹道计算",
          copy("{{profile}} · {{status}}", { profile: f.ballisticProfile || "—", status: f.ballistics || "—" }))]),
      ];
    case "triggerbot": return f.triggerEnabled===undefined?[message("triggerbot:placeholder", "当前游戏尚未提供自动扳机")]:
      [message("triggerbot:state",f.triggerEnabled?"自动扳机已启用":"自动扳机已关闭",triggerDetail(f),f.triggerEnabled?"info":"quiet")];
    case "rage": return [message("rage:state",f.rageEnabled?"Rage已启用":"Rage已关闭",
      copy("{{reason}} · 目标 {{target}} · 预计伤害 {{damage}} · 已提交射击 {{count}}",{reason:f.rageReason??"",target:f.rageTarget??"—",damage:metric(f.rageDamage),count:f.rageClicks??0}),f.rageActive?"success":f.rageEnabled?"info":"quiet")];
    case "recoil": return [message("recoil:state","No Recoil")];
    case "learning": return learningMessages(f);
    case "reports": return reportMessages(f);
    case "logs":
      return [message("logs:summary", "本次运行日志", copy("{{total}} 条记录 · {{warnings}} 条警告 · {{errors}} 条错误", {
        total: f.logCount ?? 0, warnings: f.logWarnings ?? 0, errors: f.logErrors ?? 0,
      }))];
    case "firmware":
      return [message("settings:firmware", f.boxReady ? "鼠标盒子已连接" : f.boxPhase === "connecting" ? "正在连接鼠标盒子" : "等待鼠标盒子接入",
        f.boxReady ? (f.boxMessage ? copy(f.boxMessage) : undefined) : f.serialMessage ? copy(f.serialMessage) : undefined,
        f.boxReady ? "success" : "info")];
    case "input":
      return [message("settings:input", "游戏输入与实时 FOV", copy("{{dpi}} DPI · FOV {{fov}}°", {
        dpi: f.dpi ?? "—", fov: metric(f.fov),
      }))];
    case "algorithm":
      return [message("settings:algorithm", "当前算法的数据来源与计算原理", f.ballistics ? copy(f.ballistics) : undefined)];
    case "movement":
      return [message("settings:movement", "当前鼠标移动上限", copy("每轴 {{speed}} 计数/秒 · {{dpi}} DPI", {
        speed: f.speedLimit ?? "—", dpi: f.dpi ?? "—",
      }))];
    case "calibration":
      if (f.calibrating) return [activity("calibration:running", "正在进行实测校准", f.calibration ? copy(f.calibration) : undefined)];
      if (f.testArmed) return [activity("calibration:test", "F8 桌面位移测试已开启", copy("短按 F8 测试，松开停止。"))];
      return [message("settings:calibration", "校准记录与桌面位移诊断",
        copy(f.mapping === 0 ? "当前使用实时 FOV 换算，实测记录作为备用。" : "当前使用匹配的实测记录换算。"))];
    case "rendering":
      return [message("settings:rendering", f.adapterReady ? "当前 ESP 绘制设备" : "等待创建 ESP 绘制设备",
        f.adapterReady && f.adapter ? copy(f.adapter) : copy("按数据到达更新，不设软件 FPS 上限。"))];
    default: return [message("page:ready", "game-cheat-client 已就绪")];
  }
}
export function buildContextMessages(page: ContextPage, facts: ContextFacts): ContextMessage[] {
  const faults = contextFaults(facts);
  if (!facts.ready || !facts.connected || facts.migrationRequired)
    return faults.length ? faults : [activity("runtime:starting", "正在连接原生运行时…")];
  return [...faults, ...(facts.workspaceSwitching ? [activity("workspace:switching", "正在切换游戏…")] : pageMessages(page, facts))];
}
