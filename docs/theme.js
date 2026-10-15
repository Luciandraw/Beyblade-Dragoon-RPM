// Light/dark theme toggle. Without a saved choice the page follows the system
// setting; the inline snippet in each page's <head> applies a saved choice
// before first paint so there is no flash.
const root = document.documentElement;
const media = matchMedia('(prefers-color-scheme: dark)');
const current = () => root.dataset.theme || (media.matches ? 'dark' : 'light');

const button = document.createElement('button');
button.type = 'button';
button.id = 'site-theme';
button.setAttribute('aria-label', 'Switch theme');
function sync() {
  const dark = current() === 'dark';
  button.textContent = dark ? '☀' : '☾';
  button.setAttribute('aria-pressed', String(dark));
}
button.addEventListener('click', () => {
  root.dataset.theme = current() === 'dark' ? 'light' : 'dark';
  try { localStorage.setItem('beyblade-theme', root.dataset.theme); } catch { /* Storage may be blocked. */ }
  sync();
});
media.addEventListener('change', sync);
document.querySelector('header')?.append(button);
sync();
