import {useTranslation} from 'react-i18next';
import {FeatureSection} from '~/components/shared/FeatureSection';
import {useRuntimeStore} from '~/runtime/state';

export function ScoringDetails(){
  const {t}=useTranslation(),score=useRuntimeStore(s=>s.state.cs2Rage?.scoring);
  const raw=useRuntimeStore(s=>s.state.cs2Rage?.damage);
  const number=(value:number|null|undefined)=>value==null?'—':value.toFixed(1);
  const rows:[string,string][]=[
    ['瞄准分',number(score?.aim)],['敌方预计伤害',number(score?.incomingDamage)],
    ['敌方伤害分',number(score?.incomingDamageScore)],['低弹量扣分',score?`${score.lowAmmoDeduction?'−':''}${number(score.lowAmmoDeduction)}`:'—'],
    ['换弹扣分',score?`${score.reloadDeduction?'−':''}${number(score.reloadDeduction)}`:'—'],['威胁分',number(score?.threatScore)],
    ['我方预计伤害',number(raw)],['我方伤害分',number(score?.outgoingDamageScore)],['综合分',number(score?.totalScore)],
  ];
  return <FeatureSection title="最近评分拆解" className="rage-score-details">
    {!score?<p className="hint">{t('等待有效评分结果')}</p>:<>
      <p className="rage-score-state" role="status">{t(score.model==='legacy'?'旧版朝向评分':'综合威胁 v2')}
        {score.degraded&&` · ${t('部分威胁数据未知，已降级评分')}`}
        {score.stabilized&&` · ${t('分数接近，保持当前目标')}`}
        {score.assumedFriendlyFire&&` · ${t('友军伤害为条件估算')}`}</p>
      <dl>{rows.map(([label,value])=><div key={label}><dt>{t(label)}</dt><dd>{value}</dd></div>)}</dl>
      <p className="rage-score-state">{t('敌方武器编号')} {score.weapon??'—'} · {t('弹匣')} {score.clip??'—'} · {t('换弹状态')} {t(score.reloading==null?'未知':score.reloading?'换弹中':'未换弹')}</p>
    </>}
  </FeatureSection>;
}
