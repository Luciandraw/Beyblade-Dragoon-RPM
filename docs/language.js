import { polish } from './translations.mjs';

export function translate(text, language) {
  if(language!=='pl')return text;
  const key=text.trim();let result=polish[key];
  if(!result){
    let match;
    if((match=key.match(/^Selected · (.+)\. All (\d+) parts remain visible\.$/)))result=`Wybrano · ${match[1]}. Wszystkie części (${match[2]}) pozostają widoczne.`;
    else if((match=key.match(/^Ready · (\d+) parts\. Click a part to highlight it\.$/)))result=`Gotowe · ${match[1]} części. Kliknij część, aby ją podświetlić.`;
    else if((match=key.match(/^Installer unavailable\. (.+) Check your connection and reload this page\.$/)))result=`Instalator niedostępny. ${translate(match[1],'pl')} Sprawdź połączenie i odśwież stronę.`;
    else if((match=key.match(/^3D viewer unavailable\. (.+) Check your connection and WebGL support, then retry\.$/)))result=`Podgląd 3D niedostępny. ${translate(match[1],'pl')} Sprawdź połączenie i obsługę WebGL, a następnie spróbuj ponownie.`;
    else if((match=key.match(/^File unavailable \(HTTP (\d+)\)\.$/)))result=`Plik niedostępny (HTTP ${match[1]}).`;
    else if((match=key.match(/^Model download failed \(HTTP (\d+)\)\.$/)))result=`Nie udało się pobrać modelu (HTTP ${match[1]}).`;
    else if(/^(power|esp|display|sensor|blade|center|switch) /.test(key)&&key.includes('→')){
      result=key.replace(/\b(power|esp|display|sensor|blade|center|switch)\b/g,s=>({power:'zasilanie',esp:'ESP',display:'ekran',sensor:'czujnik',blade:'BeySense',center:'przycisk',switch:'wyłącznik'}[s])).replace('common GND','wspólna masa GND').replace('Lower left','Dolny lewy').replace('Lower right','Dolny prawy').replace('Middle','Środkowy').replace('Right','Prawy').replace('Left','Lewy');
    }
  }
  return result?text.replace(key,result):text;
}

// Translate presentation nodes only; do not alter connection data, GPIO keys,
// firmware checks or third-party shadow DOM. Preserve source text for switching back.
if(typeof document!=='undefined'){
 let language='en';try{if(localStorage.getItem('beyblade-language')==='pl')language='pl';}catch{}
 const originals=new WeakMap();
 const picker=document.createElement('select');picker.id='site-language';picker.setAttribute('aria-label','Language / Język');
 for(const [value,label]of [['en','English'],['pl','Polski']]){const option=document.createElement('option');option.value=value;option.textContent=label;picker.append(option);}
 picker.value=language;document.querySelector('header').append(picker);
 function replace(node,slot,current,set){
   let cache=originals.get(node);if(!cache){cache={};originals.set(node,cache);}
   let record=cache[slot];if(!record||current!==record.output)record=cache[slot]={source:current,output:current};
   const next=translate(record.source,language);record.output=next;if(next!==current)set(next);
 }
 function apply(){
   observer.disconnect();document.documentElement.lang=language;
   const walker=document.createTreeWalker(document.documentElement,NodeFilter.SHOW_TEXT);
   while(walker.nextNode()){
     const node=walker.currentNode;
     if(node.parentElement?.closest('script,style,noscript,select,#parts,.brand'))continue;
     replace(node,'text',node.nodeValue,v=>{node.nodeValue=v;});
   }
   for(const element of document.querySelectorAll('[aria-label],[title],meta[name="description"]')){
     if(element===picker)continue;
     for(const attr of ['aria-label','title',...(element.matches('meta')?['content']:[])])if(element.hasAttribute(attr))replace(element,attr,element.getAttribute(attr),v=>element.setAttribute(attr,v));
   }
   observer.observe(document.documentElement,{subtree:true,childList:true,characterData:true,attributes:true,attributeFilter:['aria-label','title']});
 }
 const observer=new MutationObserver(apply);
 picker.addEventListener('change',()=>{language=picker.value;try{localStorage.setItem('beyblade-language',language);}catch{}apply();});
 apply();
}
