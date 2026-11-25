import {useTranslation} from "react-i18next";
import {FeatureSection} from "~/components/shared/FeatureSection";
import {SettingField,SettingToggle} from "~/components/shared/SettingField";

export function TargetSettings(){
  const {t}=useTranslation();
  return <FeatureSection title="包含对象" help={t("只使用 Reader 明确提供的敌我关系。未分类人物和本地人物不作为目标；当前 Reader 没有存活字段，不能自动排除尸体。")}>
    <div className="aim-fields">
      <SettingToggle setting="valorantAimEnemies" label="敌方人物"/>
      <SettingToggle setting="valorantAimTeammates" label="友方人物"/>
    </div>
  </FeatureSection>;
}
export function RuntimeSettings(){
  const {t}=useTranslation();
  return <FeatureSection title="运行条件"><div className="aim-fields">
    <SettingToggle setting="valorantCheckForeground" label="检测游戏窗口在前台"
      help={t("检查本机配置的游戏进程。Reader 在远端时可关闭此检查；远端前台状态不会被假设为已知。")}/>
    <SettingField setting="valorantAimMaxAgeMs" label="控制采样年龄上限" suffix="ms" slider={{min:10,max:1000}}
      help={t("分别检查位置、骨骼和相机从采样到当前的年龄；缺失、过期、时钟对齐不可用或三流时间差过大时暂停跟随。")}/>
    <SettingField setting="valorantResponseMs" label="预计执行延迟" suffix="ms" slider={{min:0,max:250}}
      help={t("手动设置的输出响应时间模型，用来扣除尚未反映到相机的已提交位移；不是硬件实测值，也不用于伪造目标运动速度。")}/>
  </div></FeatureSection>;
}
export function InputMapping(){
  const {t}=useTranslation();
  return <FeatureSection title="手动鼠标换算" help={t("填写当前游戏设置下，每个正鼠标计数引起的有符号角度变化。0 表示未知，未填写两轴时不输出。改变灵敏度或缩放后需重新核对；这不是 Reader 原生输入参数。") }>
    <div className="aim-fields">
      <SettingField setting="valorantYawPerCount" label="水平角度/鼠标计数" suffix="°/count"
        help={t("正鼠标计数为向右，正水平角为向右；如横轴反向，应填写负值。")}/>
      <SettingField setting="valorantPitchPerCount" label="纵向角度/鼠标计数" suffix="°/count"
        help={t("正鼠标计数为向下，正俯仰角为抬头，因此普通纵轴通常填写负值。请使用自己的换算结果。")}/>
    </div>
  </FeatureSection>;
}
