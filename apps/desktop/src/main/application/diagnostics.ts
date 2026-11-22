import {nativeImage,type BrowserWindow} from 'electron'
import {mkdir,readFile,writeFile,unlink} from 'node:fs/promises'
import {dirname,join,resolve} from 'node:path'
import {randomUUID} from 'node:crypto'
import type {NativePort} from '../domain/ports'
type GpuCapture={available:boolean;width?:number;height?:number;x?:number;y?:number}
type SurfaceReport={surface:{clientWidth:number;clientHeight:number}}
export async function saveDiagnosticScreenshot(window:BrowserWindow,native:NativePort,dataDirectory:string,path:string):Promise<void>{
  const web=await window.webContents.capturePage(),size=web.getSize()
  const buffer=web.toBitmap()
  const temporary=join(dataDirectory,'diagnostics',randomUUID()+'.bgra')
  const capture=await native.request('render.capture',{path:temporary}) as GpuCapture
  if(capture.available&&capture.width&&capture.height){
    try{
      const surface=await native.request('render.inspect') as SurfaceReport
      const xScale=size.width/surface.surface.clientWidth,yScale=size.height/surface.surface.clientHeight
      const width=Math.round(capture.width*xScale),height=Math.round(capture.height*yScale)
      const gpu=nativeImage.createFromBitmap(await readFile(temporary),{width:capture.width,height:capture.height,scaleFactor:1}).resize({width,height}).toBitmap()
      const left=Math.round((capture.x||0)*xScale),top=Math.round((capture.y||0)*yScale)
      for(let y=0;y<height;y++){
        const yy=y+top;if(yy<0||yy>=size.height)continue
        const x0=Math.max(0,-left),x1=Math.min(width,size.width-left)
        if(x1>x0)gpu.copy(buffer,(yy*size.width+left+x0)*4,(y*width+x0)*4,(y*width+x1)*4)
      }
    }finally{await unlink(temporary).catch(()=>{})}
  }
  await mkdir(dirname(resolve(path)),{recursive:true})
  await writeFile(resolve(path),nativeImage.createFromBitmap(buffer,{width:size.width,height:size.height,scaleFactor:1}).toPNG())
}
export async function saveDiagnosticReport(native:NativePort,path:string,version:string):Promise<void>{
  const runtime=await native.request('runtime.snapshot'),algorithm=await native.request('algorithm.read'),render=await native.request('render.inspect')
  await mkdir(dirname(resolve(path)),{recursive:true})
  await writeFile(resolve(path),JSON.stringify({version,ui:'Electron/React',renderer:'Direct3D11/Direct2D/DirectWrite',runtime,algorithm,render},null,2))
}
