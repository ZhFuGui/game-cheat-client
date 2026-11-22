import { readFile, mkdir, writeFile } from "node:fs/promises";
import { resolve } from "node:path";
import { NativeClient } from "../../desktop/src/main/infrastructure/native/NativeClient";
import { displayText } from "../src/i18n/displayText";

async function main() {
  const report = JSON.parse(
    await readFile(resolve("out/web-electron-complete/report.json"), "utf8"),
  );
  const directory = resolve("out/web-translation-audit", String(process.pid));
  await mkdir(directory, { recursive: true });
  const native = new NativeClient(
    resolve("out/x64/Debug/mousebox-native.exe"),
    directory,
  );
  let diagnostics: unknown;
  native.start();
  try {
    await native.activate();
    const snapshot = (await native.request("bootstrap")) as {
      state: { readerRunning: boolean; boxReady: boolean };
    };
    if (snapshot.state.readerRunning || snapshot.state.boxReady)
      throw new Error("Audit runtime must be disconnected");
    diagnostics = await native.request("algorithm.read");
  } finally {
    await native.stop();
  }
  const strings: { path: string; source: string; translated: string }[] = [];
  function visit(value: unknown, path: string) {
    if (typeof value === "string" && /[\u4e00-\u9fff]/.test(value)) {
      const translated = displayText(value, "en");
      strings.push({ path, source: value, translated });
    } else if (Array.isArray(value))
      value.forEach((v, i) => visit(v, `${path}[${i}]`));
    else if (value && typeof value === "object")
      Object.entries(value).forEach(([k, v]) => visit(v, `${path}.${k}`));
  }
  visit(report.bootstrap, "bootstrap");
  visit(diagnostics, "diagnostics");
  const missing = strings.filter((v) => /[\u4e00-\u9fff]/.test(v.translated));
  await writeFile(
    resolve(directory, "audit.json"),
    JSON.stringify({ checked: strings.length, missing, diagnostics }, null, 2),
  );
  process.stdout.write(
    JSON.stringify({ checked: strings.length, missing }, null, 2) + "\n",
  );
}
void main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
