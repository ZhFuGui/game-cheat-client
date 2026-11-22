import {writeFile} from "node:fs/promises";
import {dirname, join} from "node:path";
import type {SmokeContext} from "./context";
import {navigate, requireHidden, waitDom} from "./dom";

const pages = [
  {section: "general", selector: ".aim-general-page", pairs: [
    ["敌人", "队友"],
    ["检测游戏窗口位于前台", "Hitbox 内缩比例"],
    ["移动轨迹", "速度变化控制"],
    ["控制采样年龄上限", "预计执行延迟"],
  ]},
  {section: "aimbot", selector: ".aimbot-page", pairs: [
    ["手枪", "步枪"],
    ["冲锋枪", "狙击枪"],
    ["目标优先级", "触发侧键"],
  ]},
  {section: "triggerbot", selector: ".triggerbot-page", pairs: [
    ["连续点击最小间隔", "左键按下时长"],
  ]},
  {section: "rage", selector: ".rage-page", pairs: [
    ["连续点击最小间隔", "左键按下时长"],
    ["掩体判断", "其他人物遮挡判断"],
    ["低弹量扣分", "换弹扣分"],
  ]},
] as const;

/** Check the rendered field geometry, including reflow within the same window. */
export async function verifyAimLayout({window, args, report}: Pick<SmokeContext, "window" | "args" | "report">) {
  requireHidden(window);
  const previews: string[] = [];
  const measurements = [];
  for (const {section, selector, pairs} of pages) {
    await navigate(window, "/aim?section=" + section, selector);
    const stack = "document.querySelector(" + JSON.stringify(selector + " .aim-section-stack") + ")";
    await waitDom(window, stack + "?.textContent.includes(" + JSON.stringify(pairs.at(-1)![1]) + ")", "Aim fields did not settle: " + section);
    const layout = await window.webContents.executeJavaScript(`(() => {
      const stack = ${stack};
      const labels = [...stack.querySelectorAll('label, .field-label > span[id]')];
      const field = name => labels.find(label => label.textContent === name)?.closest('.field, .toggle-row');
      const pairs = ${JSON.stringify(pairs)}.map(pair => pair.map(field));
      if (!pairs.flat().every(Boolean)) throw new Error('Missing fields for the compact layout check');
      const read = () => {
        const visible = element => element.getClientRects().length > 0;
        const groups = [...stack.children].filter(visible).map(element => element.getBoundingClientRect());
        const fields = [...stack.querySelectorAll('.field')].filter(visible);
        const inlineLabels = fields.every(element => {
          const label = element.querySelector(':scope > .field-label')?.getBoundingClientRect();
          const control = element.querySelector(':scope > :is(.ui-select, .input-wrap, .setting-random-range)')?.getBoundingClientRect();
          return label && control && control.width > 0 && label.right <= control.left + 1 &&
            label.bottom > control.top && label.top < control.bottom;
        });
        return {
          width: Math.round(stack.getBoundingClientRect().width),
          height: Math.round(stack.getBoundingClientRect().height),
          inlineLabels,
          noOverflow: [stack, ...stack.querySelectorAll('.aim-fields, .field')].filter(visible)
            .every(element => element.scrollWidth <= element.clientWidth + 1),
          stackedGroups: groups.every((group, index) => index === 0 || group.top >= groups[index - 1].bottom),
          pairs: pairs.map(([a, b]) => {
            const first = a.getBoundingClientRect(), second = b.getBoundingClientRect();
            if (first.right <= second.left && Math.abs(first.top - second.top) < 1) return 'beside';
            if (second.top >= first.bottom && Math.abs(first.left - second.left) < 1) return 'below';
            return 'misaligned';
          }),
        };
      };
      const wide = read();
      const previous = stack.style.maxWidth;
      try {
        stack.style.maxWidth = '480px';
        return {wide, narrow: read()};
      } finally {
        stack.style.maxWidth = previous;
      }
    })()`) as {
      wide: {inlineLabels: boolean; noOverflow: boolean; stackedGroups: boolean; pairs: string[]};
      narrow: {inlineLabels: boolean; noOverflow: boolean; stackedGroups: boolean; pairs: string[]};
    };
    if (![layout.wide, layout.narrow].every(view => view.inlineLabels && view.noOverflow && view.stackedGroups) ||
        !layout.wide.pairs.every(position => position === "beside") ||
        !layout.narrow.pairs.every(position => position === "below"))
      throw new Error("Compact Aim layout failed for " + section + " " + JSON.stringify(layout));
    measurements.push({section, ...layout});
    if (args.includes("--preview-aim-layout")) {
      await window.webContents.executeJavaScript("document.fonts.ready.then(() => document.querySelector('.aim-page').scrollTo(0, 0))");
      await window.webContents.capturePage(undefined, {stayHidden: true, stayAwake: true});
      await window.webContents.executeJavaScript("new Promise(resolve => setTimeout(resolve, 120))");
      const image = await window.webContents.capturePage(undefined, {stayHidden: true, stayAwake: true});
      if (image.isEmpty()) throw new Error("Aim layout preview was empty: " + section);
      const path = join(dirname(report), "aim-layout-" + section + ".png");
      await writeFile(path, image.toPNG());
      previews.push(path);
    }
  }
  requireHidden(window);
  return {measurements, previews};
}
