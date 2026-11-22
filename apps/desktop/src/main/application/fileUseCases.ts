import { dialog, type BrowserWindow } from 'electron'
export class FileUseCases {
  private allowedReads=new Set<string>()
  private allowedWrites=new Set<string>()
  constructor(private readonly window:BrowserWindow,private readonly occlude:(value:boolean)=>Promise<void>){}
  async open(purpose:string,multiple=false):Promise<{paths:string[]}>{
    await this.occlude(true)
    try{
      const result=await dialog.showOpenDialog(this.window,{title:purpose,filters:purpose==='recording'?[]:[{name:'JSON',extensions:['json']}],properties:purpose==='recording'?['openDirectory']:multiple?['openFile','multiSelections']:['openFile']})
      const paths=result.canceled?[]:result.filePaths
      for(const path of paths)this.allowedReads.add(path)
      return {paths}
    } finally {await this.occlude(false)}
  }
  async save(purpose:string):Promise<{path:string|null}>{
    await this.occlude(true)
    try{
      const csv=purpose==='mouse-reports'
      const result=await dialog.showSaveDialog(this.window,{title:purpose,defaultPath:`mousebox-${purpose}.${csv?'csv':'json'}`,filters:[{name:csv?'CSV':'JSON',extensions:[csv?'csv':'json']}]})
      const path=result.canceled?null:result.filePath||null
      if(path)this.allowedWrites.add(path)
      return {path}
    }finally{await this.occlude(false)}
  }
  validate(command:string,payload:Record<string,unknown>):void {
    const reads=['recording.load','samples.import','model.import']
    const writes=['calibration.export','samples.export','model.export','logs.export','mouseReports.export']
    if(reads.includes(command))for(const path of payload.paths as string[]||[payload.path])if(typeof path!=='string'||!this.allowedReads.has(path))throw new Error('Select the source file with the file dialog')
    if(writes.includes(command)&&(typeof payload.path!=='string'||!this.allowedWrites.has(payload.path)))throw new Error('Select the destination with the save dialog')
  }
}
