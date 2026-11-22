const { readdir, lstat, realpath, rm, rmdir } = require('node:fs/promises')
const path = require('node:path')
const { createRequire } = require('node:module')

const licenseDirectory = /^(?:licen[sc]es?)$/i
function isLicenseFile(name) {
  if (!name) return false
  const text = ['', '.txt', '.md', '.html', '.htm', '.rtf', '.json', '.license', '.licence']
  return text.includes(path.extname(name).toLowerCase()) &&
    /(?:^|[._-])(?:licen[sc]es?|notice|copying|copyright)(?:[._-]|$)/i.test(name)
}
async function stripDistributionLicenses(directory) {
  const root = await realpath(directory)
  if (root === path.parse(root).root) throw new Error('Package output cannot be a filesystem root')
  const removed = []
  const walk = async (current, inLicenseDirectory = false) => {
    for (const entry of await readdir(current, { withFileTypes: true })) {
      const file = path.resolve(current, entry.name)
      if (!file.startsWith(root + path.sep)) throw new Error('Package cleanup escaped its output directory')
      if ((await lstat(file)).isSymbolicLink()) continue
      if (entry.isDirectory()) {
        const remove = inLicenseDirectory || licenseDirectory.test(entry.name)
        await walk(file, remove)
        if (remove) await rmdir(file)
      } else if (inLicenseDirectory || isLicenseFile(entry.name)) {
        await rm(file)
        removed.push(path.relative(root, file))
      }
    }
  }
  await walk(root)
  return removed
}
async function afterPack(context) {
  const removed = await stripDistributionLicenses(context.appOutDir)
  const resources = context.electronPlatformName === 'darwin'
    ? path.join(context.appOutDir, `${context.packager.appInfo.productFilename}.app`, 'Contents', 'Resources')
    : path.join(context.appOutDir, 'resources')
  const archive = path.join(resources, 'app.asar')
  if (await lstat(archive).catch(() => null)) {
    const desktop = createRequire(path.resolve(__dirname, '../apps/desktop/package.json'))
    const builder = createRequire(desktop.resolve('electron-builder/package.json'))
    const asar = createRequire(builder.resolve('app-builder-lib/package.json'))('@electron/asar')
    const leftovers = asar.listPackage(archive).filter(file => {
      const parts = file.split(/[\\/]/).filter(Boolean)
      return parts.slice(0, -1).some(part => licenseDirectory.test(part)) || isLicenseFile(parts.at(-1))
    })
    if (leftovers.length) throw new Error('Archive still contains excluded package documents: ' + leftovers.join(', '))
  }
  console.log(`Package document cleanup: ${removed.length} files removed`)
}
module.exports = afterPack
module.exports.stripDistributionLicenses = stripDistributionLicenses
