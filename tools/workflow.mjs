import { mkdir, open, readFile, unlink, writeFile } from 'node:fs/promises'
import { spawn } from 'node:child_process'
import { fileURLToPath } from 'node:url'
import path from 'node:path'
import { parseArgs } from 'node:util'
import { Cache, changedFiles, digest, files, snapshot } from './workflow/cache.mjs'
import { selectChecks } from './workflow/selection.mjs'

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..')
const { values, positionals } = parseArgs({ allowPositionals: true, options: {
  scope: { type: 'string', default: 'all' }, changed: { type: 'boolean' }, dir: { type: 'boolean' },
  force: { type: 'boolean' }, configuration: { type: 'string', default: 'Release' },
} })
const command = positionals[0] ?? 'build'
if (!['build', 'check', 'package', 'plan'].includes(command) || positionals.length > 1 ||
    !['all', 'native', 'web', 'ui'].includes(values.scope) || !['Debug', 'Release'].includes(values.configuration))
  throw new Error('Usage: pnpm workflow build|check|package|plan [--scope all|native|web|ui] [--changed] [--dir] [--force]')
const cache = new Cache(root, path.join(root, 'out/workflow/state.json'))
const globalInputs = ['pnpm-lock.yaml', 'pnpm-workspace.yaml', 'package.json', 'tsconfig.base.json', 'tools/workflow.mjs', 'tools/workflow']
const shared = ['packages/shared/src', 'packages/shared/package.json', 'packages/shared/tsconfig.json']
const desktopResources = ['apps/desktop/resources']
const fontResources = ['data/fonts']
const nativeInputs = ['src', 'apps/native-host', 'projects', 'build', 'protocol', 'tests/native',
  'third_party/reader-consumer', 'third_party/cs2-reader', 'third_party/traceline/include', 'mousebox-firmware/protocol', 'game-cheat-client.sln', 'game-cheat-client.local.sln', ...fontResources]
const sourceRoots = [...globalInputs, ...shared, ...nativeInputs, 'apps/ui/src', 'apps/desktop/src', 'tests', 'tools',
  'apps/ui', 'apps/desktop', 'package.json', 'eslint.config.js', 'vitest.config.ts',
  'apps/ui/package.json', 'apps/desktop/package.json', 'apps/ui/tsconfig.json', 'apps/desktop/tsconfig.json', 'data/games/pubg/weapons/2026-10-15/profiles.json', 'data/games/cs2/hitboxes']
