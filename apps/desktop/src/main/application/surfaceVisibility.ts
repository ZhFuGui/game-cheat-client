/** Hidden verification cannot request an overlay; a minimized user window can. */
export function surfaceVisibility(visible:boolean,active:boolean|undefined,windowVisible:boolean,minimized:boolean){
  const requested=(active??visible)&&(windowVisible||minimized);
  return {active:requested,visible:requested&&visible&&windowVisible&&!minimized};
}
