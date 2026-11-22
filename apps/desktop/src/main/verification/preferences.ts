import { writeFile } from 'node:fs/promises';
import { dirname, join } from 'node:path';
import { bootstrapSchema } from '@mousebox/shared';
import type { SmokeContext } from './context';
import { requireHidden, waitDom } from './dom';

/** Exercises only the isolated renderer and its own preferences; no device output. */
export async function verifyPreferences({window,container,args,report}:SmokeContext) {
    const original=bootstrapSchema.parse(await container.invoke('bootstrap',{})).preferences;
    const previews:string[]=[];
    await window.webContents.executeJavaScript("document.querySelector('[data-section=general]').click()");
    await waitDom(window,"document.querySelectorAll('.ui-choice-buttons').length===2",'Preference buttons did not mount');
    const layout=await window.webContents.executeJavaScript(`(()=>{
      const groups=[...document.querySelectorAll('.ui-choice-buttons')];
      return {groups:groups.length,choices:groups.map(group=>group.querySelectorAll('[role=radio]').length),
        buttonsOnly:groups.every(group=>[...group.querySelectorAll('[role=radio]')].every(choice=>choice.tagName==='BUTTON')),
        selected:groups.map(group=>group.querySelectorAll('[aria-checked=true]').length),
        dropdowns:document.querySelectorAll('.settings-content .ui-select').length};
    })()`) as {groups:number;choices:number[];buttonsOnly:boolean;selected:number[];dropdowns:number};
    if(!layout.buttonsOnly||layout.groups!==2||layout.dropdowns||layout.choices.some(n=>n!==2)||layout.selected.some(n=>n!==1))
        throw new Error('Preference button layout failed: '+JSON.stringify(layout));
    // A hidden Chromium window can leave CSS transitions running at time zero.
    // Inspect the settled appearance without changing normal application motion.
    const motionId='preference-preview-motion';
    await window.webContents.executeJavaScript(`document.head.appendChild(Object.assign(document.createElement('style'),{id:${JSON.stringify(motionId)},textContent:'.ui-choice-button{transition:none!important}'}))`);
    const appearances:unknown[]=[];
    const capture=async(name:string)=>{
        if(!args.includes('--settings-previews'))return;
        requireHidden(window);
        await window.webContents.executeJavaScript('new Promise(resolve=>setTimeout(resolve,100))');
        await window.webContents.capturePage(undefined,{stayHidden:true,stayAwake:true});
        const image=await window.webContents.capturePage(undefined,{stayHidden:true,stayAwake:true});
        if(image.isEmpty())throw new Error('Own settings preview was empty');
        const file=join(dirname(report),name+'.png');await writeFile(file,image.toPNG());previews.push(file);
        requireHidden(window);
    };
    try {
        for(const mode of ['light','dark'] as const) {
            await window.webContents.executeJavaScript(`document.querySelectorAll('.ui-choice-buttons')[0].querySelectorAll('[role=radio]')[${mode==='dark'?1:0}].click()`);
            await waitDom(window,`document.documentElement.classList.contains('dark')===${mode==='dark'}&&!document.querySelector('.ui-choice-button[data-disabled]')&&document.querySelectorAll('.ui-choice-buttons')[0].querySelectorAll('[role=radio]')[${mode==='dark'?1:0}].getAttribute('aria-checked')==='true'`,'Theme button did not settle');
            if(bootstrapSchema.parse(await container.invoke('bootstrap',{})).preferences.colorMode!==mode)
                throw new Error('Theme button did not save its explicit value');
            const appearance=await window.webContents.executeJavaScript(`(()=>{
                const group=document.querySelector('.ui-choice-buttons'),selected=group.querySelector('[aria-checked=true]');
                return {mode:${JSON.stringify(mode)},selected: selected.textContent,selectedCount:group.querySelectorAll('[aria-checked=true]').length,
                    background:getComputedStyle(selected).backgroundColor,card:getComputedStyle(document.querySelector('.settings-content .section')).backgroundColor,
                    unselected:[...group.querySelectorAll('[aria-checked=false]')].map(button=>getComputedStyle(button).backgroundColor)};
            })()`) as {selectedCount:number;background:string;card:string;unselected:string[]};
            if(appearance.selectedCount!==1||appearance.background!==appearance.card||appearance.unselected.some(color=>color!=='rgba(0, 0, 0, 0)'))
                throw new Error('Theme selection appearance failed: '+JSON.stringify(appearance));
            appearances.push(appearance);
            await capture('general-'+mode);
        }
        await window.webContents.executeJavaScript("document.querySelectorAll('.ui-choice-buttons')[1].querySelectorAll('[role=radio]')[1].click()");
        await waitDom(window,"document.documentElement.lang==='en'&&!document.querySelector('.ui-choice-button[data-disabled]')&&document.querySelectorAll('.ui-choice-buttons')[1].querySelectorAll('[role=radio]')[1].getAttribute('aria-checked')==='true'",'Language button did not settle');
        if(bootstrapSchema.parse(await container.invoke('bootstrap',{})).preferences.language!=='en')
            throw new Error('Language button did not save');
    } finally {
        try {
            await container.invoke('preferences.set',{colorMode:original.colorMode,language:original.language});
            await waitDom(window,`document.documentElement.lang===${JSON.stringify(original.language==='zh'?'zh-CN':'en')}&&document.documentElement.classList.contains('dark')===${original.colorMode==='dark'}`,'Original preferences did not restore');
        } finally {
            await window.webContents.executeJavaScript(`document.getElementById(${JSON.stringify(motionId)})?.remove()`);
        }
    }
    await window.webContents.executeJavaScript("document.querySelector('[data-section=firmware]').click()");
    await waitDom(window,"document.querySelector('.settings-content .actions button')",'Firmware actions did not mount');
    const firmware=await window.webContents.executeJavaScript(`(()=>{
      const content=document.querySelector('.settings-content'),buttons=[...content.querySelectorAll('.actions button')];
      return {sections:content.querySelectorAll('.section').length,detectEnabled:!buttons[0].disabled,actions:buttons.length,
        removedStatus:![...content.querySelectorAll('h2')].some(e=>/设备状态|Device status/.test(e.textContent))};
    })()`) as {sections:number;detectEnabled:boolean;actions:number;removedStatus:boolean};
    if(firmware.sections!==1||!firmware.detectEnabled||firmware.actions!==2||!firmware.removedStatus)
        throw new Error('Firmware settings layout failed: '+JSON.stringify(firmware));
    await window.webContents.executeJavaScript("document.querySelector('.settings-content .actions button').click()");
    await waitDom(window,"!document.querySelector('.settings-content .actions button').disabled",'Manual port discovery did not finish');
    await capture('firmware');
    return {layout,appearances,firmware,previews};
}
