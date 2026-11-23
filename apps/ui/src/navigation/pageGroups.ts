import {definePageGroup,type PageGroup} from "~/framework/pages/model";

export const visualsPages = definePageGroup({
  id: "visuals",
  route: "/visuals",
  label: "Visuals",
  tabsLabel: "Visuals 子页",
  defaultSection: "esp",
  sections: [
    { id: "general", label: "常规", icon: "settings", placement: "start", context: "visuals-general",view:"general",panelClassName:"visuals-general-panel" },
    { id: "esp", label: "ESP", icon: "esp", view: "esp", context: "esp", panelClassName: "visuals-esp-panel" },
    { id: "radar", label: "Radar", icon: "radar", view: "radar", context: "radar", panelClassName: "visuals-radar-panel" },
  ],
});

/** The page content and its status bar must resolve the same available sections. */
export function availableVisualsPages(esp:boolean,radar:boolean):PageGroup{
  return {...visualsPages,defaultSection:esp?"esp":radar?"radar":"general",
    sections:visualsPages.sections.filter(section=>section.id==="general"||section.id==="esp"&&esp||section.id==="radar"&&radar)};
}

export const aimPages=definePageGroup({
  id:"aim",route:"/aim",label:"Aim",tabsLabel:"Aim 子页",defaultSection:"aimbot",context:"aimbot",
  sections:[
    {id:"general",label:"常规",icon:"settings",placement:"start",view:"general",context:"aim-general",attributes:{"data-aim-section":"general"}},
    {id:"aimbot",label:"Aimbot",icon:"aimbot",view:"aimbot",attributes:{"data-aim-section":"aimbot"}},
    {id:"triggerbot",label:"Triggerbot",icon:"triggerbot",view:"triggerbot",context:"triggerbot",attributes:{"data-aim-section":"triggerbot"}},
    {id:"rage",label:"Rage",icon:"aimbot",view:"rage",context:"rage",attributes:{"data-aim-section":"rage"}},
  ],
});

export function availableAimPages(aimbot:boolean,triggerbot:boolean,rage:boolean):PageGroup{
  const enabled:Record<string,boolean>={general:true,aimbot,triggerbot,rage};
  return {...aimPages,defaultSection:aimbot?"aimbot":triggerbot?"triggerbot":rage?"rage":"general",
    sections:aimPages.sections.filter(section=>enabled[section.id])};
}

export const readerPages=definePageGroup({
  id:"reader",route:"/",label:"READER",tabsLabel:"READER 子页",defaultSection:"connection",context:"reader",
  sections:[
    {id:"connection",label:"连接",icon:"connection",view:"connection",attributes:{"data-reader-section":"connection"}},
    {id:"dashboard",label:"数据看板",icon:"dashboard",view:"dashboard",context:"dashboard",queryKeys:["view"],panelClassName:"data-dashboard-panel",attributes:{"data-reader-section":"dashboard"}},
  ],
});
export const mousePages=definePageGroup({
  id:"mouse",route:"/mouse-debug",label:"鼠标移动调试",tabsLabel:"鼠标移动调试子页",defaultSection:"learning",context:"learning",
  sections:[
    {id:"learning",label:"速度学习",icon:"speedLearning",view:"learning"},
    {id:"characteristics",label:"鼠标特征",icon:"mouseDevice",view:"characteristics",context:"reports"},
  ],
});
export const recoilPages=definePageGroup({
  id:"recoil",route:"/recoil",label:"Recoil",tabsLabel:"Recoil子页",defaultSection:"no-recoil",context:"recoil",
  sections:[{id:"no-recoil",label:"No Recoil"}],
});
