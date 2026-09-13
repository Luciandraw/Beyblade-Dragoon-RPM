import test from 'node:test';
import assert from 'node:assert/strict';
import {existsSync} from 'node:fs';
import {photos,connections,mapPoint,drawContacts} from '../docs/contact-map.mjs';
import {drawOverview,rowColor,accents} from '../docs/overview.mjs';
test('Overview contains seven clipped photos and matching wire/detail colors',()=>{
 class E{constructor(){this.children=[];this.a={};this.style={setProperty(){}};this.events={};}setAttribute(k,v){this.a[k]=v;}getAttribute(k){return this.a[k];}append(...v){this.children.push(...v);}replaceChildren(){this.children=[];}addEventListener(k,f){this.events[k]=f;}}
 globalThis.document={createElement:()=>new E(),createElementNS:()=>new E()};
 try{const root=new E();drawOverview('esp',root,()=>{});const[svg,legend]=root.children;assert.equal(svg.children.filter(c=>c.a['data-component-photo']).length,7);assert.ok(legend.children.length>15);legend.children[0].events.click();assert.equal(legend.children[0].getAttribute('aria-pressed'),'true');assert.equal(rowColor('display',['SCL','GPIO6']),accents.display);assert.equal(rowColor('switch',['Right leg','Boost VO+']),accents.power);}finally{delete globalThis.document;}
});
test('All wire endpoints are named photo contacts inside original photo crops',()=>{
 for(const[id,photo]of Object.entries(photos)){assert.ok(existsSync(new URL(`../docs/assets/${id}-original.jpg`,import.meta.url)));const[x,y,w,h]=photo.crop;for(const p of Object.values(photo.pins))assert.ok(p[0]>=x&&p[0]<=x+w&&p[1]>=y&&p[1]<=y+h);}
 for(const[id,wires]of Object.entries(connections))for(const wire of wires){assert.ok(photos[wire.source||id].pins[wire.from]);assert.ok(photos[wire.target||'esp'].pins[wire.to]);}
 assert.equal(connections.display.length,8);assert.equal(connections.blade[0].to,'GPIO10');assert.equal(connections.center[0].to,'GPIO11');assert.equal(connections.sensor[2].via,'1 kΩ series');assert.equal(connections.power.length,3);
 assert.equal(connections.switch[0].to,'Right');assert.equal(connections.switch[1].from,'Middle');assert.equal(connections.switch[1].to,'5V');assert.ok(connections.switch.every(w=>w.from!=='Left'&&w.to!=='Left'));
});
test('Point mapping uses the same aspect-fit transform as photos',()=>{const p={crop:[10,20,100,200]};assert.deepEqual(mapPoint(p,[10,20],[0,0,200,200]),[50,0]);assert.deepEqual(mapPoint(p,[110,220],[0,0,200,200]),[150,200]);});
test('Contact drawing renders and wire buttons isolate and restore endpoints',()=>{
 class E{constructor(){this.children=[];this.a={};this.style={setProperty(){}};this.events={};}setAttribute(k,v){this.a[k]=v;}getAttribute(k){return this.a[k];}append(...v){this.children.push(...v);}replaceChildren(){this.children=[];}addEventListener(k,f){this.events[k]=f;}}
 globalThis.document={createElement:()=>new E(),createElementNS:()=>new E()};
 try{for(const id of Object.keys(photos)){const root=new E();drawContacts(id,root);assert.ok(root.children.length);if(id==='display'){const[svg,legend]=root.children;assert.equal(legend.children.length,8);legend.children[0].events.click();assert.equal(legend.children[0].getAttribute('aria-pressed'),'true');const wires=svg.children.filter(c=>c.a['data-wire']);assert.equal(wires.filter(g=>g.a.opacity==='1').length,1);legend.children[0].events.click();assert.ok(wires.every(g=>g.a.opacity==='1'));}}}finally{delete globalThis.document;}
});
