import path from 'node:path'
import { builtinModules } from 'node:module'
import ts from 'typescript'

const builtins = new Set(builtinModules.map(name => name.replace(/^node:/, '')))

const nativeRules = {
  domain: ['domain'], protocol: ['protocol'], platform: ['platform'],
  control: ['control', 'domain', 'protocol'],
  render: ['render', 'domain', 'control'],
  application: ['application', 'domain', 'control', 'platform', 'protocol', 'render'],
  games: ['games', 'application', 'adapters', 'domain', 'control', 'platform', 'protocol', 'render'],
  adapters: ['adapters', 'application', 'domain', 'control', 'platform', 'protocol'],
}
const slash = value => value.replaceAll('\\', '/')

// Read real import syntax, including multiline/type-only declarations, re-exports
// and lazy imports. Comments and example strings are not dependencies.
function scriptImports(filename, source) {
  const result = []
  const ast = ts.createSourceFile(filename, source, ts.ScriptTarget.Latest, true,
    filename.endsWith('.tsx') ? ts.ScriptKind.TSX : ts.ScriptKind.TS)
  const visit = node => {
    if ((ts.isImportDeclaration(node) || ts.isExportDeclaration(node)) && node.moduleSpecifier && ts.isStringLiteral(node.moduleSpecifier))
      result.push(node.moduleSpecifier.text)
    if (ts.isCallExpression(node) && (node.expression.kind === ts.SyntaxKind.ImportKeyword || ts.isIdentifier(node.expression) && node.expression.text === 'require') && node.arguments.length && ts.isStringLiteral(node.arguments[0]))
      result.push(node.arguments[0].text)
    ts.forEachChild(node, visit)
  }
  visit(ast)
  return result
}
function importTarget(file, spec) {
  if (spec.startsWith('.')) return path.posix.normalize(path.posix.join(path.posix.dirname(file), spec))
  if (spec.startsWith('~/') && file.startsWith('apps/ui/src/')) return `apps/ui/src/${spec.slice(2)}`
  return spec
}

