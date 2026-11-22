import {z} from "zod";

export const deviceMouseBindings = [
  {value:"mouse:left",label:"鼠标左键"}, {value:"mouse:right",label:"鼠标右键"},
  {value:"mouse:middle",label:"鼠标中键"}, {value:"mouse:back",label:"后退侧键"},
  {value:"mouse:forward",label:"前进侧键"},
] as const;
export const localKeyboardCodes = [
  ..."ABCDEFGHIJKLMNOPQRSTUVWXYZ".split("").map(key=>`Key${key}`),
  ..."0123456789".split("").map(key=>`Digit${key}`),
  "F1","F2","F3","F4","F5","F6","F7","F9","F10","F11","F12",
  "Space","Tab","Enter","ShiftLeft","ShiftRight","ControlLeft","ControlRight","AltLeft","AltRight",
  "ArrowUp","ArrowDown","ArrowLeft","ArrowRight",
] as const;
export const activationBindingSchema = z.string().refine(value=>
  deviceMouseBindings.some(b=>b.value===value)||value.startsWith("key:")&&localKeyboardCodes.includes(value.slice(4)),
  "Unsupported activation binding");
export function localBindingLabel(binding:string):string {
  const code=binding.slice(4);
  return code.startsWith("Key")?code.slice(3):code.startsWith("Digit")?code.slice(5):code;
}
