import { explosionOffset } from './explosion-layout.mjs?v=20260914-26parts';
const status=document.querySelector('#model-status'),load=document.querySelector('#load-model');
let dispose=()=>{};
load.addEventListener('click',async()=>{
 load.disabled=true;status.dataset.error='false';status.textContent='Loading 3D viewer…';
 let renderer,controls,resizeObserver;
 try{
  const [THREE,{GLTFLoader},{OrbitControls}]=await Promise.all([import('three'),import('three/addons/loaders/GLTFLoader.js'),import('three/addons/controls/OrbitControls.js')]);
  const host=document.querySelector('#viewport');
  renderer=new THREE.WebGLRenderer({antialias:true,alpha:true});renderer.setPixelRatio(Math.min(devicePixelRatio,2));
  renderer.setClearColor(0,0);renderer.outputColorSpace=THREE.SRGBColorSpace;renderer.toneMapping=THREE.ACESFilmicToneMapping;
  renderer.domElement.setAttribute('aria-label','Launcher V2: drag to rotate, scroll to zoom. Arrow keys pan.');renderer.domElement.tabIndex=0;
  host.replaceChildren(renderer.domElement);
  const scene=new THREE.Scene(),camera=new THREE.PerspectiveCamera(40,1,.001,10000);
  scene.add(new THREE.HemisphereLight(0xffffff,0x53604d,2.5));
  for(const[position,intensity]of [[[3,5,4],3],[[-4,1,-2],2]]){const light=new THREE.DirectionalLight(0xffffff,intensity);light.position.set(...position);scene.add(light);}
  controls=new OrbitControls(camera,renderer.domElement);controls.listenToKeyEvents(renderer.domElement);
  const render=()=>renderer.render(scene,camera);controls.addEventListener('change',render);
  status.textContent='Downloading LauncherV2.glb…';
  const response=await fetch('./models/LauncherV2.glb?v=20260914-26parts',{signal:AbortSignal.timeout(90000)});
  if(!response.ok)throw new Error(`Model download failed (HTTP ${response.status}).`);
  const bytes=await response.arrayBuffer();status.textContent='Preparing model geometry…';
  const gltf=await new GLTFLoader().parseAsync(bytes,new URL('./models/',location.href).href);
  const model=gltf.scene?.children.length?gltf.scene:gltf.scenes.find(s=>s.children.length);
  if(!model)throw new Error('The file contains no populated scene.');
  scene.add(model);model.updateMatrixWorld(true);
  const bounds=new THREE.Box3().setFromObject(model),sphere=bounds.getBoundingSphere(new THREE.Sphere());
  if(!Number.isFinite(sphere.radius)||sphere.radius<=0)throw new Error('Model dimensions are invalid.');
  const center=sphere.center,radius=sphere.radius;
  // Palette-tinted fill from below the assembly (world -Y), independent of camera.
  // Directional fill stays even when the assembly layers are separated.
  const lowerFill=new THREE.DirectionalLight(0xc4f866,1.08);
  lowerFill.position.copy(center).add(new THREE.Vector3(.25,-2,.35).multiplyScalar(radius));
  lowerFill.target.position.copy(center);
  scene.add(lowerFill,lowerFill.target);
  camera.near=radius/1000;camera.far=radius*100;controls.minDistance=radius*.15;controls.maxDistance=radius*20;
  function size(){const w=host.clientWidth,h=host.clientHeight;renderer.setSize(w,h,false);camera.aspect=w/h;camera.updateProjectionMatrix();render();}
  function reset(){size();model.updateMatrixWorld(true);const current=new THREE.Box3().setFromObject(model).getBoundingSphere(new THREE.Sphere());const vertical=THREE.MathUtils.degToRad(camera.fov/2),horizontal=Math.atan(Math.tan(vertical)*camera.aspect);const distance=current.radius/Math.sin(Math.min(vertical,horizontal))*1.15;controls.target.copy(current.center);camera.position.copy(current.center).add(new THREE.Vector3(1,.65,1).normalize().multiplyScalar(distance));controls.update();render();}
  reset();resizeObserver=new ResizeObserver(size);resizeObserver.observe(host);
  const parts=document.querySelector('#parts');parts.replaceChildren();const entries=[];
  // Keep X/Z alignment and all rotations: separate only along world Y (thickness).
  const owners=new Map();
  for(const object of model.children){
    const button=document.createElement('button');button.type='button';button.textContent=object.name||'Unnamed part';button.setAttribute('aria-pressed','false');
    const direction=new THREE.Vector3(...explosionOffset(object.name,radius*.22));
    const origin=object.position.clone(),worldOrigin=object.getWorldPosition(new THREE.Vector3());
    const offset=object.parent.worldToLocal(worldOrigin.add(direction)).sub(origin);
    const materials=[],meshes=[],edges=[];
    object.traverse(mesh=>{if(!mesh.isMesh)return;owners.set(mesh,object);meshes.push(mesh);
      const clones=(Array.isArray(mesh.material)?mesh.material:[mesh.material]).map(material=>{
        const copy=material.clone();materials.push({material:copy,color:copy.color?.clone(),emissive:copy.emissive?.clone()});return copy;
      });mesh.material=Array.isArray(mesh.material)?clones:clones[0];
    });
    button.onclick=()=>select(object);parts.append(button);entries.push({object,button,origin,offset,materials,meshes,edges});
  }
  let selectedPart=null,wireframe=false;
  function select(selected){
    selectedPart=selected;
    for(const entry of entries){const active=entry.object===selected;entry.button.setAttribute('aria-pressed',String(active));
      // Feature edges only: discard coplanar triangulation and shallow facets.
      // Build once on demand; parent each overlay to its mesh so explosion follows it.
      if(wireframe&&!entry.edges.length){
        for(const mesh of entry.meshes){
          const lines=new THREE.LineSegments(new THREE.EdgesGeometry(mesh.geometry,30),new THREE.LineBasicMaterial({color:0xa7cd65,toneMapped:false,depthTest:true,depthWrite:false}));
          lines.renderOrder=1;lines.raycast=()=>{};mesh.add(lines);entry.edges.push(lines);
        }
      }
      for(const lines of entry.edges){lines.visible=wireframe;lines.material.color.setHex(active?0x74eaff:0xa7cd65);}
      for(const {material,color,emissive} of entry.materials){
        // Opaque faces occlude rear/internal edges; offset prevents surface z-fighting.
        material.wireframe=false;material.polygonOffset=wireframe;
        material.polygonOffsetFactor=1;material.polygonOffsetUnits=1;
        if(color)material.color.copy(wireframe?new THREE.Color(active?0x12303a:0x101b13):active?new THREE.Color(0xc4f866):color);
        if(emissive)material.emissive.copy(wireframe?new THREE.Color(0x000000):active?new THREE.Color(0x304b08):emissive);
      }
    }
    status.textContent=selected?`Selected · ${selected.name}. All ${entries.length} parts remain visible.`:`Ready · ${entries.length} parts. Click a part to highlight it.`;render();
  }
  const raycaster=new THREE.Raycaster(),pointer=new THREE.Vector2();let press=null;
  renderer.domElement.addEventListener('pointerdown',event=>{press=event.isPrimary&&event.button===0?{x:event.clientX,y:event.clientY,id:event.pointerId}:null;});
  renderer.domElement.addEventListener('pointercancel',()=>{press=null;});
  renderer.domElement.addEventListener('pointerup',event=>{
    const start=press;press=null;if(!start||start.id!==event.pointerId||Math.hypot(event.clientX-start.x,event.clientY-start.y)>5)return;
    const rect=renderer.domElement.getBoundingClientRect();pointer.set((event.clientX-rect.left)/rect.width*2-1,-(event.clientY-rect.top)/rect.height*2+1);
    model.updateMatrixWorld(true);raycaster.setFromCamera(pointer,camera);const hit=raycaster.intersectObject(model,true)[0];select(hit?owners.get(hit.object):null);
  });
  const slider=document.querySelector('#explosion'),value=document.querySelector('#explosion-value');
  let amount=0,target=0,frame=0,fitAfter=false;
  function animate(){
    amount=matchMedia('(prefers-reduced-motion: reduce)').matches?target:amount+(target-amount)*.18;
    if(Math.abs(target-amount)<.001)amount=target;
    for(const entry of entries)entry.object.position.copy(entry.origin).addScaledVector(entry.offset,amount);
    model.updateMatrixWorld(true);render();
    if(amount!==target)frame=requestAnimationFrame(animate);else{frame=0;if(fitAfter){fitAfter=false;reset();}}
  }
  function separate(percent,fit=false){target=Number(percent)/100;slider.value=percent;value.textContent=`${percent}%`;fitAfter=fit;if(!frame)frame=requestAnimationFrame(animate);}
  slider.disabled=false;slider.oninput=()=>separate(slider.value);
  const wireframeButton=document.querySelector('#wireframe-mode');wireframeButton.disabled=false;
  wireframeButton.onclick=()=>{
    wireframe=!wireframe;wireframeButton.setAttribute('aria-pressed',String(wireframe));
    host.classList.toggle('wireframe-view',wireframe);select(selectedPart);
  };
  for(const [id,handler] of [['reset-camera',reset],['explode-model',()=>separate(100,true)],['assemble-model',()=>separate(0,true)],['clear-selection',()=>select(null)]]){const button=document.getElementById(id);button.disabled=false;button.onclick=handler;}
  select(null);load.hidden=true;
  renderer.domElement.addEventListener('webglcontextlost',event=>{event.preventDefault();status.dataset.error='true';status.textContent='3D graphics context lost. Reload this page to restart the viewer.';});
  dispose=()=>{cancelAnimationFrame(frame);resizeObserver.disconnect();controls.dispose();scene.traverse(o=>{if(o.isMesh||o.isLineSegments){o.geometry.dispose();for(const m of Array.isArray(o.material)?o.material:[o.material])m.dispose();}});renderer.dispose();};
 }catch(error){resizeObserver?.disconnect();controls?.dispose();renderer?.dispose();document.querySelector('#viewport').replaceChildren();status.dataset.error='true';status.textContent=`3D viewer unavailable. ${error.message} Check your connection and WebGL support, then retry.`;load.disabled=false;}
});
window.addEventListener('pagehide',event=>{if(!event.persisted)dispose();});
