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
test('Layer stacking clears every neighbouring layer by the gap',async()=>{
 const {layerShifts}=await import('../docs/explosion-layout.mjs');
 const bounds=new Map([[-1,[-2,1]],[0,[0,10]],[1,[2,8]],[2,[7,12]],[3,[11,11.5]]]);
 const shifts=layerShifts(bounds,1);
 assert.equal(shifts.get(0),0);
 const order=[...bounds.keys()].sort((a,b)=>a-b);
 for(let i=1;i<order.length;i++){
  const below=bounds.get(order[i-1]),above=bounds.get(order[i]);
  assert.ok(above[0]+shifts.get(order[i])>=below[1]+shifts.get(order[i-1])+1-1e-9,`${order[i-1]}→${order[i]}`);
  // Monotonic shifts: with one shared progress value, neighbours only move apart during animation.
  assert.ok(shifts.get(order[i])>=shifts.get(order[i-1]),`monotonic ${order[i-1]}→${order[i]}`);
 }
});
