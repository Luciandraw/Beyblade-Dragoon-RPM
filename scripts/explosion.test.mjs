import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {explosionLayer, explosionOffset} from '../docs/explosion-layout.mjs';
test('All model parts use aligned Y-only offsets; assembly restores exactly',()=>{
 const bytes=fs.readFileSync(new URL('../docs/models/LauncherV2.glb',import.meta.url));
 const gltf=JSON.parse(bytes.toString('utf8',20,20+bytes.readUInt32LE(12)));
 assert.equal(gltf.nodes.length,26);
 assert.equal(gltf.scenes[gltf.scene ?? 0].nodes.length,26);
 for(const removed of ['Board','PinHeads','PinSockets'])assert.ok(!gltf.nodes.some(n=>n.name===removed));
 for(const node of gltf.nodes){
   const offset=explosionOffset(node.name,20);
   assert.equal(offset[0],0);assert.equal(offset[2],0);
   assert.equal(explosionLayer(node.name),explosionLayer(node.name.replace(/\s/g,'_').replace(/\./g,'')));
   if(node.name!=='Case')assert.notEqual(offset[1],0,node.name);
   const origin=node.translation;
   assert.deepEqual(origin.map((v,i)=>v+offset[i]*0),origin);
 }
 assert.equal(explosionLayer('HeadTop'),explosionLayer('Eye1'));
 assert.equal(explosionLayer('Eye1'),explosionLayer('Eye2'));
 assert.equal(explosionLayer('LauncherClip 1'),explosionLayer('LauncherClip 2'));
 assert.equal(explosionLayer('Switch'),explosionLayer('SwitchCover'));
 assert.equal(explosionLayer('HeadDecorConnector_2'),explosionLayer('HeadDecorConnector_3'));
});
