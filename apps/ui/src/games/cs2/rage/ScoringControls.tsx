import {useTranslation} from "react-i18next";
import {SettingChoice,SettingField,SettingToggle} from "~/components/shared/SettingField";
import {useRuntimeStore} from "~/runtime/state";
import {Disclosure} from "~/components/shared/Disclosure";
import {ScoringFormulas} from './ScoringFormulas';

export function ScoringControls(){
  const {t}=useTranslation();
  const weight=useRuntimeStore(s=>s.values?.cs2RageThreatWeight??70);
  const aim=useRuntimeStore(s=>s.values?.cs2RageAimWeight??60);
  const legacy=useRuntimeStore(s=>s.values?.cs2RageScoreModel==='legacy');
  const stable=useRuntimeStore(s=>s.values?.cs2RageStableTarget??true);
  return <Disclosure label={t('评分设置')} className="rage-scoring aim-subgroup aim-full-row" heading={<span>{t('评分设置')}</span>}>
    <div className="aim-fields">
      <SettingChoice setting="cs2RageScoreModel" label="评分方案" options={[
        {value:'threat-v2',label:'综合威胁 v2'},{value:'legacy',label:'旧版朝向评分'}]}
        help={t('旧版保留15°单点朝向及相对最高伤害排序，忽略新增扣分和稳定规则；保存的新参数不丢失。')}/>
      <SettingToggle setting="cs2RageHealthNormalization" label="伤害归一化" disabled={legacy}
        help={t('分值=100×预计伤害÷(预计伤害+受击者当前血量)。保留未封顶原始伤害；关闭时使用相对最高伤害，便于对照。')}/>
      <SettingField setting="cs2RageThreatWeight" label="威胁权重" suffix="%" slider={{min:0,max:100}}
        help={t('综合分由威胁分与我方伤害分加权。新版威胁考虑对方视线与自身完整Hitbox、对方武器预计伤害、低弹量及换弹扣分。')}/>
      <div className="aim-value-row rage-derived"><span>{t('伤害权重')}</span><output aria-label={t('伤害权重')}>{100-weight}%</output></div>
      <SettingField setting="cs2RageAimWeight" label="瞄准占比" suffix="%" slider={{min:0,max:100}} disabled={legacy}/>
      <div className="aim-value-row rage-derived"><span>{t('敌伤占比')}</span><output aria-label={t('敌伤占比')}>{100-aim}%</output></div>
      <SettingField setting="cs2RageLowAmmoPenalty" label="低弹量扣分" suffix={t('分')} disabled={legacy}
        help={t('仅对有效弹匣弹量小于等于1扣分；大于1或未知均不扣，未知会标记降级。')}/>
      <SettingField setting="cs2RageReloadPenalty" label="换弹扣分" suffix={t('分')} disabled={legacy}
        help={t('确认正在换弹时扣分，可与低弹量扣分叠加；威胁分最低为0。')}/>
      <SettingToggle setting="cs2RageStableTarget" label="稳定目标选择" disabled={legacy}
        help={t('当前目标仍有效且新目标领先不超过切换分差时保持当前目标；死亡、失效或明显更高分立即重评。同人仍使用伤害最高骨点。')}/>
      <SettingField setting="cs2RageSwitchMargin" label="目标切换分差" suffix={t('分')} disabled={legacy||!stable}
        help={t('默认1分，使用0–100分制；新目标领先超过此值立即切换。')}/>
      <ScoringFormulas/>
    </div>
  </Disclosure>;
}
