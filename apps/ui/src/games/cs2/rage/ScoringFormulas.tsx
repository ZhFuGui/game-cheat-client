import {useId} from 'react';
import {useTranslation} from 'react-i18next';
import {useRuntimeStore} from '~/runtime/state';

/** Documents the native scoring policy; does not evaluate scores or write settings. */
export function ScoringFormulas(){
  const {t,i18n}=useTranslation(),heading=useId();
  const legacy=useRuntimeStore(s=>s.values?.cs2RageScoreModel==='legacy');
  const weight=useRuntimeStore(s=>s.values?.cs2RageThreatWeight??70);
  const aim=useRuntimeStore(s=>s.values?.cs2RageAimWeight??60);
  const lowAmmo=useRuntimeStore(s=>s.values?.cs2RageLowAmmoPenalty??10);
  const reload=useRuntimeStore(s=>s.values?.cs2RageReloadPenalty??60);
  const normalized=useRuntimeStore(s=>s.values?.cs2RageHealthNormalization??true);
  const stable=useRuntimeStore(s=>s.values?.cs2RageStableTarget??true);
  const margin=useRuntimeStore(s=>s.values?.cs2RageSwitchMargin??1);
  const format=new Intl.NumberFormat(i18n.resolvedLanguage,{useGrouping:false,maximumSignificantDigits:15});
  const rows:[string,string,string?][]=[
    ['综合分',t('综合分 = {{threat}}% × 威胁分 + {{damage}}% × 我方伤害分',{
      threat:format.format(weight),damage:format.format(100-weight)})],
  ];
  if(legacy){
    rows.push(['威胁分',t('威胁分 = 100 × 限制到0–1(1 − 夹角 ÷ 15°)'),
      t('夹角是敌方视线与“敌方视点→我的视点”方向的夹角；未知朝向按0分计算。')]);
  }else{
    rows.push(
      ['威胁分',t('威胁分 = 限制到0–100({{aim}}% × 瞄准分 + {{incoming}}% × 敌方伤害分 − 低弹量扣分 − 换弹扣分)',{
        aim:format.format(aim),incoming:format.format(100-aim)}),t('限制到0–100：低于0取0，高于100取100。')],
      ['瞄准分',t('视线命中我的任一Hitbox：100；背向或无可接近命中体：0；其余 = min(99.9999, 100 ÷ max(1, k))'),
        t('k是让射线触及自身命中体所需的最小尺寸放大倍数；越接近命中，瞄准分越高。')],
    );
  }
  if(!legacy&&normalized){
    rows.push(
      ['我方伤害分',t('我方伤害分 = 100 × 我方预计伤害 ÷ (我方预计伤害 + 目标当前血量)')],
      ['敌方伤害分',t('敌方伤害分 = 100 × 敌方预计伤害 ÷ (敌方预计伤害 + 我的当前血量)')],
    );
  }else{
    rows.push(['我方伤害分',t('我方伤害分 = 100 × 我方预计伤害 ÷ 当前候选最高我方伤害')]);
    if(!legacy)rows.push(['敌方伤害分',t('敌方伤害分 = 100 × 敌方预计伤害 ÷ 当前候选最高敌方伤害'),
      t('我方和敌方分别取最大值；最大值为0时，对应伤害分为0。')]);
  }
  if(!legacy){
    rows.push(
      ['低弹量扣分',t('已知弹匣为0或1发：扣{{points}}分；其他情况扣0分。',{points:format.format(lowAmmo)})],
      ['换弹扣分',t('确认正在换弹：扣{{points}}分；其他情况扣0分。',{points:format.format(reload)})],
      ['稳定目标选择',stable?t('新目标综合分 − 当前目标综合分 > {{margin}}分 → 立即切换；否则保持仍有效的当前目标。',{
        margin:format.format(margin)}):t('已关闭：直接选择综合分最高的目标。'),t('当前目标死亡或失效时立即重新选择。')],
    );
  }
  return <section className="rage-score-formulas aim-full-row" aria-labelledby={heading}>
    <h3 id={heading}>{t('当前计算公式')}</h3>
    <p>{t('所有分值为0–100；公式随已保存的设置更新。')}</p>
    <dl>{rows.map(([label,formula,note])=><div key={label}>
      <dt>{t(label)}</dt><dd><div className="rage-score-equation">{formula}</div>{note&&<p>{note}</p>}</dd>
    </div>)}</dl>
    {legacy?<p>{t('旧版不使用低弹量、换弹扣分、健康归一化及目标保持规则。')}</p>:<>
      <p>{t('瞄准或敌伤数据缺失时，仅在已知且占比大于0的项之间重新分配比例；没有可用的威胁正项时，综合分 = 我方伤害分。未知弹匣或换弹状态不扣分。')}</p>
      <p>{t('有限采样全被掩体挡住不会直接清零威胁；只有已验证敌方完全无伤害能力才直接置0。')}</p>
    </>}
    <p>{t('预计伤害计入武器、距离、护甲与静态掩体，保留未封顶伤害。同一人始终优先最高伤害瞄点；评分用于排序，不是命中率。')}</p>
  </section>;
}
