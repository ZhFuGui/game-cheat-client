import {useEffect,useRef,useState} from 'react'
import {useTranslation} from 'react-i18next'
import type {MouseReportsResult} from '@mousebox/shared'

export type ReportChannel='magnitude'|'x'|'y'
type Histogram=MouseReportsResult['histograms']['x']
export const formatReportNumber=(value:number|null|undefined,digits=1)=>value==null?'—':new Intl.NumberFormat(undefined,{maximumFractionDigits:digits}).format(value)
function useChartWidth(){
  const ref=useRef<HTMLDivElement>(null),[width,setWidth]=useState(640)
  useEffect(()=>{if(!ref.current)return;const element=ref.current;const update=()=>{if(element.clientWidth)setWidth(Math.max(280,element.clientWidth))};update();const observer=new ResizeObserver(update);observer.observe(element);return()=>observer.disconnect()},[])
  return {ref,width}
}
export function DistributionChart({bins,label,unit}:{bins:Histogram;label:string;unit:string}){
  const {t}=useTranslation(),{ref,width}=useChartWidth(),height=220,left=46,right=16,top=15,bottom=36
  const maximum=Math.max(1,...bins.map(b=>b.count)),total=bins.reduce((n,b)=>n+b.count,0),plotWidth=width-left-right,plotHeight=height-top-bottom
  const ticks=[...new Set([0,Math.ceil(maximum/2),maximum])]
  return <div className="report-chart" ref={ref}>
    {!bins.length?<p className="empty-state">{t('暂无报文数据')}</p>:<>
      <svg width="100%" height={height} viewBox={`0 0 ${width} ${height}`} role="img" aria-label={label}>
        {ticks.map(tick=><g key={tick}><line x1={left} x2={width-right} y1={top+plotHeight*(1-tick/maximum)} y2={top+plotHeight*(1-tick/maximum)} className="chart-grid"/><text x={left-9} y={top+plotHeight*(1-tick/maximum)+4} textAnchor="end" className="chart-label">{formatReportNumber(tick,0)}</text></g>)}
        {bins.map((bin,i)=>{const barWidth=plotWidth/bins.length,barHeight=bin.count/maximum*plotHeight;return <rect key={i} x={left+i*barWidth+0.5} y={top+plotHeight-barHeight} width={Math.max(0.5,barWidth-1)} height={barHeight} className="chart-bar"><title>{formatReportNumber(bin.low,3)}–{formatReportNumber(bin.high,3)} {unit} · {formatReportNumber(bin.count,0)} {t('条报文')} · {formatReportNumber(bin.count/total*100,2)}%</title></rect>})}
        <text x={left} y={height-13} className="chart-label">{formatReportNumber(bins[0].low,2)}</text><text x={width-right} y={height-13} textAnchor="end" className="chart-label">{formatReportNumber(bins.at(-1)!.high,2)} {unit}</text>
      </svg>
      <details className="report-data-details"><summary>{t('查看分布明细')}</summary><div><table><thead><tr><th>{t('区间')}</th><th>{t('报文数')}</th><th>{t('占比')}</th></tr></thead><tbody>{bins.map((bin,i)=><tr key={i}><td>{formatReportNumber(bin.low,3)}–{formatReportNumber(bin.high,3)} {unit}</td><td>{formatReportNumber(bin.count,0)}</td><td>{formatReportNumber(total?bin.count/total*100:0,2)}%</td></tr>)}</tbody></table></div></details>
    </>}
  </div>
}
export function ReportTimeline({timeline,channel}:{timeline:MouseReportsResult['timeline'];channel:ReportChannel}){
  const {t}=useTranslation(),{ref,width}=useChartWidth(),[hover,setHover]=useState<number|null>(null)
  const height=250,left=52,right=16,top=18,bottom=36,plotWidth=width-left-right,plotHeight=height-top-bottom
  const values=timeline.map(bin=>({start:bin.startMs,end:bin.endMs,n:bin.reports,min:bin[`${channel}Min`],max:bin[`${channel}Max`],mean:bin[`${channel}Mean`]}))
  const low=Math.min(0,...values.flatMap(b=>b.min===null?[]:[b.min])),high=Math.max(1,...values.flatMap(b=>b.max===null?[]:[b.max])),duration=timeline.at(-1)?.endMs||1
  const y=(n:number)=>top+(high-n)/(high-low)*plotHeight,x=(n:number)=>left+n/duration*plotWidth
  let connected=false,path=''
  for(const value of values){if(value.mean===null){connected=false;continue}path+=`${connected?'L':'M'}${x((value.start+value.end)/2)},${y(value.mean)} `;connected=true}
  const selected=hover===null?undefined:values[hover]
  return <div className="report-chart" ref={ref}>
    <div className="chart-legend"><span><i className="chart-range-key"/>{t('区间最小—最大')}</span><span><i className="chart-mean-key"/>{t('区间平均')}</span><small>{t('移入或使用方向键查看')}</small></div>
    <svg width="100%" height={height} viewBox={`0 0 ${width} ${height}`} role="img" aria-label={t('报文计数随时间变化图')} tabIndex={0}
      onPointerMove={event=>{const rect=event.currentTarget.getBoundingClientRect();const at=(event.clientX-rect.left)/Math.max(1,rect.width)*width;setHover(Math.max(0,Math.min(values.length-1,Math.floor((at-left)/plotWidth*values.length))))}} onPointerLeave={()=>setHover(null)}
      onKeyDown={event=>{
        if(event.key==='ArrowRight'||event.key==='ArrowLeft'){
          event.preventDefault()
          setHover(old=>Math.max(0,Math.min(values.length-1,(old??0)+(event.key==='ArrowRight'?1:-1))))
        }
      }}>
      {[0,0.5,1].map(f=><g key={f}><line x1={left} x2={width-right} y1={top+f*plotHeight} y2={top+f*plotHeight} className="chart-grid"/><text x={left-9} y={top+f*plotHeight+4} textAnchor="end" className="chart-label">{formatReportNumber(high-f*(high-low),1)}</text></g>)}
      {values.map((b,i)=>b.min!==null&&b.max!==null?<rect key={i} x={x(b.start)} width={Math.max(1,x(b.end)-x(b.start))} y={y(b.max)-0.5} height={Math.max(1,y(b.min)-y(b.max))} className="chart-range"/>:null)}
      <path d={path} className="chart-mean"/>
      {values.map((b,i)=>b.mean!==null&&(i===0||values[i-1].mean===null)&&(i===values.length-1||values[i+1].mean===null)?<circle key={i} cx={x((b.start+b.end)/2)} cy={y(b.mean)} r={2.5} className="chart-point"/>:null)}
      {[0,0.5,1].map(f=><text key={f} x={left+f*plotWidth} y={height-12} textAnchor={f===0?'start':f===1?'end':'middle'} className="chart-label">{formatReportNumber(duration*f/1000,2)} s</text>)}
      {selected&&<line x1={x((selected.start+selected.end)/2)} x2={x((selected.start+selected.end)/2)} y1={top} y2={height-bottom} className="chart-cursor"/>}
    </svg>
    <output className="chart-readout">{selected?<>{formatReportNumber(selected.start,1)}–{formatReportNumber(selected.end,1)} ms · {formatReportNumber(selected.n,0)} {t('条报文')} · {selected.n?`${formatReportNumber(selected.min,2)}–${formatReportNumber(selected.max,2)} ${t('计数')} · ${t('平均')} ${formatReportNumber(selected.mean,2)}`:t('此时间段未收到报文')}</>:t('图表保留每个时间段的最小值、最大值与平均值；空白区间不补造报文。')}</output>
  </div>
}
