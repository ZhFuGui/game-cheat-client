import { copyFileSync, existsSync, readFileSync, mkdirSync, renameSync, writeFileSync } from 'node:fs'
import { dirname } from 'node:path'
import type { UiPreferences } from '../../domain/ports'
import { clampSidebarWidth, SIDEBAR_DEFAULT_WIDTH,SIDEBAR_MAX_WIDTH,preferencesSchema } from '@mousebox/shared'
export const defaultPreferences:UiPreferences={colorMode:'light',language:'zh',width:1280,height:840,sidebarWidth:SIDEBAR_DEFAULT_WIDTH,sidebarCollapsed:false}
export class UiPreferenceStore {
  private value:UiPreferences
  constructor(private readonly path:string,legacyColorMode:UiPreferences['colorMode']='light'){
    this.value={...defaultPreferences}
    if(existsSync(path)){
      const parsed=JSON.parse(readFileSync(path,'utf8')) as Omit<Partial<UiPreferences>,'colorMode'>&{colorMode?:UiPreferences['colorMode']|'system'}
      const legacy=parsed.colorMode==='system'
      const colorMode=parsed.colorMode==='system'?legacyColorMode:parsed.colorMode??defaultPreferences.colorMode
      // Migrate integer widths from previous ranges, which allowed up to 400px.
      const width=parsed.sidebarWidth
      const resize=typeof width==='number'&&Number.isInteger(width)&&width>SIDEBAR_MAX_WIDTH&&width<=400
      this.value=this.validate({...this.value,...parsed,colorMode,...(resize?{sidebarWidth:SIDEBAR_MAX_WIDTH}:{})})
      if(legacy){
        const backup=path+'.before-explicit-color-mode.bak'
        if(!existsSync(backup))copyFileSync(path,backup)
      }
      if(resize){
        const backup=path+'.before-sidebar-150-240.bak'
        if(!existsSync(backup))copyFileSync(path,backup)
      }
      if(legacy||resize)this.set({})
    }
  }
  private validate(value:UiPreferences):UiPreferences {
    preferencesSchema.parse({language:value.language,colorMode:value.colorMode,themePreset:value.themePreset,sidebarWidth:value.sidebarWidth,sidebarCollapsed:value.sidebarCollapsed,
      pageTabOrder:value.pageTabOrder,espPlacement:value.espPlacement,espCapture:value.espCapture,espRenderer:value.espRenderer})
    if(!['light','dark'].includes(value.colorMode)||!['zh','en'].includes(value.language))throw new Error('Invalid UI preferences')
    if(!Number.isFinite(value.width)||!Number.isFinite(value.height))throw new Error('Invalid window size')
    return {...value,width:Math.max(960,Math.min(7680,value.width)),height:Math.max(640,Math.min(4320,value.height)),
      sidebarWidth:clampSidebarWidth(value.sidebarWidth ?? SIDEBAR_DEFAULT_WIDTH)}
  }
  get():UiPreferences{return {...this.value}}
  set(patch:Partial<UiPreferences>):UiPreferences{
    const next=this.validate({...this.value,...patch})
    mkdirSync(dirname(this.path),{recursive:true})
    const temp=this.path+'.tmp';writeFileSync(temp,JSON.stringify(next,null,2),'utf8');renameSync(temp,this.path)
    this.value=next;return this.get()
  }
}
