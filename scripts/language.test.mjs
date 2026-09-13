import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {translate} from '../docs/language.js';
import {polish} from '../docs/translations.mjs';
test('Polish dictionary is complete for visible HTML text and component details',async()=>{
 const unchanged=new Set(['R','BEYBLADE','RPM','BEYBLADE RPM','BEYBLADE RPM / ESP32-S3-Zero','V2.','ESP32-S3-Zero','FLASH','PSRAM','4 MB','01','0%']);
 for(const page of ['index','model','wiring']){
   const html=fs.readFileSync(new URL(`../docs/${page}.html`,import.meta.url),'utf8');
   assert.ok(html.includes('./language.js'));
   const source=html.replace(/<script[\s\S]*?<\/script>/g,'');
   for(const [,raw] of source.matchAll(/>([^<>]+)</g)){
     const value=raw.trim().replaceAll('&amp;','&');
     if(!/[a-z]/i.test(value)||unchanged.has(value))continue;
     assert.ok(polish[value],`${page}: ${value}`);
   }
 }
 const {components}=await import('../docs/wiring-data.mjs');
 for(const part of components){assert.ok(polish[part.note],part.id);}
});
test('English text and electrical identifiers stay unchanged',()=>{
 for(const text of ['Firmware version',' GPIO11 ','1 kΩ','SDA','ESP32-S3-Zero'])assert.equal(translate(text,'en'),text);
 for(const text of ['GPIO11','3V3','RES','SDA','ESP32-S3-Zero'])assert.equal(translate(text,'pl'),text);
 assert.equal(translate(' Firmware version ','pl'),' Wersja oprogramowania ');
});
test('Dynamic model and installer messages are translated',()=>{
 assert.match(translate('Selected · Case. All 28 parts remain visible.','pl'),/Wybrano · Case/);
 assert.match(translate('Ready · 28 parts. Click a part to highlight it.','pl'),/Gotowe/);
 assert.match(translate('Installer unavailable. Invalid release metadata. Check your connection and reload this page.','pl'),/Nieprawidłowe metadane/);
 assert.match(translate('display SDA → GPIO8','pl'),/ekran SDA → GPIO8/);
});
