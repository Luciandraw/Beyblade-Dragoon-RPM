import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync, existsSync} from 'node:fs';
import {components, photoCrops} from '../docs/wiring-data.mjs';
const config=readFileSync(new URL('../main/config.h',import.meta.url),'utf8');
test('Wiring follows the firmware GPIO assignment',()=>{
 const expected={RES:'PIN_EPD_RST','D/C':'PIN_EPD_DC',SCL:'PIN_EPD_SCK',BUSY:'PIN_EPD_BUSY',SDA:'PIN_EPD_MOSI',CS:'PIN_EPD_CS'};
 const pins=Object.fromEntries(components.find(c=>c.id==='display').pins);
 for(const [name,key] of Object.entries(expected)) assert.equal(pins[name],`GPIO${config.match(new RegExp(`${key} = (\\d+)`))[1]}`);
 for(const [id,key] of [['blade','PIN_BLADE_PRESENT'],['center','PIN_BUTTON_CENTER']]) assert.equal(components.find(c=>c.id===id).pins[0][1],`GPIO${config.match(new RegExp(`${key} = (\\d+)`))[1]}`);
 assert.match(components.find(c=>c.id==='sensor').pins[2][1],new RegExp(`GPIO${config.match(/PIN_SENSOR = (\d+)/)[1]}$`));
});
test('All seven components have complete data and valid photo bounds',()=>{
 assert.equal(new Set(components.map(c=>c.id)).size,7);
 for(const c of components){assert.ok(c.note&&c.name&&c.pins.length);const[x,y,w,h]=c.crop;assert.ok(x>=0&&y>=0&&w>0&&h>0&&x+w<=1536&&y+h<=1024);}
 assert.ok(existsSync(new URL('../docs/assets/components-review.png',import.meta.url)));
 assert.ok(existsSync(new URL('../docs/assets/components-dark.png',import.meta.url)));
 for(const part of components){const[x,y,w,h]=photoCrops[part.id];assert.ok(x>=0&&y>=0&&w>0&&h>0&&x+w<=1536&&y+h<=1024);}
});
test('Every selector updates detail and highlights its link without USB access',async()=>{
 class Element{constructor(){this.children=[];this.dataset={};this.attrs={};this.style={};this.classList={toggle:(k,v)=>{this[k]=v;}};}setAttribute(k,v){this.attrs[k]=v;}append(...items){this.children.push(...items);}replaceChildren(...items){this.children=items;}querySelectorAll(){return this.children;}addEventListener(){}createTHead(){return this.section();}createTBody(){return this.section();}section(){return {insertRow:()=>({append(){},insertCell:()=>({style:{}})})};}}
 const grid=new Element(), detail=new Element(), paths=components.filter(c=>c.id!=='esp').map(c=>{const p=new Element();p.dataset.link=c.id;return p;});
 globalThis.document={querySelector:s=>s==='#components'?grid:s==='#connection-detail'?detail:null,querySelectorAll:()=>paths,createElement:()=>new Element()};
 try{const{selectComponent}=await import('../docs/wiring.js');assert.equal(grid.children.length,7);for(const part of components){selectComponent(part.id);assert.equal(detail.children[0].textContent,part.name);assert.equal(grid.children.filter(b=>b.attrs['aria-pressed']==='true').length,1);for(const p of paths)assert.equal(p.active,part.id==='esp'||p.dataset.link===part.id);}}finally{delete globalThis.document;}
});
