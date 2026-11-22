import {useTranslation} from 'react-i18next'
import {useRuntimeStore} from '~/runtime/state'
import {Section} from '~/components/shared/Content'
import {DeviceSpeedBasis} from './DeviceSpeedBasis'
export const speedNames=['最慢','较慢','正常','较快','最快'] as const
export function SpeedPresets(){
  const {t}=useTranslation(),presets=useRuntimeStore(s=>s.speedPresets)
  return <Section title="五档鼠标移动上限" help={<> <p>{t('数值来自当前原生控制算法。它是每轴鼠标计数上限，不是像素/秒或固定转向角速度；实际移动还受灵敏度、FOV、加速制动和模型上限影响。')}</p> </>}>
    <DeviceSpeedBasis />
    <div className="speed-policy-scroll"><table className="speed-policy-table"><thead><tr><th>{t('档位')}</th><th>{t('上限比例')}</th><th>{t('每轴速度上限')}</th><th>{t('每轴加速度上限')}</th><th>{t('单步上限')}</th></tr></thead><tbody>{presets.map(p=><tr key={p.value}><td>{t(speedNames[p.value])}</td><td>{p.percent}%</td><td>{p.maximumCountsPerSecond} {t('计数/秒')}</td><td>{p.acceleration} {t('计数/秒²')}</td><td>{p.maximumStepCounts} {t('计数')}</td></tr>)}</tbody></table></div>
  </Section>
}
