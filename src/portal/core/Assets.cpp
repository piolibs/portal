/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "Render.h"

namespace portal {

// One column for phones with the menu at the bottom, follows the system
// light/dark theme. Only classes: the CSP blocks inline styles.
const char STYLE_CSS[] PROGMEM =
    ":root{color-scheme:light dark;--bg:#f3f4f6;--fg:#1f2328;--mu:#57606a;--card:#fff;--line:#d0d7de;"
    "--ac:#0969da;--bad:#cf222e;--okb:#dafbe1;--okf:#116329;--dot:#1a7f37;--wb:#fff8c5;--wf:#4d2d00;--sel:#ddf4ff;--pill:#eaeef2}"
    "@media(prefers-color-scheme:dark){:root{--bg:#0d1117;--fg:#e6edf3;--mu:#9198a1;--card:#161b22;--line:#30363d;"
    "--ac:#4493f8;--bad:#f85149;--okb:#12361f;--okf:#56d364;--dot:#3fb950;--wb:#3b2e0a;--wf:#f2cc60;--sel:#0c2d6b;--pill:#30363d}}"
    "*{box-sizing:border-box}[hidden]{display:none!important}"
    "body{margin:0;background:var(--bg);color:var(--fg);font:16px/1.4 system-ui,-apple-system,\"Segoe UI\",Roboto,sans-serif;"
    "-webkit-text-size-adjust:100%}"
    "body.nav{padding-bottom:calc(76px + env(safe-area-inset-bottom))}"
    "a{color:var(--ac)}"
    "header{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:14px 16px;"
    "background:var(--card);border-bottom:1px solid var(--line)}"
    "h1{margin:0;font-size:17px;font-weight:700}"
    "header small{display:block;margin-top:2px;font-size:13px;color:var(--mu)}"
    ".dev{font-size:13px;color:var(--mu)}"
    ".pill{display:flex;align-items:center;gap:6px;padding:4px 10px;border-radius:999px;background:var(--pill);"
    "font-size:13px;font-weight:600;white-space:nowrap}"
    ".pill.ok{background:var(--okb);color:var(--okf)}"
    ".pill.ok circle{fill:var(--dot)}"
    "main{max-width:560px;margin:0 auto;padding:12px 16px 16px;display:flex;flex-direction:column;gap:10px}"
    "form{display:flex;flex-direction:column;gap:10px}"
    "h2{margin:6px 0 0;font-size:13px;font-weight:600;letter-spacing:.04em;text-transform:uppercase;color:var(--mu)}"
    ".card,.list{background:var(--card);border:1px solid var(--line);border-radius:12px}"
    ".card{display:flex;flex-direction:column;gap:2px;padding:12px 16px}"
    ".l{font-size:14px;color:var(--mu)}"
    ".st{flex-direction:row;align-items:center;gap:12px;padding:12px 14px;font-size:15px}"
    ".st div{display:flex;flex-direction:column;gap:1px}.st .l{font-size:13px}"
    ".st svg{flex-shrink:0;stroke:var(--dot)}.st.off svg{stroke:var(--mu)}"
    ".v{font-size:32px;font-weight:600;font-variant-numeric:tabular-nums;line-height:1.15;overflow-wrap:anywhere}"
    ".v small{font-size:16px;font-weight:400;color:var(--mu)}"
    ".list{padding:0 14px;font-size:15px}"
    ".row{display:flex;justify-content:space-between;gap:12px;padding:10px 0;border-top:1px solid var(--line)}"
    ".row:first-child{border-top:0}"
    ".row span:first-child{color:var(--mu)}"
    ".row span+span{text-align:right;overflow-wrap:anywhere;font-variant-numeric:tabular-nums}"
    ".note{margin:2px 0 0;font-size:13px;color:var(--mu);text-align:center}"
    ".f{display:flex;flex-direction:column;gap:4px}"
    "label{font-size:14px;color:var(--mu)}"
    ".in{display:flex;align-items:center;gap:8px;height:48px;padding:0 14px;background:var(--card);"
    "border:1px solid var(--line);border-radius:10px}"
    ".in:focus-within{border:2px solid var(--ac);padding:0 13px}"
    ".in.bad{border:2px solid var(--bad);padding:0 13px}"
    ".in input{flex:1;min-width:0;height:100%;border:0;outline:0;background:none;color:inherit;font:inherit;"
    "font-size:17px;font-variant-numeric:tabular-nums}"
    ".in span{font-size:15px;color:var(--mu)}"
    ".err{font-size:13px;color:var(--bad)}"
    "button{width:100%;height:50px;margin-top:6px;border:0;border-radius:12px;background:var(--ac);color:#fff;"
    "font:inherit;font-size:17px;font-weight:600}"
    "button:disabled{opacity:.6}"
    "button.danger{background:none;border:2px solid var(--bad);color:var(--bad)}"
    "button.action{display:flex;align-items:center;justify-content:center;gap:8px;background:none;"
    "border:2px solid var(--ac);color:var(--ac)}"
    "button.action:disabled{background:var(--bg);border-color:var(--line);color:var(--mu);opacity:1}"
    ".msg{display:flex;align-items:center;gap:8px;margin:0;padding:10px 12px;border-radius:10px;"
    "background:var(--okb);color:var(--okf);font-size:15px}"
    ".msg.bad{background:none;border:2px solid var(--bad);color:var(--bad)}"
    ".warn{display:flex;gap:10px;padding:12px 14px;border-radius:12px;background:var(--wb);color:var(--wf);font-size:14px}"
    ".warn svg{flex-shrink:0}"
    ".nh{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-top:2px}"
    ".nh h2{margin:0}.nh h2 span{font-weight:400;letter-spacing:0;text-transform:none}"
    "button.scan{display:flex;align-items:center;gap:6px;width:auto;height:36px;margin:0;padding:0 14px;"
    "border:1px solid var(--line);border-radius:999px;background:var(--card);color:var(--ac);font-size:14px}"
    "button.scan:disabled{background:var(--bg);color:var(--mu);opacity:1}"
    ".empty{align-items:center;gap:6px;padding:22px 16px;border:1px dashed var(--mu);text-align:center}"
    ".empty b,.scanning b{font-size:15px}"
    ".scanning{gap:8px;padding:14px 16px}.scanning progress{height:6px}.scanning .l{font-size:13px}"
    ".nets{padding:0;overflow:hidden}"
    ".nets button{display:flex;align-items:center;justify-content:space-between;gap:12px;height:auto;min-height:48px;"
    "margin:0;padding:0 14px;border-top:1px solid var(--line);border-radius:0;background:none;color:var(--fg);"
    "font-size:16px;font-weight:400;text-align:left}"
    ".nets button:first-child{border-top:0}"
    ".nets button.on{background:var(--sel);font-weight:600}"
    ".nets i{display:flex;align-items:center;gap:8px;font-style:normal;font-size:13px;color:var(--mu)}"
    ".sig rect{fill:var(--line)}.sig rect.on{fill:var(--mu)}"
    ".nets .on .sig rect.on{fill:var(--ac)}"
    ".drop{position:relative;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:6px;"
    "min-height:150px;padding:16px;background:var(--card);border:2px dashed var(--mu);border-radius:12px;"
    "text-align:center;color:var(--ac);font-size:17px;font-weight:600}"
    ".drop svg{stroke:var(--ac)}"
    ".drop small{font-size:14px;font-weight:400;color:var(--mu)}"
    ".drop input{position:absolute;inset:0;opacity:0}"
    "progress{width:100%;height:8px;accent-color:var(--ac)}"
    "nav{position:fixed;left:0;right:0;bottom:0;display:grid;grid-template-columns:repeat(5,minmax(0,1fr));"
    "padding:6px 4px calc(6px + env(safe-area-inset-bottom));background:var(--card);border-top:1px solid var(--line)}"
    "nav a{display:flex;flex-direction:column;align-items:center;justify-content:center;gap:3px;min-height:48px;"
    "color:var(--mu);text-decoration:none;font-size:12px}"
    "nav a.on{color:var(--ac);font-weight:600}";

// - refreshes the dashboard values, also when the page comes back to the screen;
// - a tap on a network fills the WiFi form;
// - the firmware upload shows its progress
const char APP_JS[] PROGMEM =
    "(function(){var d=document,$=function(i){return d.getElementById(i)};"
    "function u(){fetch('/api/values',{cache:'no-store'}).then(function(r){return r.json()})"
    ".then(function(v){for(var k in v){var e=$('v-'+k);if(e)e.textContent=v[k]}}).catch(function(){})}"
    "if(d.querySelector('[id^=\"v-\"]')){setInterval(u,30000);"
    "d.addEventListener('visibilitychange',function(){if(!d.hidden)u()})}"
    "d.querySelectorAll('[data-s]').forEach(function(b){b.addEventListener('click',function(){"
    "d.querySelectorAll('.nets .on').forEach(function(o){o.classList.remove('on')});b.classList.add('on');"
    "$('s').value=b.getAttribute('data-s');$('p').value='';$('p').focus()})});"
    // Dashboard actions: busy while the device runs one (reset when the
    // page comes back from the history), the time of the done message, and
    // a reload doesn't show it again
    "d.querySelectorAll('form.act').forEach(function(f){var b=f.querySelector('button'),l=b.lastChild,t=l.textContent;"
    "f.addEventListener('submit',function(){b.disabled=true;l.textContent=b.getAttribute('data-busy')});"
    "window.addEventListener('pageshow',function(e){if(e.persisted){b.disabled=false;l.textContent=t}})});"
    "var a=d.querySelector('[data-at]');if(a){a.lastChild.textContent+=' at '+new Date().toTimeString().slice(0,8);"
    "history.replaceState(null,'','/')}"
    "var f=$('fw'),i=$('f');if(!f)return;"
    "i.addEventListener('change',function(){var x=i.files[0];if(x)$('fn').textContent=x.name+', '+x.size+' B'});"
    "f.addEventListener('submit',function(e){if(!i.files[0]||!window.FormData)return;e.preventDefault();"
    "var x=new XMLHttpRequest(),p=$('pr'),b=f.querySelector('button');p.hidden=false;b.disabled=true;"
    "x.upload.onprogress=function(v){if(v.lengthComputable)p.value=v.loaded*100/v.total};"
    "x.onload=function(){d.open();d.write(x.responseText);d.close()};"
    "x.onerror=function(){p.hidden=true;b.disabled=false;$('fn').textContent='Upload failed, try again'};"
    "x.open('POST',f.action);x.send(new FormData(f))})"
    "})();";

} // namespace portal
