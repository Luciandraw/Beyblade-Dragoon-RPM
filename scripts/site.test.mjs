import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const root=fileURLToPath(new URL('../docs/',import.meta.url));
function files(dir){return fs.readdirSync(dir,{withFileTypes:true}).flatMap(e=>e.isDirectory()?files(path.join(dir,e.name)):[path.join(dir,e.name)]);}
test('Static site references exist and remain inside the Pages directory',()=>{
 for(const file of files(root).filter(f=>/\.(html|css|m?js)$/.test(f))){
  const source=fs.readFileSync(file,'utf8');
  assert.ok(!/localhost:8080|127\.0\.0\.1:8080|C:\\Users|D:\\ArduinoProjects/.test(source),file);
  const refs=[...source.matchAll(/(?:href|src)=["']([^"']+)["']|url\(["']?([^)'"\s]+)|(?:from\s*|fetch\(|import\()["'](\.\.?\/[^"']+)["']/g)];
  for(const match of refs){const ref=match[1]||match[2]||match[3];if(/^(?:https?:|data:|#)/.test(ref))continue;
   const target=path.resolve(path.dirname(file),ref.split(/[?#]/)[0]);
   assert.ok(target===path.resolve(root)||target.startsWith(root),ref);
   assert.ok(fs.existsSync(target),`${path.relative(root,file)} → ${ref}`);
  }
 }
});
test('Landing page exposes both hardware tools and retains installer controls',()=>{
 const html=fs.readFileSync(path.join(root,'index.html'),'utf8');
 for(const token of ['lang="en"','./model.html','./wiring.html','id="installer"','id="consent"','id="install-host"'])assert.ok(html.includes(token),token);
 assert.ok(fs.existsSync(path.join(root,'.nojekyll')));
});
