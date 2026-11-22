export function selectChecks(changes, firstRun = false) {
  const full = firstRun || changes.some(file => /^(?:packages\/shared\/|protocol\/|third_party\/|build\/|projects\/|tests\/fixtures\/|data\/games\/pubg\/)/.test(file)
    || /^data\/fonts\//.test(file)
    || /^(?:package\.json|pnpm-lock\.yaml|tsconfig\.base\.json|vitest\.config\.ts|eslint\.config\.js)$/.test(file))
  const native = changes.filter(file => /^(?:src\/|apps\/native-host\/|tests\/native\/)/.test(file) || /^tests\/.*\.(?:cpp|hpp|h)$/.test(file))
  let suites = []
  if (full) suites = ['all']
  else if (native.length) {
    if (native.every(file => /^(?:src\/(?:games\/cs2\/|adapters\/(?:sources\/cs2_reader\/|spatial\/traceline\/))|tests\/native\/(?:cs2_|traceline_))/.test(file))) suites = ['cs2']
    else suites = ['all'] // Unknown/shared native dependencies conservatively run the complete suite.
  }
  const web = changes.filter(file => /^(?:apps\/(?:ui|desktop)\/|packages\/shared\/|tests\/(?:desktop|ui|interop)\/)/.test(file))
  return { full, suites, web: full ? 'all' : web, tools: full || changes.some(file => /^(?:tools\/|tests\/workflow\/|tests\/assets\/|data\/games\/cs2\/hitboxes\/|src\/games\/cs2\/hitboxes\/generated\/)/.test(file)),
    interop: full || native.length > 0 || web.some(file => file.startsWith('tests/interop/')),
    assets: changes.some(file => /^(?:data\/games\/cs2\/cover\/|third_party\/traceline\/|src\/adapters\/spatial\/traceline\/|src\/platform\/windows\/dynamic_library\.)/.test(file)) }
}