// A source-level boundary check complements the compiler/type checker. It does
// not attempt to interpret generated code, conditional compilation or JS strings.
export function boundaryViolations(filename, source) {
  const file = slash(filename), errors = []
  const fail = message => errors.push(`${file}: ${message}`)
  if (/\.(tsx?|mjs)$/.test(file)) {
    const imports = scriptImports(file, source)
    for (const spec of imports) {
      const target = importTarget(file, spec)
      const system = spec.startsWith('node:') || builtins.has(spec) || spec === 'electron'
      if (file.startsWith('apps/ui/src/') && (system || /^(?:apps\/(?:desktop|native-host)\/|@mousebox\/(?!shared(?:$|\/)))/.test(target))) fail(`Web interface cannot import ${spec}`)
      if (file.startsWith('apps/ui/src/') && !file.startsWith('apps/ui/src/components/icons/') &&
          /^(?:lucide-react(?:$|\/)|@mingcute\/)/.test(spec)) fail(`Use semantic AppIcon instead of importing ${spec}`)
      if (file.startsWith('packages/shared/src/') && (system || /^(?:react$|zustand$|apps\/)/.test(target) || target.startsWith('~/'))) fail(`Shared contracts cannot depend on ${spec}`)
      if (file.startsWith('apps/desktop/src/') && /^(?:apps\/ui\/|~\/)/.test(target)) fail(`Desktop shell cannot import renderer implementation ${spec}`)
      if (file.startsWith('apps/ui/src/components/') && /^apps\/ui\/src\/(?:games|pages|features)\//.test(target)) fail(`Reusable UI components cannot import game/page/feature implementation ${spec}`)
      if (file.startsWith('apps/ui/src/framework/') && /^apps\/ui\/src\/(?:games|pages|features|runtime|shell|workspaces)\//.test(target)) fail(`UI page framework cannot import business implementation ${spec}`)
      if (file.startsWith('apps/ui/src/framework/tabs/') && !target.startsWith('apps/ui/src/framework/tabs/'))
        fail(`Pure tab policies cannot import rendering, storage or business implementation ${spec}`)
      if (file === 'apps/ui/src/navigation/pageGroups.ts' && /^apps\/ui\/src\/(?:games|pages|features|runtime|shell|workspaces)\//.test(target)) fail(`Page descriptions must remain independent of view implementation ${spec}`)
      const ownUiGame = /^apps\/ui\/src\/games\/([^/]+)\//.exec(file)?.[1]
      const targetUiGame = /^apps\/ui\/src\/games\/([^/]+)\//.exec(target)?.[1]
      if (ownUiGame && targetUiGame && ownUiGame !== targetUiGame) fail(`Game UI ${ownUiGame} cannot import game ${targetUiGame}: ${spec}`)
      if (file.startsWith('apps/ui/src/runtime/') && target.startsWith('apps/ui/src/games/')) fail(`Runtime event ownership cannot depend on game UI ${spec}`)
      if (file.startsWith('apps/ui/src/features/extensions/') && /^apps\/ui\/src\/games\/[^/]+\//.test(target)) fail(`Extension slots cannot import a concrete game panel: ${spec}`)
      if (file.startsWith('apps/desktop/src/main/infrastructure/transport/') &&
          spec !== 'node:stream' && !target.startsWith('apps/desktop/src/main/infrastructure/transport/'))
        fail(`Framed transport cannot depend on platform shell or application contracts ${spec}`)
      if (file === 'apps/desktop/src/main/infrastructure/native/NativeSession.ts' &&
          (system || target.startsWith('apps/desktop/src/main/infrastructure/transport/')))
        fail(`Native message session must use an injected send function ${spec}`)
      if (file === 'apps/ui/src/runtime/events.ts' && target.startsWith('apps/ui/src/lib/desktopApi'))
        fail(`Runtime event handlers must receive events independently of the desktop bridge ${spec}`)
      if (file.startsWith('apps/desktop/src/main/domain/') && (system || /^apps\/desktop\/src\/main\/(?:application|infrastructure|ipc|window)\//.test(target))) fail(`Desktop domain cannot depend on platform/application implementation ${spec}`)
      if (/^apps\/desktop\/src\/main\/application\/(?:commandRouter|desktopSnapshots|workspaceSwitchCoordinator)\.ts$/.test(file) && (system || target.startsWith('apps/desktop/src/main/infrastructure/'))) fail(`Desktop business orchestration must use ports instead of ${spec}`)
    }
  }
  if (/\.(?:cpp|hpp|h)$/.test(file) && file.startsWith('src/')) {
    const layer = file.split('/')[1], allowed = nativeRules[layer]
    for (const match of source.matchAll(/^\s*#\s*include\s*["<]([^">]+)[">]/gm)) {
      const spec = slash(match[1])
      const local = spec.startsWith('.') || !spec.includes('/') ? path.posix.normalize(path.posix.join(path.posix.dirname(file), spec)) : `src/${spec}`
      const target = local.startsWith('src/') ? local.split('/')[1] : undefined
      if (file.startsWith('src/application/output/') &&
          (['platform', 'render'].includes(target) || /^(?:windows\.h|winsock2\.h)$/i.test(spec)))
        fail(`Output scheduling must use the output link and injected clock/input ${spec}`)
      if (layer === 'render' && (spec === 'domain/source_sample.hpp' || spec.endsWith('font_resource.h')))
        fail(`Renderer must receive font bytes and completed-frame callbacks from its caller ${spec}`)
      if (file.startsWith('src/application/extensions/') && ['games', 'adapters', 'platform', 'render'].includes(target))
        fail(`Extension host must stay independent of concrete games, binary adapters and rendering: ${spec}`)
      if (/^src\/games\/[^/]+\/extensions\//.test(file) && ['adapters', 'platform'].includes(target))
        fail(`Game extension processing must use injected ports: ${spec}`)
      if (spec.startsWith('cs2-external-autowall/') &&
          (!file.startsWith('src/adapters/spatial/traceline/') || !/^cs2-external-autowall\/(?:api|types)\.h$/.test(spec)))
        fail(`Only the traceline adapter may consume its public C API, never internal implementation: ${spec}`)
      const explicitComposition = /^src\/application\/settings\/(?:settings\.(?:cpp|hpp)|game_configurations\.cpp)$/.test(file)
      const sourceModel = (file.startsWith('src/adapters/sources/pubg_reader/')
        && /^games\/pubg\/(?:model\/|module\.hpp$)/.test(spec))
        || (file.startsWith('src/adapters/sources/cs2_reader/') && /^games\/cs2\/model\//.test(spec))
      if (allowed && nativeRules[target] && !allowed.includes(target) && !(explicitComposition && target === 'games') && !sourceModel)
        fail(`${layer} cannot depend on ${target}: ${spec}`)
      const ownGame = /^src\/games\/([^/]+)\//.exec(file)?.[1]
      const importedGame = /^src\/games\/([^/]+)\//.exec(local)?.[1]
      if (ownGame && importedGame && ownGame !== importedGame) fail(`Game ${ownGame} cannot import game ${importedGame}: ${spec}`)
      if (layer === 'adapters' && local.startsWith('src/adapters/') && file.split('/')[2] !== local.split('/')[2])
        fail(`Source and output adapters must not depend on one another: ${spec}`)
      if (layer === 'adapters' && target === 'application' && !local.startsWith('src/application/ports/'))
        fail(`Adapters may import application ports only: ${spec}`)
      if (/^(?:bridge|host|window|input)\//.test(spec) || local.startsWith('apps/')) fail(`Core cannot depend on native host: ${spec}`)
      if (/^(?:synchronization|model)\//.test(spec) && !file.startsWith('src/adapters/sources/pubg_reader/'))
        fail(`Frozen Reader SDK must stay behind its source adapter: ${spec}`)
      if (['domain', 'control', 'protocol'].includes(layer) && /^(?:windows\.h|winsock2\.h|d3d\d*\.h|dxgi.*\.h)$/i.test(spec)) fail(`Pure ${layer} layer cannot import OS/GPU API ${spec}`)
    }
  }
  if (file.startsWith('mousebox-firmware/') && /\.(?:ino|hpp|h|cpp)$/.test(file)) {
    const layer = file.split('/')[1]
    const firmwareRules = {
      protocol: ['protocol'], core: ['core', 'protocol'], profiles: ['profiles', 'core', 'protocol'],
      platforms: ['platforms', 'profiles', 'core', 'protocol'], apps: ['apps', 'core', 'protocol'],
      targets: ['targets', 'apps', 'platforms', 'profiles', 'core', 'protocol'],
    }
    for (const match of source.matchAll(/^\s*#\s*include\s*(["<])([^">]+)[">]/gm)) {
      const spec = slash(match[2])
      const target = path.posix.normalize(path.posix.join(path.posix.dirname(file), spec))
      if (/^(?:src|apps|packages)\//.test(target)) fail(`Firmware cannot depend on PC implementation ${spec}`)
      if (target.startsWith('mousebox-firmware/')) {
        const dependency = target.split('/')[1]
        if (firmwareRules[layer] && firmwareRules[dependency] && !firmwareRules[layer].includes(dependency)) fail(`Firmware ${layer} cannot depend on ${dependency}: ${spec}`)
        if (['profiles', 'platforms'].includes(layer) && /mousebox-firmware\/core\/(?:control|relay)\//.test(target)) fail(`Adapter cannot own control implementation: ${spec}`)
      }
      if (['core', 'protocol', 'profiles', 'apps'].includes(layer) && /^(?:Arduino\.h|avr\/|hidcomposite\.h|Usb\.h|SPI\.h|PluggableUSB\.h|USBAPI\.h)/.test(spec)) fail(`Portable firmware ${layer} cannot import platform SDK ${spec}`)
    }
  }
  return errors
}
