import { readdir, readFile, mkdir, writeFile } from 'node:fs/promises'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import { boundaryViolations } from './architecture/dependencies.mjs'

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..')
const roots = ['apps/desktop/src', 'apps/ui/src', 'apps/native-host', 'packages/shared/src', 'src',
  ...['apps', 'core', 'protocol', 'profiles', 'platforms', 'targets'].map(part => `mousebox-firmware/${part}`)]
const ignored = new Set(['test', 'tests', 'node_modules', 'dist', 'build', 'build-v2'])
const files = []
async function scan(directory) {
  for (const entry of await readdir(path.join(root, directory), { withFileTypes: true })) {
    const file = `${directory}/${entry.name}`
    if (entry.isDirectory()) { if (!ignored.has(entry.name)) await scan(file) }
    else if (/\.(?:ts|tsx|cpp|hpp|h|ino|css)$/.test(file) && !/\.(?:test|d)\.ts$/.test(file)) files.push(file)
  }
}
for (const directory of roots) await scan(directory)
const violations = [], metrics = []
for (const file of files.sort()) {
  const source = await readFile(path.join(root, file), 'utf8')
  violations.push(...boundaryViolations(file, source))
  metrics.push({ file, lines: source.split(/\r?\n/).length, bytes: Buffer.byteLength(source) })
}
const report = { ok: violations.length === 0, checkedFiles: files.length, violations, files: metrics }
const reportArg = process.argv.indexOf('--report')
if (reportArg >= 0) {
  if (!process.argv[reportArg + 1]) throw new Error('--report requires a path')
  const destination = path.resolve(process.argv[reportArg + 1])
  await mkdir(path.dirname(destination), { recursive: true })
  await writeFile(destination, JSON.stringify(report, null, 2))
}
if (violations.length) { console.error(violations.join('\n')); process.exitCode = 1 }
else console.log(`PASS: ${files.length} source files respect project dependency boundaries`)
