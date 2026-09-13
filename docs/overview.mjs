import {photos, connections, powerCircuit, mapPoint} from './contact-map.mjs';
import {components} from './wiring-data.mjs';

export const accents={power:'#ff9757',switch:'#ff6666',esp:'#c4f866',display:'#58b9ff',sensor:'#50d4ff',blade:'#f28bd4',center:'#75e5a3'};
const ground='#bac4d0';
// One color per physical connection, shared by wire, endpoint, legend and table.
export const wireColors={
 'display-VCC':'#ffb454','display-gnd':'#cbd5e1','display-RES':'#ff7895',
 'display-D/C':'#bf9aff','display-SCL':'#58b9ff','display-BUSY':'#f5e45c',
 'display-SDA':'#50e0cf','display-CS':'#a6ef70',
 'sensor-VCC':'#ffce93','sensor-OUT':'#50d4ff','sensor-gnd':'#98a9c9',
 'blade-Lower left':'#f28bd4','blade-gnd':'#d8b5d0',
 'center-Left':'#75e5a3','center-gnd':'#afc9ab',
 'power-0':'#ff9757','power-1':'#ff6666','power-gnd':'#acbac2',
 'esp-gnd':'#e3e9f0','battery-B+':'#edbe45','battery-B−':'#b8a795'
};
export function rowColor(id,pair){
 const [terminal,destination]=pair;
 if(id==='power')return wireColors[{'LiPo 1S +':'battery-B+','LiPo 1S −':'battery-B−','VO+':'power-0','VO−':'power-gnd'}[terminal]]||accents[id];
 if(id==='switch')return wireColors[{'Middle leg':'power-1','Right leg':'power-0'}[terminal]]||ground;
 if(/GND/.test(destination)||terminal==='GND')return wireColors[`${id}-gnd`]||ground;
 if(id==='blade')return wireColors['blade-Lower left'];
 if(id==='center')return wireColors['center-Left'];
 if(wireColors[`${id}-${terminal}`])return wireColors[`${id}-${terminal}`];
 if(id==='esp'){
  const keys={'GPIO10':'blade-Lower left','GPIO11':'center-Left','GPIO1':'sensor-OUT','5V':'power-1','GPIO4':'display-RES','GPIO5':'display-D/C','GPIO6':'display-SCL','GPIO7':'display-BUSY','GPIO8':'display-SDA','GPIO9':'display-CS'};
  if(keys[terminal])return wireColors[keys[terminal]];
 }
 return accents[id];
}
// Non-destructive SVG clipping of original photos. Contact coordinates and
// image pixels are unchanged; these contours only hide the tabletop.
export const contours={
 power:'165,720 650,678 650,650 977,628 985,665 1060,660 1095,1370 208,1440',
 esp:'262,651 793,647 801,1350 662,1358 657,1402 402,1402 398,1360 273,1358',
 sensor:'233,922 551,903 578,1481 264,1507',
 display:'353,214 385,167 1625,145 1697,191 1700,300 1635,375 1658,407 1664,750 1605,765 1697,860 1702,954 1655,1015 399,1032 354,984',
 blade:'480,757 490,749 491,721 516,715 521,749 576,739 578,719 598,717 605,740 650,735 650,702 677,700 682,736 707,733 733,955 712,966 716,1002 692,1007 687,971 552,987 556,1024 532,1027 527,985 505,982',
 center:'370,616 396,597 477,595 504,613 529,967 590,975 602,1090 598,1253 589,1322 569,1327 554,1254 550,1140 374,1140 371,1260 363,1330 344,1330 330,1257 316,1110 308,1000 325,981 371,973',
 switch:'510,777 790,774 794,889 741,889 741,1004 719,1004 718,892 660,892 660,1003 640,1003 640,893 577,893 579,1003 559,1003 559,889 507,888'
};
export const boxes={power:[75,135,205,155],switch:[390,145,125,105],esp:[500,290,170,220],display:[70,615,230,155],center:[380,600,85,170],sensor:[685,600,95,170],blade:[920,625,120,150]};
export function drawOverview(selected,container,onSelect){
 const ns='http://www.w3.org/2000/svg';
 const el=(tag,a={},text)=>{const n=document.createElementNS(ns,tag);for(const[k,v]of Object.entries(a))n.setAttribute(k,v);if(text)n.textContent=text;return n;};
 container.replaceChildren();
 const svg=el('svg',{viewBox:'0 0 1120 900',role:'img','aria-label':'Complete wiring overview. Select components above or wire labels below for details.'});
 const defs=el('defs');svg.append(defs);
 const point=(id,pin)=>mapPoint(photos[id],photos[id].pins[pin],boxes[id]);
 const text=(x,y,value,color='#edf3e8')=>el('text',{x,y,style:`fill:${color}`,'text-anchor':'middle'},value);
 for(const part of components){
  const p=photos[part.id],[x,y,w,h]=p.crop,b=boxes[part.id],s=Math.min(b[2]/w,b[3]/h);
  const clip=el('clipPath',{id:`outline-${part.id}`,clipPathUnits:'userSpaceOnUse'});clip.append(el('polygon',{points:contours[part.id]}));defs.append(clip);
  const g=el('g',{'data-component-photo':part.id});
  const frame=el('svg',{x:b[0]+(b[2]-w*s)/2,y:b[1]+(b[3]-h*s)/2,width:w*s,height:h*s,viewBox:p.crop.join(' ')});
  const size=p.size||[1152,2048];frame.append(el('image',{href:`./assets/${part.id}-original.jpg`,width:size[0],height:size[1],'clip-path':`url(#outline-${part.id})`}));
  g.append(frame,text(b[0]+b[2]/2,b[1]-16,part.name,accents[part.id]));g.addEventListener('click',()=>onSelect(part.id));svg.append(g);
 }
 // External battery and USB are labelled terminal nodes, not invented photos.
 svg.append(el('rect',{x:85,y:35,width:180,height:42,rx:8,fill:'#252d25',stroke:'#58664b'}),text(175,61,'LiPo 1S · 3.7 V'),text(900,150,'Computer USB-C'),text(900,174,'Power switch OFF'),text(445,277,'Left leg unused',ground));
 const usb=mapPoint(photos.esp,[530,1375],boxes.esp);
 svg.append(el('path',{d:`M900 190 V260 H${usb[0]} V${usb[1]}`,fill:'none',stroke:'#869584','stroke-dasharray':'5 5','stroke-width':2}));
 const groups=[],legend=document.createElement('div');legend.className='wire-legend';legend.setAttribute('role','group');legend.setAttribute('aria-label','Wire colors and connection selection');
 function wire(key,label,a,b,color,d,owner){
  const g=el('g',{'data-overview-wire':key,opacity:selected==='esp'||selected===owner||owner==='power'&&selected==='switch'?'1':'.22'});
  g.append(el('path',{d,stroke:'#161b17','stroke-width':6,fill:'none'}),el('path',{d,stroke:color,'stroke-width':2.6,fill:'none'}));
  for(const p of[a,b])g.append(el('circle',{cx:p[0],cy:p[1],r:4,fill:'#161b17',stroke:color,'stroke-width':2}));svg.append(g);groups.push(g);
  if(selected==='esp'||selected===owner||owner==='power'&&selected==='switch'){
   const button=document.createElement('button');button.type='button';button.style.setProperty('--wire-color',color);button.textContent=label;button.setAttribute('aria-pressed','false');
   button.addEventListener('click',()=>{const reset=button.getAttribute('aria-pressed')==='true';for(const item of groups)item.setAttribute('opacity',reset?'1':item===g?'1':'.1');for(const item of legend.children)item.setAttribute('aria-pressed',String(!reset&&item===button));});legend.append(button);
  }
 }
 // Ground is a single output-side net. Battery negative never joins it directly.
 svg.append(el('path',{d:'M55 825 H1065',fill:'none',stroke:ground,'stroke-width':3}),text(555,858,'COMMON GND · VO− / OUTPUT SIDE',ground));
 for(const id of ['power','esp','display','sensor','blade','center']){
  const pin=id==='power'?'VO−':id==='blade'?'Lower right':id==='center'?'Right':'GND',a=point(id,pin),b=[id==='power'?55:id==='esp'?1065:a[0],825];
  const d=id==='power'?`M${a} H55 V825`:id==='esp'?`M${a} H1065 V825`:`M${a} V825`;
  wire(`${id}-gnd`,`${id} ${pin} → common GND`,a,b,wireColors[`${id}-gnd`],d,id);
 }
 for(const[i,c]of powerCircuit.slice(0,2).entries()){
  const a=point(c.source,c.from),b=point(c.target,c.to),lane=315+i*22;
  wire(`power-${i}`,`${c.source} ${c.from} → ${c.target} ${c.to}`,a,b,wireColors[`power-${i}`],`M${a} V${lane} H${b[0]} V${b[1]}`,'power');
 }
 for(const [i,pin]of ['B+','B−'].entries()){
  const a=[135+i*80,77],b=point('power',pin),rail=35+i*16,color=wireColors[`battery-${pin}`];
  wire(`battery-${pin}`,`LiPo ${i?'−':'+'} → ${pin}`,a,b,color,`M${a} H${rail} V${b[1]} H${b[0]}`,'power');
 }
 for(const id of ['display','center','sensor','blade']){
  for(const[i,c]of connections[id].entries()){
   if(c.to==='GND')continue;
   const a=point('esp',c.to),b=point(id,c.from),lane=535+i*8;
   const color=rowColor(id,[c.from,c.to]);
   const rail=id==='display'?315+i*15:id==='center'?475:id==='sensor'?810:880;
   const d=`M${a} H${rail} V${lane} H${b[0]} V${b[1]}`;
   wire(`${id}-${c.from}`,`${id} ${c.from} → ${c.via?c.via+' → ':''}${c.to}`,a,b,color,d,id);
   if(c.via){svg.append(el('rect',{x:rail-23,y:lane-10,width:46,height:20,fill:'#161b17',stroke:color,'stroke-width':2}),text(rail,lane+5,'1 kΩ',color));}
  }
 }
 container.append(svg,legend);
}
