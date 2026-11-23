import {createHash} from 'node:crypto';
import {readFile} from 'node:fs/promises';
import {fileURLToPath} from 'node:url';
import path from 'node:path';

const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../apps/ui/public/radar/valorant');
const manifest=JSON.parse(await readFile(path.resolve(root,'../../../src/games/valorant/radar/manifest.json'),'utf8'));
if(manifest.schemaVersion!==1||!Array.isArray(manifest.maps)||!manifest.maps.length)throw Error('Invalid VALORANT radar manifest');
const identities=new Set();
for(const map of manifest.maps){
  if(!/^[a-f0-9-]{36}$/.test(map.id)||map.image!==`${map.id}.png`||identities.has(map.id)||
    !map.mapUrl.startsWith('/Game/Maps/')||!['xMultiplier','yMultiplier','xScalarToAdd','yScalarToAdd'].every(key=>Number.isFinite(map[key]))||
    !map.xMultiplier||!map.yMultiplier)throw Error('Invalid VALORANT map entry');
  identities.add(map.id);
  const bytes=await readFile(path.join(root,map.image));
  if(bytes.subarray(0,8).toString('hex')!=='89504e470d0a1a0a'||bytes.readUInt32BE(16)!==bytes.readUInt32BE(20)||
    createHash('sha256').update(bytes).digest('hex')!==map.sha256)throw Error('VALORANT radar image mismatch: '+map.name);
}
console.log(`PASS: ${identities.size} pinned VALORANT radar images and coordinate mappings`);
