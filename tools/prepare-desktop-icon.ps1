$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$repoRoot = Split-Path $PSScriptRoot -Parent
$resourceDirectory = Join-Path $repoRoot 'apps/desktop/resources'
$source = [Drawing.Image]::FromFile((Join-Path $repoRoot 'data/branding/application-icon.png'))
$images = @()
try {
    foreach ($size in @(16, 24, 32, 48, 64, 128, 256)) {
        $bitmap = [Drawing.Bitmap]::new($size, $size, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $graphics = [Drawing.Graphics]::FromImage($bitmap)
        $memory = [IO.MemoryStream]::new()
        try {
            $bitmap.SetResolution(96, 96)
            $graphics.Clear([Drawing.Color]::Transparent)
            $graphics.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
            $graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
            # Preserve the supplied artwork, its transparency and its aspect ratio.
            $scale = [Math]::Min($size / $source.Width, $size / $source.Height)
            $width = [single]($source.Width * $scale)
            $height = [single]($source.Height * $scale)
            $rectangle = [Drawing.RectangleF]::new(($size - $width) / 2, ($size - $height) / 2, $width, $height)
            $graphics.DrawImage($source, $rectangle)
            $bitmap.Save($memory, [Drawing.Imaging.ImageFormat]::Png)
            $images += ,@{ size = $size; bytes = $memory.ToArray() }
        } finally {
            $memory.Dispose()
            $graphics.Dispose()
            $bitmap.Dispose()
        }
    }
} finally { $source.Dispose() }

$stream = [IO.File]::Create((Join-Path $resourceDirectory 'icon.ico'))
$writer = [IO.BinaryWriter]::new($stream)
try {
    $writer.Write([UInt16]0)
    $writer.Write([UInt16]1)
    $writer.Write([UInt16]$images.Count)
    $offset = 6 + 16 * $images.Count
    foreach ($entry in $images) {
        $dimension = if ($entry.size -eq 256) { 0 } else { $entry.size }
        $writer.Write([Byte]$dimension)
        $writer.Write([Byte]$dimension)
        $writer.Write([Byte]0)
        $writer.Write([Byte]0)
        $writer.Write([UInt16]1)
        $writer.Write([UInt16]32)
        $writer.Write([UInt32]$entry.bytes.Length)
        $writer.Write([UInt32]$offset)
        $offset += $entry.bytes.Length
    }
    foreach ($entry in $images) { $writer.Write([Byte[]]$entry.bytes) }
} finally {
    $writer.Dispose()
    $stream.Dispose()
}