const cacheInput = entries => snapshot(root, [...globalInputs, ...entries])
const sourceAssetStats = () => snapshot(root, ['data/games/cs2/cover'], { metadata: true })
const built = new Map()
function run(executable, args, environment = {}) {
  return new Promise((resolve, reject) => {
    const child = spawn(executable, args, { cwd: root, env: { ...process.env, ...environment }, stdio: 'inherit', windowsHide: true })
    child.once('error', reject)
    child.once('exit', code => code === 0 ? resolve() : reject(new Error(`${path.basename(executable)} exited with ${code}`)))
  })
}
const powershell = (script, args = []) => run('powershell.exe', ['-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', path.join(root, 'tools', script), ...args])
function pnpm(args, env) {
  if (!process.env.npm_execpath) throw new Error('Run this workflow through pnpm (pnpm workflow ...).')
  return run(process.execPath, [process.env.npm_execpath, ...args], env)
}
async function nativeBuild(configuration, tests = false) {
  // MSBuild owns header/option/toolchain tracking. Do not replace it with a source-only cache.
  if (built.get(configuration) === true || (built.has(configuration) && !tests)) return
  await powershell('build.ps1', ['-Configuration', configuration, ...(tests ? ['-IncludeLocalTools'] : [])])
  built.set(configuration, tests)
}
async function webTypes(uiOnly = false) {
  const config = ['apps/ui/vite.config.ts', 'apps/desktop/electron.vite.config.ts']
  const ui = [...shared, ...fontResources, 'data/branding', 'apps/ui/src', 'apps/ui/public', 'apps/ui/package.json', 'apps/ui/tsconfig.json', 'apps/ui/index.html', ...config]
  const desktop = [...shared, 'apps/desktop/src', 'apps/desktop/package.json', 'apps/desktop/tsconfig.json', ...config]
  await cache.step('types:shared', await cacheInput(shared), [], () => pnpm(['--filter', '@mousebox/shared', 'typecheck']), { force: values.force })
  const results = await Promise.allSettled([
    cache.step('types:ui', await cacheInput(ui), [], () => pnpm(['--filter', '@mousebox/ui', 'typecheck']), { force: values.force }),
    ...(uiOnly ? [] : [cache.step('types:desktop', await cacheInput(desktop), [], () => pnpm(['--filter', '@mousebox/desktop', 'typecheck']), { force: values.force })]),
  ])
  for (const result of results) if (result.status === 'rejected') throw result.reason
  return { ui, desktop }
}
async function webBuild(uiOnly = false) {
  await cache.step('resources:valorant-radar',
    await cacheInput(['apps/ui/public/radar/valorant', 'apps/ui/src/games/valorant/radar/manifest.json', 'tools/check-valorant-radar-assets.mjs']), [],
    () => run(process.execPath, ['tools/check-valorant-radar-assets.mjs']), { force: values.force })
  await cache.step('resources:fonts', await cacheInput(fontResources), [],
    () => powershell('check-font-resources.ps1'), { force: values.force })
  const { ui, desktop } = await webTypes(uiOnly)
  const bundleInput = await snapshot(root, [...globalInputs, ...ui], { filter: file => !/\/test\/|\.test\./.test(file) })
  await cache.step('build:ui', bundleInput, ['apps/ui/dist'], () => pnpm(['--filter', '@mousebox/ui', 'exec', 'vite', 'build']), { force: values.force })
  if (!uiOnly) await cache.step('build:shell', await cacheInput(desktop), ['apps/desktop/dist'],
    () => pnpm(['--filter', '@mousebox/desktop', 'exec', 'electron-vite', 'build', '--ignoreConfigWarning'], { MOUSEBOX_ELECTRON_BUILD_SCOPE: 'shell' }), { force: values.force })
}
async function architecture() {
  await cache.step('architecture', await cacheInput(sourceRoots), [], async () => {
    await pnpm(['test:architecture']); await pnpm(['check:architecture'])
  }, { force: values.force })
}
async function toolTests() {
  const inputs = ['tools', 'tests/workflow', 'tests/assets', 'data/games/cs2/hitboxes',
    'src/games/cs2/hitboxes/generated', 'projects', 'build', 'package.json']
  await cache.step('tests:workflow', await cacheInput(inputs), [], async () => {
    const cases = (await files(root, ['tests/workflow'])).filter(file => file.endsWith('.test.mjs'))
    if (cases.length) await run(process.execPath, ['--test', ...cases])
    const assets = (await files(root, ['tests/assets'])).filter(file => /test_.*\.py$/.test(file))
    if (assets.length) await run('python', ['-m', 'unittest', 'discover', '-s', 'tests/assets', '-p', 'test_*.py'])
  }, { force: values.force })
}
async function checks(changedOnly) {
  const current = await snapshot(root, sourceRoots)
  const assets = await sourceAssetStats()
  const changes = changedFiles(cache.data.checked ?? {}, current)
  if (cache.data.assetStats !== digest(assets)) changes.push('data/games/cs2/cover/')
  const plan = selectChecks(changes, !changedOnly || !cache.data.checked)
  console.log(JSON.stringify({ checks: changedOnly ? 'changed' : 'complete', changedFiles: changes.length, plan }))
  await architecture()
  if (plan.tools) await toolTests()
  for (const configuration of (plan.full ? ['Debug', 'Release'] : [values.configuration])) {
    if (!plan.suites.length) continue
    await nativeBuild(configuration, true)
    const binary = `out/x64/${configuration}/mousebox-tests.exe`
    const nativeCases = (await files(root, ['tests'])).filter(file => /\.(?:cpp|hpp|h)$/.test(file) || file.startsWith('tests/fixtures/'))
    for (const suite of plan.suites) await cache.step(`native:${configuration}:${suite}`,
      await cacheInput([...nativeInputs, ...nativeCases, binary, 'tools/test.ps1']), [],
      () => run(path.join(root, binary), ['--suite', suite]), { force: values.force })
  }
  if (plan.web === 'all' || plan.web.length) {
    if (plan.interop) await nativeBuild('Debug')
    await webTypes()
    const webInputs = [...shared, 'apps/ui/src', 'apps/desktop/src', 'tests/desktop', 'tests/ui', 'tests/interop', 'vitest.config.ts', 'eslint.config.js',
      'apps/ui/package.json', 'apps/desktop/package.json', 'out/x64/Debug/mousebox-native.exe']
    await cache.step(plan.web === 'all' ? 'web:all' : 'web:related',
      { inputs: await cacheInput(webInputs), selection: plan.web }, [], async () => {
      await pnpm(['lint'])
      // The native Debug build has completed before real stdio/TCP bridge tests start.
      const related = plan.web !== 'all' && plan.web.length < 60 && plan.web.every(file => /\.[cm]?[jt]sx?$/.test(file))
      await pnpm(['exec', 'vitest', ...(related ? ['related', '--run', '--passWithNoTests', ...plan.web] : ['run'])])
    }, { force: values.force })
  }
  if (plan.interop && plan.web !== 'all' && !plan.web.length) {
    await nativeBuild('Debug')
    await cache.step('web:interop', await cacheInput(['tests/interop', ...shared, 'out/x64/Debug/mousebox-native.exe']), [],
      () => pnpm(['exec', 'vitest', 'run', 'tests/interop']), { force: values.force })
  }
  if (plan.assets || values.force) {
    await nativeBuild('Release', true)
    await cache.step('assets:compatibility', { assets, input: await cacheInput(['src/adapters/spatial/traceline', 'tests/native/traceline_adapter_tests.cpp', 'out/x64/Release/mousebox-tests.exe']) },
      ['out/workflow/scenes.json'], () => run(path.join(root, 'out/x64/Release/mousebox-tests.exe'),
        ['--suite', 'traceline', '--resources', path.join(root, 'data/games/cs2/cover'), '--output', path.join(root, 'out/workflow/scenes.json')]), { force: values.force })
  }
  cache.data.checked = current; cache.data.assetStats = digest(assets); await cache.save()
}
async function build() {
  if (values.scope === 'all' || values.scope === 'native') {
    await nativeBuild(values.configuration)
    if (values.configuration === 'Release') await powershell('prepare-desktop-resources.ps1')
  }
  if (values.scope !== 'native') {
    if (values.scope !== 'ui') await cache.step('resources:icon',
      await cacheInput(['data/branding/application-icon.png', 'tools/prepare-desktop-icon.ps1']),
      ['apps/desktop/resources/icon.ico'], () => powershell('prepare-desktop-icon.ps1'), { force: values.force })
    await webBuild(values.scope === 'ui')
  }
}
async function pack() {
  if (values.configuration !== 'Release' || values.scope !== 'all') throw new Error('Packaging requires Release and scope=all')
  await checks(Boolean(values.dir))
  await build()
  const version = JSON.parse(await readFile(path.join(root, 'apps/desktop/package.json'), 'utf8')).version
  const output = `out/packages/${version}/${values.dir ? 'preview' : 'release'}`
  const input = { code: await cacheInput(['apps/desktop/dist', 'apps/ui/dist', 'out/desktop-native', ...desktopResources, 'apps/desktop/package.json', 'apps/desktop/electron-builder.yml', 'tools/after-pack.cjs', 'data/games/pubg/weapons/2026-10-15/profiles.json']), assets: await sourceAssetStats() }
  // Keep individual input snapshots intact so edits during packing invalidate the receipt.
  input.ui = await outputSnapshot('apps/ui/dist'); input.shell = await outputSnapshot('apps/desktop/dist')
  await cache.step(`package:${values.dir ? 'preview' : 'release'}`, input, [output], async () => {
    if (!values.dir) await powershell('check-traceline-package.ps1', ['-IncludeScenes'])
    const config = path.join(root, 'out/workflow/builder.json')
    await writeFile(config, JSON.stringify({ extends: path.join(root, 'apps/desktop/electron-builder.yml'), directories: { output: path.join(root, output) } }))
    await pnpm(['--filter', '@mousebox/desktop', 'exec', 'electron-builder', ...(values.dir ? ['--dir'] : ['--win']), '--publish', 'never', '--config', config])
  }, { force: values.force, metadata: true })
}
// Scan output directories explicitly, bypassing only the source traversal's dist exclusion.
async function outputSnapshot(directory) { return snapshot(path.join(root, directory), ['.'], { generated: true }) }
await mkdir(path.join(root, 'out/workflow'), { recursive: true })
const lock = path.join(root, 'out/workflow/active.lock')
let handle
try { handle = await open(lock, 'wx'); await handle.writeFile(JSON.stringify({ pid: process.pid })) }
catch (error) { throw new Error(`Another workflow is active, or its lock remains: ${lock}`, { cause: error }) }
const began = performance.now()
let successful = false
try {
  await cache.load()
  if (command === 'build') await build()
  else if (command === 'check') await checks(Boolean(values.changed))
  else if (command === 'package') await pack()
  else {
    const current = await snapshot(root, sourceRoots)
    console.log(JSON.stringify(selectChecks(changedFiles(cache.data.checked ?? {}, current), !cache.data.checked), null, 2))
  }
  successful = true
} finally {
  await writeFile(path.join(root, 'out/workflow/last-run.json'), JSON.stringify({ command, ok: successful, ms: Math.round(performance.now() - began), steps: cache.report }, null, 2))
  await handle.close(); await unlink(lock)
}
