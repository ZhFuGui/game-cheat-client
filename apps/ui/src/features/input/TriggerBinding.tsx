import {useId,useState} from "react";
import {useTranslation} from "react-i18next";
import {deviceMouseBindings,activationBindingSchema,localBindingLabel,type SettingKey} from "@mousebox/shared";
import {useSettingBinding} from "~/hooks/useSettingBinding";
import {Select} from "~/components/ui/select";
import {Button} from "~/components/ui/button";
export function TriggerBinding({setting,disabled=false}:{setting:SettingKey;disabled?:boolean}){
  const {t}=useTranslation(),id=useId();
  const binding=useSettingBinding({setting});
  const [recording,setRecording]=useState(false),[error,setError]=useState("");
  const value=typeof binding.value==="string"?binding.value:"";
  const options=deviceMouseBindings.map(b=>({value:String(b.value),label:t(b.label)}));
  if(value.startsWith("key:"))options.push({value,label:`${localBindingLabel(value)} · ${t("本机键盘")}`});
  return <div className="field"><label htmlFor={id}>{t("触发键")}</label>
    <Select id={id} value={value} disabled={disabled||!value} options={options} onChange={v=>void binding.save(String(v))}/>
    <Button disabled={disabled||!value} aria-pressed={recording} onClick={()=>{setRecording(!recording);setError("");}}
      onBlur={()=>setRecording(false)} onKeyDown={event=>{
        if(!recording||event.repeat)return;
        event.preventDefault();event.stopPropagation();
        if(event.key==="Escape"){setRecording(false);return;}
        const next=`key:${event.code}`;
        if(!activationBindingSchema.safeParse(next).success){setError(t("此按键暂不支持；Esc取消，F8保留给设备测试。"));return;}
        setRecording(false);setError("");void binding.save(next);
      }}>{t(recording?"请按一个键（Esc取消）":"录入键盘按键")}</Button>
    {error&&<small className="field-error" role="status">{error}</small>}
  </div>;
}
