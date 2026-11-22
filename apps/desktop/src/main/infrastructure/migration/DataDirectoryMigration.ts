import { constants } from 'node:fs'
import { copyFile, lstat, mkdir, readFile, readdir, writeFile } from 'node:fs/promises'
import { createHash } from 'node:crypto'
import { dirname, join, resolve } from 'node:path'

export function defaultDataDirectory(local:string):string{return join(local,'Mousebox')}
export function previousDataDirectories(local:string):string[]{
  const packaged=join(local,'Packages','OpenAI.Codex_2p2nqsd0c76g0','LocalCache','Local')
  return [join(local,'FuGui','Mousebox'),join(packaged,'FuGui','Mousebox'),join(packaged,'Mousebox')]
}
const assetDirectories=['assets','human-motion','human-speed','mouse-debug','migration-backup']
type CopiedFile={path:string;sha256:string}
async function exists(path:string):Promise<boolean>{
  try{await lstat(path);return true}catch(error){if((error as NodeJS.ErrnoException).code==='ENOENT')return false;throw error}
}
async function digest(path:string):Promise<string>{return createHash('sha256').update(await readFile(path)).digest('hex')}
async function copyEntry(sourceRoot:string,targetRoot:string,relative:string,copied:CopiedFile[]):Promise<void>{
  const source=join(sourceRoot,relative),target=join(targetRoot,relative)
  if(!await exists(source))return
  const info=await lstat(source)
  if(info.isSymbolicLink())throw new Error(`Cannot migrate linked data: ${source}`)
  if(info.isDirectory()){
    if(await exists(target)){
      const targetInfo=await lstat(target)
      if(!targetInfo.isDirectory()||targetInfo.isSymbolicLink())throw new Error(`Conflicting data directory: ${target}`)
    }
    await mkdir(target,{recursive:true})
    for(const name of (await readdir(source)).sort())await copyEntry(sourceRoot,targetRoot,join(relative,name),copied)
    return
  }
  if(!info.isFile())throw new Error(`Unsupported migration source: ${source}`)
  const sha256=await digest(source)
  if(await exists(target)){
    const targetInfo=await lstat(target)
    if(!targetInfo.isFile()||targetInfo.isSymbolicLink()||await digest(target)!==sha256)
      throw new Error(`Existing data differs; original files retained: ${target}`)
  }else{
    await mkdir(dirname(target),{recursive:true})
    await copyFile(source,target,constants.COPYFILE_EXCL)
    if(await digest(target)!==sha256)throw new Error(`Data migration verification failed: ${target}`)
  }
  copied.push({path:relative,sha256})
}
// Business files only. Browser caches, lockfiles and runtime output are not migrated.
export async function copyLegacyAssets(sourceRoot:string,targetRoot:string):Promise<void>{
  if(resolve(sourceRoot)===resolve(targetRoot))return
  const copied:CopiedFile[]=[]
  for(const relative of assetDirectories)await copyEntry(sourceRoot,targetRoot,relative,copied)
}
export async function migratePreviousDataDirectory(local:string,targetRoot:string):Promise<string|undefined>{
  if(await exists(join(targetRoot,'workspace-index.json'))||await exists(join(targetRoot,'settings.json')))return
  for(const sourceRoot of previousDataDirectories(local)){
    if(resolve(sourceRoot)===resolve(targetRoot)||!await exists(join(sourceRoot,'settings.json')))continue
    const copied:CopiedFile[]=[]
    for(const relative of ['settings.ini','web-ui.json',...assetDirectories])await copyEntry(sourceRoot,targetRoot,relative,copied)
    // Commit settings last, so an interrupted import cannot appear complete.
    await mkdir(targetRoot,{recursive:true})
    await writeFile(join(targetRoot,'data-directory-migration.json'),JSON.stringify({version:1,source:sourceRoot,files:copied},null,2),'utf8')
    await copyEntry(sourceRoot,targetRoot,'settings.json',copied)
    return sourceRoot
  }
}
