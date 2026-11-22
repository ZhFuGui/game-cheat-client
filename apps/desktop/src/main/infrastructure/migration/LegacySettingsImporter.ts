import { copyFile, mkdir, readFile, stat, access } from 'node:fs/promises'
import { join } from 'node:path'
import { createHash } from 'node:crypto'
import {defaultDataDirectory,previousDataDirectories} from './DataDirectoryMigration'
import type { LegacySource } from '../../domain/ports'

export async function findLegacySources(local:string):Promise<LegacySource[]>{
  const paths=[defaultDataDirectory(local),...previousDataDirectories(local)].map(directory=>join(directory,'settings.ini'))
  const found:LegacySource[]=[]
  for(const path of paths){
    try {
      const data=await readFile(path);const info=await stat(path)
      const hash=createHash('sha256').update(data).digest('hex')
      if(!found.some(s=>s.hash===hash))found.push({id:hash,path,hash,modifiedAt:info.mtime.toISOString()})
    } catch(error) {if((error as NodeJS.ErrnoException).code!=='ENOENT')throw error}
  }
  return found
}
export function legacyUiPreferences(ini:string,fallback:'light'|'dark'='light'):{language:'zh'|'en';colorMode:'light'|'dark'} {
  const entries=parseIni(ini)
  const language=entries['ui/language']==='en'?'en':'zh'
  const color=entries['ui/colorMode']
  return {language,colorMode:color==='dark'||color==='light'?color:fallback}
}
export function parseIni(ini:string):Record<string,string>{
  let section='';const entries:Record<string,string>={}
  for(const line of ini.replace(/^\uFEFF/,'').split(/\r?\n/)){
    const text=line.trim();if(!text||text.startsWith(';')||text.startsWith('#'))continue
    if(text.startsWith('[')&&text.endsWith(']')){section=decodeURIComponent(text.slice(1,-1));continue}
    const equal=text.indexOf('=');if(equal<1)continue
    const key=decodeURIComponent(text.slice(0,equal).replaceAll('\\','/'))
    let value=text.slice(equal+1).trim()
    if(value.startsWith('"')&&value.endsWith('"'))value=value.slice(1,-1)
    value=value.replace(/\\(x[\da-fA-F]{1,4}|[\\nrt"])/g,(_all,escape:string)=>escape[0]==='x'?String.fromCharCode(parseInt(escape.slice(1),16)):({n:'\n',r:'\r',t:'\t','\\':'\\','"':'"'}[escape]||escape))
    entries[section==='General'?key:`${section}/${key}`]=value
  }
  return entries
}
const keyMap:Record<string,string>={host:'reader/host',port:'reader/port',hz:'reader/hz',dpi:'game/dpi',general:'game/general',aim:'game/aim',ads:'game/ads',vertical:'game/vertical',inverted:'game/inverted',skew:'control/skewMs',mapping:'control/inputMapping',mode:'control/outputMode',measurement:'control/measurementMode',speed:'control/speed',part:'control/aimPart',distance:'control/maximumAimDistanceMeters',zeroing:'ballistics/zeroingDistanceMeters',gravity:'ballistics/gravityMetersPerSecondSquared',trigger:'control/triggerButton',renderAdapter:'render/adapter'}
export function legacyOperationalSettings(ini:string):unknown{
  const entries=parseIni(ini),values:Record<string,unknown>={},espValues:Record<string,unknown>={}
  for(const [name,key] of Object.entries(keyMap)){
    if(entries[key]===undefined)continue
    const v=entries[key]
    values[name]=['host','renderAdapter'].includes(name)?v:name==='inverted'?v==='true'||v==='1':Number(v)
    if(typeof values[name]==='number'&&!Number.isFinite(values[name]))delete values[name]
  }
  for(const [key,v] of Object.entries(entries))if(key.startsWith('esp/')){
    const name=key.slice(4);espValues[name]=['marker','aspect','fovAxis'].includes(name)?v:['estimatedHeightMeters','maximumDistanceMeters'].includes(name)?Number(v):v==='true'||v==='1'
  }
  const profiles:Array<Record<string,unknown>>=[]
  const size=Math.min(64,Number(entries['calibration/profiles/size']||0))
  for(let i=1;i<=size;i++){
    const get=(key:string)=>entries[`calibration/profiles/${i}/${key}`]
    profiles.push({mode:Number(get('mode')),profile:get('profile'),yaw:Number(get('yaw')),pitch:Number(get('pitch')),fov:Number(get('fov'))})
  }
  if(!size&&Number(entries['calibration/schema']||1)<2&&(entries['calibration/measured']==='true'||entries['calibration/measured']==='1'))
    profiles.push({mode:0,profile:entries['calibration/profile'],yaw:Number(entries['calibration/yaw']),pitch:Number(entries['calibration/pitch']),fov:Number(entries['calibration/fov'])})
  return {values,espValues,profiles,speedSchema:Number(entries['control/speedSchema']||1)}
}
export async function backupLegacy(source:LegacySource,destination:string):Promise<void>{
  await mkdir(destination,{recursive:true})
  const info=await stat(source.path)
  if(!info.isFile()||createHash('sha256').update(await readFile(source.path)).digest('hex')!==source.hash)throw new Error('Legacy settings changed; select the latest source again')
  const backup=join(destination,source.hash+'.ini')
  try {await access(backup)}catch{await copyFile(source.path,backup)}
}
