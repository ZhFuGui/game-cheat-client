import { useTranslation } from "react-i18next";
import { Section } from "~/components/shared/Content";
import {
  SettingField,
  SettingToggle,
} from "~/components/shared/SettingField";
import { useRuntimeStore } from "~/runtime/state";
import {Select} from '~/components/ui/select';
import {useCommand} from '~/hooks/useCommand';
export function GameInput() {
  const { t } = useTranslation(),
    mapping = useRuntimeStore((s) => s.values?.mapping),
    calibrating = useRuntimeStore((s) => s.state.calibrating),
    mode=useRuntimeStore(s=>s.values?.mode),{run}=useCommand();
  return (
    <>
      <Section>
        <div className="form-grid">

          <SettingField
            setting="general"
            label="常规灵敏度"
            disabled={calibrating}
          />
          <SettingField
            setting="aim"
            label="瞄准灵敏度"
            disabled={calibrating}
          />
          <SettingField
            setting="ads"
            label="开镜灵敏度（当前镜倍）"
            disabled={calibrating}
          />
          <SettingField
            setting="vertical"
            label="垂直倍率"
            disabled={calibrating}
          />
        </div>
        <SettingToggle
          setting="inverted"
          label="反转垂直方向"
          disabled={calibrating}
         help={<> <p>{t('DPI 用于学习速度缩放和实测校准标识；原始鼠标计数不在 FOV 换算中再次乘以 DPI。')}</p> </>}/>
      </Section>
      <Section title="视角换算" help={<> <p>{t('FOV 自动更新；灵敏度模式仍由你选择。普通视角使用常规灵敏度，瞄准使用 aim，开镜使用 ADS。')}</p><p>{t('换算方式与实测记录位于本游戏“校准”的高级区域；DPI 位于全局“输入设备”。')}</p> </>}>
        <div className="form-stack">
          <div className="field"><label>{t('当前灵敏度模式（手动）')}</label><Select
            label={t('当前灵敏度模式（手动）')}
            value={mapping===0&&mode===1?0:mode??''}
            onChange={value=>void run('settings.set',{key:'mode',value})}
            disabled={calibrating||mode===undefined}
            options={[
              ...(mapping === 1
                ? [{ label: "自动匹配实测记录", value: -1 }]
                : []),
              ...(mapping===0?[{label:'普通视角（第三／第一人称）',value:0},{label:'瞄准',value:2},{label:'开镜',value:3}]:["第三人称", "第一人称", "瞄准", "开镜"].map(
                (label, value) => ({ label, value }),
              )),
            ].map(o=>({...o,label:t(o.label)}))}
          /></div>
        </div>
      </Section>
    </>
  );
}
