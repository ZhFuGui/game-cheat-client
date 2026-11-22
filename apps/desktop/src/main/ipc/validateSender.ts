import { pathToFileURL } from 'node:url'
export function isTrustedUrl(actual:string, uiPath:string,devOrigin?:string):boolean {
  try {
    const url=new URL(actual)
    if(devOrigin){const allowed=new URL(devOrigin);return url.origin===allowed.origin&&['http:','https:'].includes(url.protocol)}
    const expected=new URL(pathToFileURL(uiPath).toString())
    return url.protocol==='file:'&&url.pathname===expected.pathname&&url.host===expected.host
  }catch{return false}
}
