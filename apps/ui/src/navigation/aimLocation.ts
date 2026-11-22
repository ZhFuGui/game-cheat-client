import {pageHref} from "~/framework/pages/model";
import {aimPages} from "./pageGroups";

export function aimHref(section:string|null|undefined):string {
  return pageHref(aimPages,section??"");
}
/** Keep old routes and launch links without restoring separate sidebar pages. */
export function legacyAimHref(path:string,search:string):string {
  const query=new URLSearchParams(search);
  if(path==="/aimbot"&&(query.get("algorithm")==="1"||query.get("tab")==="algorithm"))
    return "/game-settings?section=aimbot-algorithm";
  return aimHref(path.slice(1));
}
