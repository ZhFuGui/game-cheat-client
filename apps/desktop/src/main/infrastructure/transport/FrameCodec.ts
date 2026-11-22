export const MAX_FRAME_BYTES = 8 * 1024 * 1024

/** Windows native pipe protocol: unsigned 32-bit LE byte count + UTF-8 JSON. */
export class FrameCodec {
  private buffer:Buffer = Buffer.alloc(0)
  push(chunk: Buffer): unknown[] {
    this.buffer = this.buffer.length ? Buffer.concat([this.buffer, chunk]) : chunk
    const frames: unknown[] = []
    let offset = 0
    while (this.buffer.length - offset >= 4) {
      const size = this.buffer.readUInt32LE(offset)
      if (size === 0 || size > MAX_FRAME_BYTES) throw new Error('Invalid native frame length')
      if (this.buffer.length - offset < size + 4) break
      frames.push(JSON.parse(this.buffer.toString('utf8', offset + 4, offset + 4 + size)))
      offset += size + 4
    }
    if (offset) this.buffer = this.buffer.subarray(offset)
    if (this.buffer.length > MAX_FRAME_BYTES + 4) throw new Error('Native frame buffer overflow')
    return frames
  }
  end(): void { if (this.buffer.length) throw new Error('Truncated native frame') }
  static encode(value: unknown): Buffer {
    const body = Buffer.from(JSON.stringify(value), 'utf8')
    if (!body.length || body.length > MAX_FRAME_BYTES) throw new Error('Native frame too large')
    const frame = Buffer.allocUnsafe(body.length + 4)
    frame.writeUInt32LE(body.length)
    body.copy(frame, 4)
    return frame
  }
}
