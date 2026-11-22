import { createHash } from 'node:crypto'
import { createReadStream } from 'node:fs'
import { mkdir, readFile, readdir, rename, stat, writeFile } from 'node:fs/promises'
import path from 'node:path'

const ignored = new Set(['node_modules', 'dist', 'out', '.git', '.migration', '.vs', '__pycache__', '.pytest_cache'])
const memo = new Map()
const refresh = Symbol('source snapshot')
export async function files(root, entries, { generated = false } = {}) {
  const result = new Set()
  async function visit(relative) {
    const absolute = path.join(root, relative)
    let info
    try { info = await stat(absolute) } catch (error) { if (error.code === 'ENOENT') return; throw error }
    if (info.isDirectory()) {
      for (const entry of await readdir(absolute)) if (generated || !ignored.has(entry)) await visit(path.join(relative, entry))
    } else if (info.isFile()) result.add(relative.replaceAll('\\', '/'))
  }
  for (const entry of entries) await visit(entry)
  return [...result].sort()
}
export async function sha(file) {
  const info = await stat(file, { bigint: true })
  const signature = [info.size, info.mtimeNs, info.ctimeNs, info.ino].join(':')
  const previous = memo.get(file)
  if (previous?.signature === signature) return previous.value
  const value = (async () => {
    const hash = createHash('sha256')
    for await (const block of createReadStream(file)) hash.update(block)
    return hash.digest('hex')
  })()
  memo.set(file, { signature, value })
  try { return await value } catch (error) { memo.delete(file); throw error }
}
export async function snapshot(root, entries, { metadata = false, generated = false, filter } = {}) {
  const result = {}
  for (const file of await files(root, entries, { generated })) {
    if (filter && !filter(file)) continue
    const absolute = path.join(root, file)
    if (metadata) {
      const info = await stat(absolute, { bigint: true })
      result[file] = [info.size, info.mtimeNs, info.ctimeNs, info.ino].map(String).join(':')
    } else result[file] = await sha(absolute)
  }
  Object.defineProperty(result, refresh, { value: async () => snapshot(root, entries, { metadata, generated, filter }) })
  return result
}
async function validate(input) {
  if (!input || typeof input !== 'object') return
  if (input[refresh]) {
    if (digest(input) !== digest(await input[refresh]())) throw new Error('Inputs changed during the step; retry before reusing its result')
  } else for (const value of Object.values(input)) await validate(value)
}
export const digest = value => createHash('sha256').update(JSON.stringify(value)).digest('hex')
export const changedFiles = (before, after) => [...new Set([...Object.keys(before), ...Object.keys(after)])]
  .filter(file => before[file] !== after[file]).sort()

export class Cache {
  constructor(root, location) { this.root = root; this.location = location; this.data = { schema: 1, steps: {} }; this.report = [] }
  async load() {
    try { const data = JSON.parse(await readFile(this.location, 'utf8')); if (data.schema === 1) this.data = data }
    catch (error) { if (error.code !== 'ENOENT' && !(error instanceof SyntaxError)) throw error }
  }
  async save() {
    this.saving = (this.saving ?? Promise.resolve()).then(async () => {
      await mkdir(path.dirname(this.location), { recursive: true })
      const temporary = `${this.location}.${process.pid}.tmp`
      await writeFile(temporary, JSON.stringify(this.data, null, 2) + '\n')
      await rename(temporary, this.location)
    })
    await this.saving
  }
  async step(name, input, outputs, action, { force = false, metadata = false } = {}) {
    const inputHash = digest(input)
    const output = await snapshot(this.root, outputs, { metadata, generated: true })
    const previous = this.data.steps[name]
    if (!force && previous?.input === inputHash && previous.output === digest(output) &&
        (!outputs.length || Object.keys(output).length > 0)) {
      this.report.push({ name, skipped: true, ms: 0 })
      console.log(`SKIP ${name}`)
      return false
    }
    console.log(`RUN  ${name}`)
    const start = performance.now()
    try { await action() }
    catch (error) {
      this.report.push({ name, skipped: false, failed: true, ms: Math.round(performance.now() - start) })
      throw error
    }
    const after = await snapshot(this.root, outputs, { metadata, generated: true })
    if (outputs.length && !Object.keys(after).length) throw new Error(`${name}: expected outputs are missing`)
    await validate(input)
    this.data.steps[name] = { input: inputHash, output: digest(after) }
    this.report.push({ name, skipped: false, ms: Math.round(performance.now() - start) })
    await this.save()
    return true
  }
}
