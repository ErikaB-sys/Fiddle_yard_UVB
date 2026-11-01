#pragma once

/**
 * @file FyWebPage.h
 * @brief Statische FY-Web-Demoseite mit masshaltigem SVG-Gleisbild.
 *
 * Nur im Browser simulierte Buehnenfahrt. Keine Motorbefehle!
 * HTML/CSS/SVG/JS liegen als ein PROGMEM-String im Flash.
 */
#include <Arduino.h>

static const char FY_WEB_PAGE[] PROGMEM = R"FYHTML(<!doctype html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>FY UVB – Web-Prüfstand</title>
<style>
:root{color-scheme:dark;--bg:#0c1b27;--panel:#152b3a;--line:#d1dce4;--muted:#9db1c0;--green:#83ed83;--aqua:#40d4e8;--edge:#426174}
*{box-sizing:border-box}
body{margin:0;background:linear-gradient(150deg,#122b3b,#091722);color:#edf5fa;font:16px system-ui,-apple-system,Segoe UI,sans-serif}
main{max-width:1190px;margin:0 auto;padding:22px 20px 35px}
header{display:flex;align-items:baseline;gap:15px;flex-wrap:wrap;margin-bottom:18px}
h1{font-size:clamp(23px,3vw,33px);margin:0}h2{font-size:17px;margin:0 0 10px}
.sub{color:var(--muted);font-size:13px}
.tag{background:#123a35;color:var(--green);border:1px solid #396f59;border-radius:7px;padding:5px 10px;font-weight:650}
.metrics{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:10px;margin-bottom:12px}
.metric,.card{background:var(--panel);border:1px solid var(--edge);border-radius:10px}
.metric{padding:11px 15px}.metric .cap{color:var(--muted);font-size:12px}
.metric .val{font-weight:700;font-size:19px;margin-top:3px}
.green{color:var(--green)}.cyan{color:var(--aqua)}
.card{padding:16px 14px}
.schematic{display:block;width:100%;height:auto;max-height:510px;background:#102635;border-radius:8px;user-select:none}
.tools{display:flex;align-items:center;flex-wrap:wrap;gap:9px;margin:14px 0 2px}
button{font:inherit;color:#eef7fb;background:#21445a;border:1px solid #52788d;border-radius:7px;padding:9px 13px;cursor:pointer}
button:hover{background:#2a5972}button:focus-visible,input:focus-visible{outline:2px solid var(--aqua);outline-offset:2px}
button.primary{background:#155f61;border-color:#39a9aa}
.tools label{font-size:13px;color:var(--muted)}
input[type=range]{width:min(260px,70vw);accent-color:var(--aqua)}
.foot{margin-top:14px;display:flex;justify-content:space-between;gap:12px;flex-wrap:wrap;color:var(--muted);font-size:13px}
.mini{font-variant-numeric:tabular-nums}
.rail{stroke:var(--line);stroke-width:3.7;stroke-linecap:round}
.entry{stroke:var(--green);stroke-width:5}
.guide{stroke:#6b90a6;stroke-width:1.4;stroke-dasharray:5 8;opacity:.75}
.frame{fill:#0c222e;stroke:#75e58a;stroke-width:3.4}
.trackname{fill:#edf5fa;font-size:14px;font-weight:600;text-anchor:middle;dominant-baseline:central}
.section{fill:#9ab0c0;font-size:15px;text-anchor:middle}
.index{fill:#b7c8d4;font-size:12px;text-anchor:middle}
.rail-active{stroke:var(--green);stroke-width:5}
.badge-bg{fill:#152f40;stroke:#65879a;stroke-width:1}
.active-bg{fill:#165f42;stroke:#8cf08b;stroke-width:1.5}
a{color:var(--aqua)}
@media(max-width:650px){main{padding:14px 10px}.metrics{grid-template-columns:repeat(2,minmax(0,1fr))}.metric .val{font-size:16px}.card{padding:8px}h2{font-size:14px}.tools{gap:6px}button{padding:8px 9px;font-size:13px}}
</style>
</head>
<body>
<main>
<header><h1>FY UVB · Web-Prüfstand</h1><span class="tag">SIMULATION</span><span class="sub">SVG-Gleisbild · keine Verbindung zur Motorsteuerung</span></header>
<section class="metrics" aria-label="Simulierter Zustand">
<div class="metric"><div class="cap">Betriebsart</div><div class="val">DEMO</div></div>
<div class="metric"><div class="cap">Einfahrt fluchtet mit</div><div id="entryLabel" class="val green">BG3</div></div>
<div class="metric"><div class="cap">BG1 gegenüber</div><div id="storageLabel" class="val cyan">AB7</div></div>
<div class="metric"><div class="cap">Bühne</div><div id="motorLabel" class="val">Stillstand</div></div>
</section>
<section class="card">
<h2>Schiebebühne · fünf SET_TRACK-Stellungen</h2>
<svg id="plan" class="schematic" viewBox="0 0 980 453" role="img" aria-label="Links drei gerade Gleise, Mitte fünf gemeinsam bewegliche Gleise, rechts neun gerade Abstellgleise. Alle Gleise haben identischen Abstand.">
<rect x="18" y="34" width="944" height="370" rx="10" fill="none" stroke="#34576c" stroke-width="1.5"/>
<g id="guides">
<line class="guide" x1="345" y1="34" x2="345" y2="405"/>
<line class="guide" x1="635" y1="34" x2="635" y2="405"/>
<text class="index" x="195" y="28">LINKS · 3</text>
<text class="index" x="490" y="28">BÜHNE · 5</text>
<text class="index" x="780" y="28">RECHTS · 9</text>
</g>
<g id="fixedLeft"></g>
<g id="fixedRight"></g>
<g id="bridge"></g>
<g id="labelsBottom">
<path d="M55 414v7 M345 414v7 M635 414v7 M925 414v7" stroke="#91aaba"/>
<path d="M55 418H345 M345 418H635 M635 418H925" stroke="#91aaba" opacity=".65"/>
<text class="section" x="200" y="443">1/3 · Einfahrt</text>
<text class="section" x="490" y="443">1/3 · Bühne</text>
<text class="section" x="780" y="443">1/3 · Abstellung</text>
</g>
</svg>
<div class="tools">
<button id="up" type="button">▲ Hoch</button>
<button id="down" type="button">▼ Runter</button>
<button id="demo" class="primary" type="button">▶ Demo-Fahrt</button>
<button id="stop" type="button">■ STOP</button>
<label for="position">SET_TRACK: <strong id="positionValue">3</strong> von 5</label>
<input id="position" type="range" min="1" max="5" value="3" step="1">
</div>
<div class="foot">
<span id="alignment">Einfahrt ↔ BG3 · BG1 ↔ AB7</span>
<span class="mini">5 Stellungen · 9 Abstellgleise · Gleisabstand 25 · Bühne 1/3 · <a href="/api/system">Speicher</a> · <a href="/api/status">Netzstatus</a></span>
</div>
</section>
</main>
<script>
'use strict';
// Geometrie: drei Drittel je 290 Einheiten. Neun Gleiszeilen y=72+25*(Zeile-1).
// Links: Zeilen 2,5,8; rechts: Zeilen 1..9; Bruecke: fuenf feste Gleise.
// SET_TRACK 1..5 positioniert genau BG1..BG5 an der Einfahrt (Zeile 5).
// BG1 liegt bei SET_TRACK t auf rechter Abstellgleiszeile (6-t) = AB(4+t).
// SET_TRACK 4 -> BG4 Einfahrt; SET_TRACK 1 -> BG4 AB2 / AB11.
// SET_TRACK 1 -> BG1 Einfahrt; SET_TRACK 5 -> BG1 AB9.
const $=id=>document.getElementById(id);
const ns='http://www.w3.org/2000/svg';
function el(tag,parent,attrs,text){
  const e=document.createElementNS(ns,tag);
  Object.keys(attrs||{}).forEach(k=>e.setAttribute(k,attrs[k]));
  if(text!==undefined)e.textContent=text;
  parent.appendChild(e);return e;
}
function rail(parent,x1,x2,y,cls){
  return el('line',parent,{x1:x1,x2:x2,y1:y,y2:y,'class':cls||'rail'});
}
function label(parent,x,y,name){
  el('rect',parent,{x:x-30,y:y-10,width:60,height:20,rx:5,'class':'badge-bg'});
  el('text',parent,{x:x,y:y,'class':'trackname'},name);
}
const left=$('fixedLeft'),right=$('fixedRight'),bridge=$('bridge');
[[2,'AB10'],[5,'Einfahrt'],[8,'AB11']].forEach(item=>{
  const y=72+25*(item[0]-1);
  rail(left,55,345,y,item[0]===5?'rail entry':'rail');
  el('text',left,{x:112,y:y-13,'class':'trackname'},item[1]);
});
for(let row=1;row<=9;row++){
  const y=72+25*(row-1);
  rail(right,635,925,y);
  label(right,886,y,'AB'+(10-row));
}
el('rect',bridge,{x:345,y:108,width:290,height:128,rx:8,'class':'frame'});
const rails=[],boxes=[],texts=[];
for(let n=1;n<=5;n++){
  const y=97+25*n;
  rails.push(rail(bridge,345,635,y));
  boxes.push(el('rect',bridge,{x:458,y:y-10,width:64,height:20,rx:5,'class':'badge-bg'}));
  texts.push(el('text',bridge,{x:490,y:y,'class':'trackname'},'BG'+n));
}
let offset=0,frame=null,timer=null,demoRunning=false,queue=[];
function show(){
  const track=3-offset/25;
  const rounded=Math.round(track);
  const aligned=Math.abs(track-rounded)<0.003&&rounded>=1&&rounded<=5;
  bridge.setAttribute('transform','translate(0,'+offset.toFixed(3)+')');
  const active=aligned?rounded:0;
  $('entryLabel').textContent=aligned?'BG'+active:'–';
  $('storageLabel').textContent=aligned?'AB'+(4+active):'–';
  $('positionValue').textContent=aligned?String(active):'zwischen';
  $('position').value=String(Math.max(1,Math.min(5,rounded)));
  $('alignment').textContent=aligned
    ? 'Einfahrt ↔ BG'+active+' · BG1 ↔ AB'+(4+active)
    : 'Zwischenposition · keine Gleisfluchtung';
  for(let i=0;i<5;i++){
    rails[i].setAttribute('class',i===active-1?'rail rail-active':'rail');
    boxes[i].setAttribute('class',i===active-1?'active-bg':'badge-bg');
    texts[i].setAttribute('fill',i===active-1?'#bcffcb':'#edf5fa');
  }
  $('motorLabel').textContent=frame!==null?'in Fahrt':(demoRunning?'Demo aktiv':'Stillstand');
  $('motorLabel').className=frame!==null?'val cyan':'val';
}
function stop(){
  if(frame!==null){cancelAnimationFrame(frame);frame=null;}
  if(timer!==null){clearTimeout(timer);timer=null;}
  queue=[];demoRunning=false;show();
}
function moveTo(target,done){
  target=Math.max(1,Math.min(5,Math.round(target)));
  const from=offset,to=(3-target)*25,start=performance.now();
  const duration=Math.max(300,Math.abs(to-from)*13);
  if(Math.abs(to-from)<0.1){offset=to;show();if(done)done();return;}
  function tick(t){
    const f=Math.min(1,(t-start)/duration);
    const ease=f<.5?2*f*f:1-Math.pow(-2*f+2,2)/2;
    offset=from+(to-from)*ease;
    if(f<1){frame=requestAnimationFrame(tick);show();}
    else{frame=null;offset=to;show();if(done)done();}
  }
  frame=requestAnimationFrame(tick);
  show();
}
function manual(target){stop();moveTo(target);}
$('up').addEventListener('click',()=>manual(Math.round(3-offset/25)+1));
$('down').addEventListener('click',()=>manual(Math.round(3-offset/25)-1));
$('position').addEventListener('input',()=>manual(Number($('position').value)));
$('stop').addEventListener('click',stop);
function nextDemo(){
  if(!demoRunning)return;
  if(!queue.length){demoRunning=false;show();return;}
  moveTo(queue.shift(),()=>{if(demoRunning)timer=setTimeout(nextDemo,650);});
}
$('demo').addEventListener('click',()=>{
  stop();demoRunning=true;queue=[1,5,3];show();nextDemo();
});
show();
</script>
</body>
</html>)FYHTML";
