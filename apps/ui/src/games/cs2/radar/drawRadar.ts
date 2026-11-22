import type { RadarFloor } from "./maps";
import { layoutRadarMarkers, screenHeading, type RadarCamera, type RadarMarker, type RadarMarkerPlacement } from "./projection";

function drawMarker(ctx: CanvasRenderingContext2D, camera: RadarCamera, marker: RadarMarkerPlacement, names: boolean) {
  const { player, relativeFloor, position: { x, y } } = marker;
  const color = player.team === 3 ? "#76c3ff" : "#ffc06c";
  const current = relativeFloor === "current";
  const radius = current ? (player.local ? 7 : 5) : (player.local ? 10 : 9);
  if (current) {
    if (player.yaw !== null) {
      const direction = screenHeading(camera, player.yaw);
      ctx.beginPath();
      ctx.moveTo(x, y);
      ctx.lineTo(x + Math.cos(direction) * (radius + 9), y + Math.sin(direction) * (radius + 9));
      ctx.lineCap = "round";
      ctx.lineWidth = 3;
      ctx.strokeStyle = color;
      ctx.stroke();
    }
    ctx.beginPath();
    ctx.arc(x, y, radius, 0, Math.PI * 2);
    ctx.fillStyle = color;
    ctx.fill();
    ctx.lineWidth = player.local ? 2 : 1.5;
    ctx.strokeStyle = player.local ? "#fff" : "#10202e";
    ctx.stroke();
  } else {
    ctx.beginPath();
    ctx.moveTo(x, y - radius);
    ctx.lineTo(x + radius, y);
    ctx.lineTo(x, y + radius);
    ctx.lineTo(x - radius, y);
    ctx.closePath();
    ctx.fillStyle = "#10202e";
    ctx.fill();
    ctx.lineWidth = player.local ? 2.5 : 1.5;
    ctx.lineJoin = "round";
    ctx.strokeStyle = color;
    ctx.stroke();
    ctx.font = "700 12px system-ui, sans-serif";
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillStyle = color;
    ctx.fillText(relativeFloor === "above" ? "↑" : "↓", x, y);
  }

  if (names && player.name) {
    const labelY = current || y > marker.anchor.y + 12 || y < 26 ? y + radius + 6 : y - radius - 16;
    ctx.font = "600 12px system-ui, sans-serif";
    ctx.textAlign = "center";
    ctx.textBaseline = "top";
    ctx.lineJoin = "round";
    ctx.lineWidth = 3;
    ctx.strokeStyle = "#101921";
    ctx.strokeText(player.name, x, labelY, 130);
    ctx.fillStyle = current ? "#fff" : color;
    ctx.fillText(player.name, x, labelY, 130);
  }
}

export function drawRadar(canvas: HTMLCanvasElement, image: HTMLImageElement | undefined, camera: RadarCamera,
                          markers: readonly RadarMarker[], floor: RadarFloor, names: boolean) {
  if (!camera.width || !camera.height) return;
  const dpr = Math.min(2, window.devicePixelRatio || 1);
  const width = Math.round(camera.width * dpr), height = Math.round(camera.height * dpr);
  if (canvas.width !== width) canvas.width = width;
  if (canvas.height !== height) canvas.height = height;
  const ctx = canvas.getContext("2d");
  if (!ctx) return;
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, camera.width, camera.height);
  if (!image) return;

  ctx.save();
  ctx.translate(camera.width / 2, camera.height / 2);
  ctx.rotate(camera.rotation);
  ctx.drawImage(image, -camera.center.x * camera.extent, -camera.center.y * camera.extent,
                camera.extent, camera.extent);
  ctx.restore();
  const placements = layoutRadarMarkers(camera, markers, floor);
  ctx.save();
  ctx.setLineDash([2, 2]);
  ctx.lineWidth = 1;
  ctx.strokeStyle = "#adc7d8";
  for (const { anchor, position } of placements) {
    if (anchor === position) continue;
    ctx.beginPath();
    ctx.moveTo(anchor.x, anchor.y);
    ctx.lineTo(position.x, position.y);
    ctx.stroke();
  }
  ctx.restore();
  for (const marker of placements) drawMarker(ctx, camera, marker, names);
}
