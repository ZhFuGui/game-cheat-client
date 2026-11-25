import {useTranslation} from "react-i18next";
import type {ReaderStageAverage} from "@mousebox/shared";
import {Hint,Section} from "~/components/shared/Content";
import {useRuntimeStore} from "~/runtime/state";

const reasons={"waiting-data":"等待此订阅的新帧","missing-timing":"当前帧缺少必要时间戳",
  "waiting-clock":"等待有效时钟校准","invalid-timing":"时间戳无法组成有效区间","no-samples":"近2秒没有有效计时样本"} as const;
function Stage({value,transport=false}:{value?:ReaderStageAverage;transport?:boolean}){
  const {t}=useTranslation();
  const ready=value?.status==="ready"&&value.averageMs!==null;
  const reason=value?.status&&value.status!=="ready"?reasons[value.status]:reasons["waiting-data"];
  const detail=ready?t("{{count}}帧有效样本",{count:value.samples}):t(reason);
  const error=ready&&transport&&value.uncertaintyMs!==null?t("时钟估算不确定度 ±{{error}} ms",{error:value.uncertaintyMs.toFixed(2)}):"";
  return <td title={[detail,error].filter(Boolean).join(" · ")}>
    <span className="reader-stage-value">{ready?<>{value.averageMs!.toFixed(2)} <small>ms</small></>:"—"}</span>
    <small className="reader-stage-detail">{detail}{error&&<><br/>{error}</>}</small>
  </td>;
}
export function ReaderChain(){
  const {t}=useTranslation();
  const connected=useRuntimeStore(s=>s.connected);
  const channels=useRuntimeStore(s=>s.state.readerMetrics?.channels);
  const current=connected?channels:undefined;
  return <Section title="订阅耗时" description="近 2 秒平均，按订阅统计。" className="reader-chain" help={<> <p>{t("按完整收到的新数据帧统计；心跳、重复帧不计入。发送准备包含整理、排队与封装，接收终点在解码之前。")}</p><p>{t("前两段使用 Reader 同一时钟；发送至接收通过现有时钟校准估算，缺少时间戳或有效校准时显示 —。")}</p> </>}>
    {current?.length?<div className="reader-chain-scroll"><table aria-label={t("订阅耗时")}>
      <thead><tr><th scope="col">{t("订阅")}</th>
        <th scope="col">{t("读取耗时")}<small>{t("开始读取 → 读完")}</small></th>
        <th scope="col">{t("发送准备")}<small>{t("读完 → 准备发送")}</small></th>
        <th scope="col">{t("发送至接收")}<small>{t("准备发送 → 本软件完整接收")}</small></th>
      </tr></thead>
      <tbody>{current.map(channel=><tr key={`${channel.category}:${channel.connection}`}>
        <th scope="row">{channel.name}<small>TCP · #{channel.connection}</small></th>
        <Stage value={channel.chain?.read}/><Stage value={channel.chain?.prepare}/><Stage transport value={channel.chain?.transport}/>
      </tr>)}</tbody>
    </table></div>:<Hint>{t("暂无订阅")}</Hint>}
  </Section>;
}
