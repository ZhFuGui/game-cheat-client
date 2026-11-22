import { AUTOMATIC_CONNECTION_INTERVAL_MS, type Bootstrap, type DeviceState, type RuntimeState } from '@mousebox/shared'
import type { NativePort } from '../domain/ports'

// Startup connection orchestration only. No input, calibration, or model commands.
export class AutomaticConnections {
  private started=false
  private disposed=false
  private manageBox=true
  private inFlight=false
  private timer?:ReturnType<typeof setTimeout>
  private ports:Bootstrap['ports']=[]
  private state:RuntimeState={}
  private generation=0
  constructor(private readonly native:Pick<NativePort,'request'>,private readonly onError:(error:Error)=>void=()=>{}){}
  start(snapshot:Pick<Bootstrap,'values'|'ports'|'state'|'workspace'>):void{
    if(this.started||this.disposed)return
    this.started=true;this.ports=snapshot.ports;this.state=snapshot.state
    const generation=this.generation
    const sourceAvailable=!snapshot.workspace||snapshot.workspace.games.some(g=>g.id===snapshot.workspace?.active?.gameId&&g.status==='available')
    if(sourceAvailable&&!snapshot.state.readerRunning){
      const {host,port,hz}=snapshot.values
      void this.native.request('reader.connect',{host,port,hz}).catch(error=>{if(generation===this.generation)this.report(error)})
    }
    this.schedule(0)
  }
  reset(snapshot:Pick<Bootstrap,'values'|'ports'|'state'|'workspace'>):void{
    ++this.generation;this.cancelTimer();this.started=false;this.start(snapshot)
  }
  invalidateSource():void{++this.generation}
  updatePorts(ports:Bootstrap['ports']):void{
    const changed=ports.map(p=>p.port).join('\n')!==this.ports.map(p=>p.port).join('\n')
    this.ports=ports
    if(changed){this.cancelTimer();this.schedule(0)}
  }
  updateState(state:RuntimeState):void{
    this.state=state
    if(!this.available())this.cancelTimer()
    else this.schedule(this.retryDelay())
  }
  updateDevices(devices:DeviceState):void{
    const previous=this.state.connections??{reader:{phase:'closed' as const,message:''},serial:{phase:'closed' as const,message:''},box:{phase:'closed' as const,message:''}}
    this.updateState({...this.state,...devices,connections:{...previous,...devices.connections}})
  }
  manual(command:string):void{
    if(command==='box.connect'||command==='box.disconnect'){
      this.manageBox=false;this.cancelTimer()
    }
  }
  dispose():void{this.disposed=true;this.manageBox=false;this.cancelTimer()}
  private available():boolean{
    return this.started&&!this.disposed&&this.manageBox&&!this.inFlight&&this.ports.length===1&&
      (!this.state.boxReady||this.state.connections?.box.retryable===true)&&this.state.connections?.box.phase!=='connecting'
  }
  private retryDelay():number{return AUTOMATIC_CONNECTION_INTERVAL_MS}
  private cancelTimer():void{if(this.timer)clearTimeout(this.timer);this.timer=undefined}
  private schedule(delay:number):void{
    if(this.timer||!this.available())return
    this.timer=setTimeout(()=>{this.timer=undefined;void this.connectBox()},delay)
    this.timer.unref?.()
  }
  private report(error:unknown):void{
    if(!this.disposed)this.onError(error instanceof Error?error:new Error(String(error)))
  }
  private async connectBox():Promise<void>{
    if(!this.available())return
    this.inFlight=true
    try{await this.native.request('box.connect',{port:this.ports[0].port})}
    catch(error){this.report(error)}
    finally{this.inFlight=false;this.schedule(this.retryDelay())}
  }
}
