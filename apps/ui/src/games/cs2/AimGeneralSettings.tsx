import {useTranslation} from "react-i18next";
import {FeatureSection} from "~/components/shared/FeatureSection";
import {SettingChoice,SettingField,SettingToggle} from "~/components/shared/SettingField";
import {useRuntimeStore} from "~/runtime/state";

export function Cs2RuntimeSettings() {
  const {t}=useTranslation();
  return <FeatureSection title="运行条件">
    <div className="aim-fields">
      <SettingToggle setting="cs2CheckForeground" label="检测游戏窗口位于前台"
        help={<p>{t("适用于 Triggerbot 和 Rage。默认开启，仅在本机 CS2 位于前台时允许输出。远程游戏可关闭此检查；关闭后切换窗口不会因此暂停，其他数据、会话及设备条件仍须满足。")}</p>}/>
      <SettingField setting="cs2HitboxInsetPercent" label="Hitbox 内缩比例" suffix="%" slider={{min:0,max:90}}
        help={<p>{t("Aimbot 在内缩范围内取点。Triggerbot 和 Rage 检查射线是否进入该范围。0% 使用原尺寸，胶囊和球体缩小半径，盒体从中心沿各轴缩小。")}</p>}/>
    </div>
  </FeatureSection>;
}

export function Cs2TargetSettings() {
  const {t}=useTranslation();
  return <FeatureSection title="包含对象" help={<p>{t("Aimbot、Triggerbot 和 Rage 共用此设置。敌人和队友可独立选择，默认只包含敌人。两个选项都关闭时，三个功能都没有可用目标。")}</p>}>
    <div className="aim-fields">
      <SettingToggle setting="cs2RageEnemies" label="敌人"/>
      <SettingToggle setting="cs2RageTeammates" label="队友"
        help={t("开启后队友也可成为目标。Rage 按可对队友造成伤害的条件估算，实际伤害由服务器的友军伤害规则决定。")}/>
    </div>
  </FeatureSection>;
}

export function Cs2InputMapping() {
  const {t}=useTranslation(),calibrating=useRuntimeStore(s=>s.state.cs2Controller?.calibrating??false);
  return <FeatureSection title="鼠标计数换算" help={<p>{t("Aimbot 和 Rage 共用此换算。")}</p>}>
    <div className="aim-fields">
      <SettingChoice setting="cs2InputMapping" label="输入换算" disabled={calibrating}
        options={[{label:"原生 1:1 公式（未实测）",value:"native"},{label:"使用实测换算",value:"measured"}]}
        help={<p>{t("原生公式按盒子计数与游戏原生输入增量 1:1 计算，直接使用 Reader 已包含灵敏度和开镜倍率的导数。使用实测换算仅采用已有的有效校准记录；没有记录时请选择原生公式。")}</p>}/>
    </div>
  </FeatureSection>;
}

export function Cs2TimingSettings() {
  const {t}=useTranslation(),calibrating=useRuntimeStore(s=>s.state.cs2Controller?.calibrating??false);
  return <FeatureSection title="时序与预测" help={<p>{t("Aimbot与Rage共用控制采样年龄、执行延迟及速度预测设置。")}</p>}>
    <div className="aim-fields">
      <SettingField setting="cs2AimMaxAgeMs" label="控制采样年龄上限" suffix="ms" disabled={calibrating}/>
      <SettingField setting="cs2ResponseMs" label="预计执行延迟" suffix="ms" disabled={calibrating}
        help={<p>{t("预计执行延迟表示输出指令发出到输入生效的估计耗时，默认 20 ms，可修改。预测时长还计入本批采样年龄。Triggerbot 使用同一执行延迟，并可在自己的判断条件中另设自修正时间差。")}</p>}/>
      <SettingToggle setting="cs2Prediction" label="按速度补偿执行延迟" disabled={calibrating}/>
    </div>
  </FeatureSection>;
}
