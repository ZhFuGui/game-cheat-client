import { screen,type BrowserWindow } from 'electron'
import { surfaceSchema, type UiPreferences } from '@mousebox/shared'
import type { NativePort } from '../domain/ports'
import { surfaceVisibility } from '../application/surfaceVisibility'
export class NativeSurfaces {
  private overlay=false
  constructor(private readonly window:BrowserWindow,private readonly native:NativePort,private readonly allowOverlay=true){}
  async configure(preferences:UiPreferences):Promise<void>{
    await this.native.request('surface.configure',{placement:preferences.espPlacement??'embedded',
      capture:preferences.espCapture??'include',renderer:preferences.espRenderer??'default'})
    this.overlay=preferences.espPlacement==='overlay'
  }
  async update(payload:unknown):Promise<unknown>{
    const s=surfaceSchema.parse(payload)
    const handle=this.window.getNativeWindowHandle()
    const parentHandle=(handle.length===8?handle.readBigUInt64LE():BigInt(handle.readUInt32LE())).toString()
    const [width,height]=this.window.getContentSize()
    const scale=screen.getDisplayMatching(this.window.getBounds()).scaleFactor*this.window.webContents.getZoomFactor()
    if(s.x<0||s.y<0||s.x+s.width>width+2||s.y+s.height>height+2)throw new Error('Surface lies outside the application window')
    const visibility=surfaceVisibility(s.visible,s.active,this.window.isVisible(),this.window.isMinimized())
    if(this.overlay&&!this.allowOverlay){visibility.active=false;visibility.visible=false}
    return this.native.request('surface.update',{...s,scale,parentHandle,parentPid:process.pid,
      attach:s.visible,
      ...visibility})
  }
  async hide():Promise<void>{await this.native.request('surface.hide',{})}
  async minimize():Promise<void>{await this.native.request('surface.minimize',{})}
}
