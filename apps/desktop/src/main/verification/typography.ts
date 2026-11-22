import type { SmokeContext } from "./context";
import { requireHidden } from "./dom";

export async function verifyTypography({ window }: Pick<SmokeContext, "window">) {
    requireHidden(window);
    const font = await window.webContents.executeJavaScript(`(async () => {
        const faces = await document.fonts.load('13px "Mousebox UI"', '中文仓耳A0');
        await document.fonts.ready;
        return { loaded: faces.length > 0 && faces.every(face => face.status === 'loaded'),
            family: getComputedStyle(document.body).fontFamily };
    })()`) as { loaded: boolean; family: string };
    if (!font.loaded || !font.family.startsWith('"Mousebox UI"'))
        throw new Error("The bundled primary font did not load in the actual renderer");
    return { ...font, systemInstallationRequired: false };
}
