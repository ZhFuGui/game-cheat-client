import { contextBridge, ipcRenderer } from 'electron'
import type { CommandName, CommandInput, Topic, DesktopApi, EventPayload,CommandResult, WorkspaceContext } from '@mousebox/shared'
// This entry has no runtime import besides Electron: sandboxed preload cannot
// require arbitrary packages. Main validates every command and event.
const topics:Readonly<Record<Topic,true>>={workspace:true,application:true,devices:true,values:true,espValues:true,speedPresets:true,state:true,learning:true,mouseReports:true,ports:true,graphicsAdapters:true,algorithm:true,logs:true,preferences:true,bridge:true,error:true}
const api:DesktopApi={
  invoke:<K extends CommandName>(name:K,args:CommandInput<K>,context?:WorkspaceContext)=>ipcRenderer.invoke('mousebox:invoke',name,args,context) as Promise<CommandResult<K>>,
  subscribe:<K extends Topic>(topic:K,listener:(payload:EventPayload<K>,context?:WorkspaceContext)=>void)=>{
    if(!Object.prototype.hasOwnProperty.call(topics,topic))throw new Error('Unknown subscription')
    const callback=(_event:Electron.IpcRendererEvent,payload:EventPayload<K>,context?:WorkspaceContext)=>listener(payload,context)
    ipcRenderer.on(`mousebox:event:${topic}`,callback)
    return ()=>ipcRenderer.removeListener(`mousebox:event:${topic}`,callback)
  }
}
contextBridge.exposeInMainWorld('mousebox',api)
setInterval(()=>ipcRenderer.send('mousebox:heartbeat'),500)
