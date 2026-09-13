// Coordinates measured on unmodified user photographs, expressed at their
// displayed 1152x2048 (portrait) or 2048x1152 (landscape) dimensions.
export const photos = {
 esp:{crop:[250,635,560,780],pins:{GPIO1:[767,1084],GPIO4:[767,853],GPIO5:[767,775],GPIO6:[767,697],GPIO7:[300,697],GPIO8:[300,775],GPIO9:[300,853],GPIO10:[300,931],GPIO11:[300,1007],'3V3':[767,1161],GND:[767,1239],'5V':[767,1315]}},
 display:{crop:[345,135,1360,905],size:[2048,1152],pins:{BUSY:[400,493],RES:[470,493],'D/C':[400,562],CS:[470,562],SCL:[400,631],SDA:[470,631],GND:[400,700],VCC:[470,700]}},
 sensor:{crop:[225,900,360,615],pins:{GND:[280,970],OUT:[395,960],VCC:[500,950]}},
 blade:{crop:[475,695,265,345],pins:{'Lower left':[540,1010],'Lower right':[700,994]}},
 center:{crop:[300,590,320,760],pins:{Left:[355,1310],Right:[576,1310]}},
 power:{crop:[155,620,945,825],pins:{'VO+':[265,1370],'VO−':[575,1345],'B+':[730,1340],'B−':[1050,1325]}},
 switch:{crop:[500,765,310,250],pins:{Middle:[650,990],Right:[730,990],Left:[570,990]}}
};
const colors={VCC:'#ffb454',GND:'#b9c5d2',RES:'#ff7d9d','D/C':'#bb9aff',SCL:'#5cbbff',BUSY:'#ffe16a',SDA:'#55e0cf',CS:'#b7f36b'};
export const powerCircuit=[
 {source:'power',from:'VO+',target:'switch',to:'Right',color:'#ffb454'},
 {source:'switch',from:'Middle',target:'esp',to:'5V',color:'#ff6a76'},
 {source:'power',from:'VO−',target:'esp',to:'GND',color:colors.GND}
];
export const connections={
 display:[['VCC','3V3'],['GND','GND'],['RES','GPIO4'],['D/C','GPIO5'],['SCL','GPIO6'],['BUSY','GPIO7'],['SDA','GPIO8'],['CS','GPIO9']].map(([from,to])=>({from,to,color:colors[from]})),
 sensor:[{from:'VCC',to:'3V3',color:colors.VCC},{from:'GND',to:'GND',color:colors.GND},{from:'OUT',to:'GPIO1',color:'#55e0cf',via:'1 kΩ series'}],
 blade:[{from:'Lower left',to:'GPIO10',color:'#ff6a76'},{from:'Lower right',to:'GND',color:colors.GND}],
 center:[{from:'Left',to:'GPIO11',color:'#55df9a'},{from:'Right',to:'GND',color:colors.GND}],
 power:powerCircuit,
 switch:powerCircuit
};
export function mapPoint(photo,point,box){const[x,y,w,h]=photo.crop;const s=Math.min(box[2]/w,box[3]/h);return[box[0]+(box[2]-w*s)/2+(point[0]-x)*s,box[1]+(box[3]-h*s)/2+(point[1]-y)*s];}
export function drawContacts(id,container){
 const ns='http://www.w3.org/2000/svg';
 const el=(tag,attrs={},text)=>{const n=document.createElementNS(ns,tag);for(const[k,v]of Object.entries(attrs))n.setAttribute(k,v);if(text)n.textContent=text;return n;};
 container.replaceChildren();
 const svg=el('svg',{viewBox:'0 0 1000 550',role:'img','aria-label':`Physical contact wiring for ${id}`});
 svg.append(el('title',{},'Select a wire below to highlight both contacts. Gray wires are GND.'));
 const left=[60,95,290,350],right=[710,95,240,350];
 function photo(key,box){const p=photos[key], [x,y,w,h]=p.crop,s=Math.min(box[2]/w,box[3]/h),size=p.size||[1152,2048];
 const frame=el('svg',{x:box[0]+(box[2]-w*s)/2,y:box[1]+(box[3]-h*s)/2,width:w*s,height:h*s,viewBox:`${x} ${y} ${w} ${h}`,overflow:'hidden'});
 frame.append(el('image',{href:`./assets/${key}-original.jpg`,width:size[0],height:size[1]}));svg.append(frame);
 }
 if(id==='esp'){photo('esp',[370,80,250,365]);svg.append(el('text',{x:500,y:490,'text-anchor':'middle'},'Choose a peripheral to inspect its individual wires.'));container.append(svg);return;}
 const isPower=id==='power'||id==='switch';
 const boxes=isPower?{power:[40,100,290,320],switch:[415,150,170,160],esp:right}:{[id]:left,esp:right};
 for(const[key,box]of Object.entries(boxes))photo(key,box);
 svg.append(el('text',{x:205,y:55,'text-anchor':'middle'},isPower?'CHARGE + BOOST':id.toUpperCase()),el('text',{x:830,y:55,'text-anchor':'middle'},'ESP32-S3-Zero'));
 if(isPower)svg.append(el('text',{x:500,y:110,'text-anchor':'middle'},'POWER SWITCH'));
 const legend=document.createElement('div');legend.className='wire-legend';legend.setAttribute('role','group');legend.setAttribute('aria-label','Highlight an individual wire');
 const groups=[];
 for(const [i,c]of (connections[id]||[]).entries()){
 const source=c.source||id,target=c.target||'esp';
 const a=mapPoint(photos[source],photos[source].pins[c.from],boxes[source]),b=mapPoint(photos[target],photos[target].pins[c.to],boxes[target]),rail=410+i*29;
 const d=isPower?`M ${a[0]} ${a[1]} V ${365+i*29} H ${b[0]} V ${b[1]}`:`M ${a[0]} ${a[1]} H ${rail} V ${b[1]} H ${b[0]}`;
 const g=el('g',{'data-wire':`${c.from}-${c.to}`});g.append(el('path',{d,stroke:'#101211','stroke-width':7,fill:'none'}),el('path',{d,stroke:c.color,'stroke-width':3,fill:'none'}));
 for(const p of[a,b])g.append(el('circle',{cx:p[0],cy:p[1],r:6,fill:'#101211',stroke:c.color,'stroke-width':3}));
 svg.append(g);groups.push(g);
 const button=document.createElement('button');button.type='button';button.style.setProperty('--wire-color',c.color);button.textContent=`${isPower?source+' ':''}${c.from} → ${c.via?c.via+' → ':''}${isPower?target+' ':''}${c.to}`;button.setAttribute('aria-pressed','false');
 button.addEventListener('click',()=>{const already=button.getAttribute('aria-pressed')==='true';for(const [j,item]of groups.entries())item.setAttribute('opacity',already||i===j?'1':'.15');for(const item of legend.children)item.setAttribute('aria-pressed',String(!already&&item===button));});legend.append(button);
 if(c.via)svg.append(el('text',{x:rail+12,y:465,fill:c.color},c.via));
 }
 if(id==='power'||id==='switch'){
 svg.append(el('text',{x:500,y:478,'text-anchor':'middle'},'VO+ → right leg | middle leg → ESP 5V | left leg unused'),el('text',{x:500,y:507,'text-anchor':'middle'},'As marked in your reference. Set boost to 5.0 V; switch OFF for USB.'));
 }
 container.append(svg,legend);
}
