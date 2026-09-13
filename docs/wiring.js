import { components, photoCrops } from './wiring-data.mjs';
import { drawOverview, accents, rowColor } from './overview.mjs';
const grid = document.querySelector('#components');
const detail = document.querySelector('#connection-detail');
export function selectComponent(id) {
  const part = components.find(item => item.id === id);
  if (!part) return;
  for (const button of grid.querySelectorAll('button')) button.setAttribute('aria-pressed', String(button.dataset.component === id));
  for (const path of document.querySelectorAll('[data-link]')) path.classList.toggle('active', id === 'esp' || path.dataset.link === id);
  const heading = document.createElement('h2'); heading.id = 'component-title'; heading.textContent = part.name;
  heading.style.color = accents[id];
  const tag = document.createElement('p'); tag.className = 'detail-tag'; tag.textContent = part.tag;
  const table = document.createElement('table'); table.className = 'pin-table';
  const head = table.createTHead().insertRow();
  for (const label of ['Terminal', 'Connect to']) { const th = document.createElement('th'); th.scope = 'col'; th.textContent = label; head.append(th); }
  const body = table.createTBody();
  for (const pair of part.pins) { const row = body.insertRow(); for (const value of pair) { const cell=row.insertCell();cell.textContent=value;cell.style.color=rowColor(id,pair); } }
  const note = document.createElement('p'); note.className = 'connection-note'; note.textContent = part.note;
  detail.replaceChildren(heading, tag, table, note);
  const contactView = document.querySelector('#contact-view');
  if (contactView) drawOverview(id, contactView, selectComponent);
}
for (const part of components) {
  const button = document.createElement('button'); button.type = 'button'; button.dataset.component = part.id; button.className = `component component-${part.id}`;
  button.setAttribute('aria-controls', 'connection-detail'); button.setAttribute('aria-pressed', 'false');
  const photo = document.createElement('span'); photo.className = 'component-photo'; photo.setAttribute('aria-hidden', 'true');
  const [x,y,w,h] = photoCrops[part.id];
  photo.style.width = `${52*w/h}px`;
  photo.style.aspectRatio = `${w} / ${h}`;
  photo.style.backgroundSize = `${1536/w*100}% ${1024/h*100}%`;
  photo.style.backgroundPosition = `${x/(1536-w)*100}% ${y/(1024-h)*100}%`;
  const label = document.createElement('span'); label.className = 'component-label'; label.textContent = part.name;
  label.style.color = accents[part.id];
  const tag = document.createElement('span'); tag.className = 'component-tag'; tag.textContent = part.tag;
  button.append(photo,label,tag); button.addEventListener('click', () => selectComponent(part.id)); grid.append(button);
}
selectComponent('esp');
