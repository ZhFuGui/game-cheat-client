import {pageHref,resolvePageGroup} from "~/framework/pages/model";
import {readerPages} from "./pageGroups";

/** Translate old nested links into one primary section plus an optional content mode. */
export function readerHref(raw:string,view?:string|null):string{
  const base=pageHref(readerPages,raw);
  const mode=view??raw.split("/")[1];
  return resolvePageGroup(readerPages,raw).section.id==="dashboard"&&mode==="source"?`${base}&view=source`:base;
}
