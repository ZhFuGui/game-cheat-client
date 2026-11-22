import type {NativeSnapshot} from '@mousebox/shared'
export type AlgorithmScope='aimbot'|'mouse'
const movementIds=new Set(['mapping','input','speed','trajectory','feedforward','learning','variation','feedback','shot_effects','settling','output','breath_reference'])
export function algorithmSectionsForScope(sections:NativeSnapshot['algorithmSections'],scope:AlgorithmScope){
  return sections.map(section=>({...section,ids:section.scope ? section.scope===scope ? section.ids : [] : section.ids.filter(id=>movementIds.has(id)===(scope==='mouse'))})).filter(section=>section.ids.length)
}
