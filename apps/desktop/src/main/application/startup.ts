export type StartupOptions={route:string;algorithmOpen:boolean;minimized:boolean;settings?:string;evidence?:string;samples:string[];model?:string;trace?:string;renderReport?:string;screenshot?:string;calibrationPort?:string;calibrationOutput?:string;startupReport?:string;connectionReport?:string;timingReport?:string}
export function automaticConnectionsEnabled(startup:StartupOptions,isolated:boolean,smoke:boolean,args:string[]):boolean{
  return !(isolated||smoke||startup.evidence||startup.screenshot||startup.renderReport||startup.calibrationPort||args.includes('--no-auto-connect'))
}
export function startupOptions(args:string[]):StartupOptions{
  const result:StartupOptions={route:'/',algorithmOpen:false,minimized:false,samples:[]}
  const panes:Record<string,string>={appearance:'general',language:'general',device:'firmware',general:'general',firmware:'firmware',input:'input',calibration:'calibration',rendering:'rendering','aimbot-algorithm':'aimbot-algorithm','aimbot-mouse':'aimbot-mouse'}
  for(let i=0;i<args.length;i++){
    const key=args[i]
    if(key.startsWith('--timing-report=')){const path=key.slice('--timing-report='.length);if(!path)throw new Error('Missing startup argument: --timing-report');result.timingReport=path;continue}
    if(key==='--reader')result.route='/'
    else if(key==='--dashboard')result.route='/dashboard'
    else if(key==='--esp')result.route='/esp'
    else if(key==='--recoil')result.route='/recoil'
    else if(key==='--triggerbot')result.route='/triggerbot'
    else if(key==='--rage')result.route='/rage'
    else if(key==='--controls'||key==='--aimbot')result.route='/aimbot'
    else if(key==='--learning')result.route='/mouse-debug?section=learning'
    else if(key==='--mouse-characteristics')result.route='/mouse-debug?section=characteristics'
    else if(key==='--logs')result.route='/logs'
    else if(key==='--appearance')result.route='/settings?section=general'
    else if(key==='--aimbot-algorithm'){result.route='/settings?section=aimbot-algorithm';result.algorithmOpen=true}
    else if(key==='--minimized')result.minimized=true
    else if(['--settings-section','--settings','--evidence','--speed-samples','--speed-model','--trace','--render-report','--screenshot','--calibrate','--calibration-output','--startup-report','--connection-report','--timing-report'].includes(key)){
      const value=args[++i];if(!value||value.startsWith('--'))throw new Error(`Missing startup argument: ${key}`)
      if(key==='--settings-section'){if(!panes[value])throw new Error('Unknown settings section');result.route=`/settings?section=${panes[value]}`}
      else if(key==='--settings')result.settings=value
      else if(key==='--evidence')result.evidence=value
      else if(key==='--speed-samples')result.samples.push(value)
      else if(key==='--speed-model')result.model=value
      else if(key==='--trace')result.trace=value
      else if(key==='--render-report')result.renderReport=value
      else if(key==='--screenshot')result.screenshot=value
      else if(key==='--calibrate')result.calibrationPort=value
      else if(key==='--calibration-output')result.calibrationOutput=value
      else if(key==='--startup-report')result.startupReport=value
      else if(key==='--connection-report')result.connectionReport=value
      else if(key==='--timing-report')result.timingReport=value
    }
  }
  return result
}
