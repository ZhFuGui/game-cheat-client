import { ipcMain, type BrowserWindow } from 'electron'
import { contracts, workspaceContextSchema, isWorkspaceCommand, type CommandName, type WorkspaceContext } from '@mousebox/shared'
import { isTrustedUrl } from './validateSender'
export function registerHandlers(window:BrowserWindow,uiPath:string,devOrigin:string|undefined,invoke:(name:CommandName,args:unknown,context?:WorkspaceContext)=>Promise<unknown>,pulse:()=>void):()=>void{
  const trusted=(event:Electron.IpcMainEvent|Electron.IpcMainInvokeEvent)=>event.sender===window.webContents&&event.senderFrame===window.webContents.mainFrame&&isTrustedUrl(event.senderFrame?.url||'',uiPath,devOrigin)
  ipcMain.handle('mousebox:invoke',async(event,name:unknown,args:unknown,context:unknown)=>{
    if(!trusted(event))throw new Error('Untrusted interface source')
    if(typeof name!=='string'||!Object.prototype.hasOwnProperty.call(contracts,name))throw new Error('Unknown interface command')
    const key=name as CommandName;const input=contracts[key].input.parse(args)
    const scoped=context===undefined?undefined:workspaceContextSchema.parse(context)
    if(isWorkspaceCommand(key,input)&&!scoped)throw new Error('Workspace context is required')
    return contracts[key].output.parse(await invoke(key,input,scoped))
  })
  const heartbeat=(event:Electron.IpcMainEvent)=>{if(trusted(event))pulse()}
  ipcMain.on('mousebox:heartbeat',heartbeat)
  return ()=>{ipcMain.removeHandler('mousebox:invoke');ipcMain.removeListener('mousebox:heartbeat',heartbeat)}
}
