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
.console{display:grid;grid-template-columns:minmax(0,1.4fr) minmax(245px,1fr);gap:16px;margin-bottom:14px}
.console-side{display:flex;flex-direction:column;gap:10px}
.console-part{background:#0e2533;border:1px solid #416175;border-radius:10px;padding:12px}
.console-heading{margin:0 0 7px;color:#d8e8ee;font-weight:650;font-size:14px}
.dial{width:100%;height:auto;max-height:260px;display:block;touch-action:manipulation;user-select:none}
.dial-arc{fill:none;stroke:#ad956b;stroke-width:7;stroke-linecap:round}
.dial-tick{stroke:#a8b3b7;stroke-width:3}
.dial-position{cursor:pointer}
.dial-position:focus-visible{outline:none}
.dial-position:focus-visible circle{stroke:#7fe9fb;stroke-width:4}
.dial-position circle{fill:#223d4b;stroke:#8e9ca0;stroke-width:2}
.dial-position.selected circle{fill:#94764a;stroke:#f0cc86;stroke-width:3}
.dial-position text{fill:#e7f0f3;font-size:13px;font-weight:700;text-anchor:middle;dominant-baseline:central;pointer-events:none}
.dial-position.selected text{fill:#fff3ce}
.dial-lever{stroke:#d9bd82;stroke-width:10;stroke-linecap:round}
.dial-hub{fill:#b38f56;stroke:#f3d298;stroke-width:3}
.dial-center{font-size:14px;font-weight:bold;fill:#f1d8a7;text-anchor:middle}
.lamp-row{display:flex;align-items:center;gap:12px;min-height:33px;font-size:14px}
.lamp{width:20px;height:20px;flex:0 0 20px;border-radius:50%;background:#283039;border:2px solid #76818b;box-shadow:inset 0 2px 5px #10151c}
.lamp.green{background:#70ea85;border-color:#bdf7c2;box-shadow:0 0 10px #54ce70,inset 0 0 4px #eefedc}
.lamp.red{background:#f04a4b;border-color:#ffc0a3;box-shadow:0 0 10px #e83b44,inset 0 0 4px #ffe0d9}
.lamp.blink{animation:blink .7s steps(1,end) infinite}
@keyframes blink{50%{opacity:.18}}
@media(prefers-reduced-motion:reduce){.lamp.blink{animation:none}}
.console-actions{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:9px}
.console-actions button{min-height:46px;font-weight:700}
#go{background:#1c5e45;border-color:#5fb98d}#go:disabled{opacity:.45;cursor:not-allowed}
#reference{background:#544e35;border-color:#a6a073}
.console-info{font-size:12px;color:var(--muted);margin-top:7px}

.green{color:var(--green)}.cyan{color:var(--aqua)}
.card{padding:16px 14px}
.schematic{display:block;width:100%;height:auto;max-height:510px;background:#102635;border-radius:8px;user-select:none}
.tools{display:flex;align-items:center;flex-wrap:wrap;gap:9px;margin:14px 0 2px}
button{font:inherit;color:#eef7fb;background:#21445a;border:1px solid #52788d;border-radius:7px;padding:9px 13px;cursor:pointer}
button:hover{background:#2a5972}button:focus-visible,input:focus-visible{outline:2px solid var(--aqua);outline-offset:2px}
#stop{background:#69272f;border-color:#b96870;font-weight:750;min-width:144px}
#stop:hover{background:#81313b}
.tools label{font-size:13px;color:var(--muted)}

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
@media(max-width:650px){main{padding:14px 10px}.metrics{grid-template-columns:repeat(2,minmax(0,1fr))}.metric .val{font-size:16px}.card{padding:8px}h2{font-size:14px}.tools{gap:6px}button{padding:8px 9px;font-size:13px}.console{grid-template-columns:1fr;gap:8px}.console-side{display:grid;grid-template-columns:1fr 1fr;align-items:start;gap:8px}.console-actions{grid-template-columns:1fr}.console-actions button{min-height:44px}}
</style>
</head>
<body>
<main>
<header><h1>FY UVB · Web-Prüfstand</h1><span class="tag">SIMULATION</span><span class="sub">SVG-Gleisbild · keine Verbindung zur Motorsteuerung</span></header>
<section class="console" aria-label="FY-Bedienpult">
<div class="console-part">
<div class="console-heading">Gleiswahl · historischer Fahrgeber</div>
<svg id="selector" class="dial" viewBox="0 0 380 230" role="group" aria-label="Brückengleis BG1 bis BG5 vorwählen">
<path class="dial-arc" d="M65 190 A125 125 0 0 1 315 190"/>
<g id="dialPositions"></g>
<line id="dialLever" class="dial-lever" x1="190" y1="190" x2="190" y2="105"/>
<circle class="dial-hub" cx="190" cy="190" r="18"/>
<circle cx="190" cy="190" r="6" fill="#2c3540"/>
<text id="selectedLabel" class="dial-center" x="190" y="224">BG3 gewählt</text>
</svg>
<div class="console-info">Erst Gleis vorwählen, dann FAHRT drücken. Simulation ohne Motoranbindung.</div>
</div>
<div class="console-side">
<div class="console-part">
<div class="console-heading">Kontrolllampen</div>
<div class="lamp-row"><span id="lampLocal" class="lamp green" aria-hidden="true"></span><span>LOCAL · Simulation</span></div>
<div class="lamp-row"><span id="lampJmri" class="lamp" aria-hidden="true"></span><span>JMRI · nicht verbunden</span></div>
<div class="lamp-row"><span id="lampTravel" class="lamp green" aria-hidden="true"></span><span id="lampTravelLabel" aria-live="polite">Zielstellung erreicht</span></div>
</div>
<div class="console-part">
<div class="console-heading">Fahrtauftrag</div>
<div class="console-actions">
<button id="go" type="button">FAHRT</button>
<button id="reference" type="button">REFERENZ</button>
</div>
<div id="actionLabel" class="console-info" role="status">Bereit · BG3 an der Einfahrt</div>
</div>
</div>
</section>
<section class="metrics" aria-label="Simulierter Zustand">
<div class="metric"><div class="cap">Betriebsart</div><div class="val">LOCAL · SIM</div></div>
<div class="metric"><div class="cap">Einfahrt fluchtet mit</div><div id="entryLabel" class="val green">BG3</div></div>
<div class="metric"><div class="cap">Vorwahl</div><div id="selectedMetric" class="val cyan">BG3</div></div>
<div class="metric"><div class="cap">Bühne</div><div id="motorLabel" class="val">Stillstand</div></div>
</section>
<section class="card">
<h2>Schiebebühne · Gleisbild</h2>
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
<text class="section" x="200" y="443">25 cm · Einfahrt</text>
<text class="section" x="490" y="443">50 cm · Bühne</text>
<text class="section" x="780" y="443">30 cm · Abstellung</text>
</g>
</svg>
<div class="tools">
<button id="stop" type="button" aria-label="Simulierte Bühnenfahrt stoppen">■ STOP</button>
</div>
<div class="foot">
<span id="alignment">Einfahrt ↔ BG3 · BG1 ↔ AB7</span>
<span class="mini"><a href="/api/system">Speicher</a> · <a href="/api/status">Netzstatus</a></span>
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

const dialPositions=$('dialPositions');
const angles=[Math.PI,Math.PI*3/4,Math.PI/2,Math.PI/4,0];
const dialEntries=[];
let selectedTrack=3,offset=0,frame=null,referenceRun=false,completed=true;
for(let n=1;n<=5;n++){
  const angle=angles[n-1],x=190+125*Math.cos(angle),y=190-125*Math.sin(angle);
  const group=el('g',dialPositions,{'class':'dial-position',role:'button',tabindex:'0','aria-label':'BG'+n+' vorwählen'});
  el('circle',group,{cx:x.toFixed(2),cy:y.toFixed(2),r:23});
  el('text',group,{x:x.toFixed(2),y:y.toFixed(2)},'BG'+n);
  function choose(){if(frame!==null)return;selectedTrack=n;updateDial();show();}
  group.addEventListener('click',choose);
  group.addEventListener('keydown',e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();choose();}});
  dialEntries.push(group);
}
function updateDial(){
  const angle=angles[selectedTrack-1];
  $('dialLever').setAttribute('x2',(190+87*Math.cos(angle)).toFixed(2));
  $('dialLever').setAttribute('y2',(190-87*Math.sin(angle)).toFixed(2));
  $('selectedLabel').textContent='BG'+selectedTrack+' gewählt';
  $('selectedMetric').textContent='BG'+selectedTrack;
  dialEntries.forEach((e,i)=>{
    e.setAttribute('class',i+1===selectedTrack?'dial-position selected':'dial-position');
    e.setAttribute('aria-pressed',i+1===selectedTrack?'true':'false');
    e.setAttribute('aria-disabled',frame!==null?'true':'false');
  });
}
function lampState(name){
  $('lampTravel').className=name==='moving'?'lamp red blink':name==='arrived'?'lamp green':'lamp red';
  $('lampTravelLabel').textContent=name==='moving'?'Fahrt läuft …':name==='arrived'?'Zielstellung erreicht':'Zwischenposition / STOP';
}
function show(){
  const track=3-offset/25;
  const rounded=Math.round(track);
  const aligned=frame===null&&Math.abs(track-rounded)<0.003&&rounded>=1&&rounded<=5;
  bridge.setAttribute('transform','translate(0,'+offset.toFixed(3)+')');
  const active=aligned?rounded:0;
  $('entryLabel').textContent=aligned?'BG'+active:'–';
  $('alignment').textContent=aligned
    ?'Einfahrt ↔ BG'+active+' · BG1 ↔ AB'+(4+active)
    :frame!==null?'Bühne in Fahrt · Gleisfluchtung nicht bestätigt'
    :'Zwischenposition · keine Gleisfluchtung';
  for(let i=0;i<5;i++){
    rails[i].setAttribute('class',i===active-1?'rail rail-active':'rail');
    boxes[i].setAttribute('class',i===active-1?'active-bg':'badge-bg');
    texts[i].setAttribute('fill',i===active-1?'#bcffcb':'#edf5fa');
  }
  $('motorLabel').textContent=frame!==null?'in Fahrt':(aligned?'Stillstand':'STOP / Zwischenposition');
  $('motorLabel').className=frame!==null?'val cyan':'val';
  $('go').disabled=frame!==null;
  $('reference').disabled=frame!==null;
  lampState(frame!==null?'moving':aligned&&completed?'arrived':'stopped');
  updateDial();
}
function stop(){
  const wasMoving=frame!==null;
  if(frame!==null){cancelAnimationFrame(frame);frame=null;}
  if(wasMoving){completed=false;referenceRun=false;$('actionLabel').textContent='STOP · Position überprüfen';}
  show();
}
function moveTo(target,isReference){
  if(!Number.isInteger(target)||target<1||target>5||frame!==null)return;
  referenceRun=!!isReference;
  const from=offset,to=(3-target)*25,start=performance.now();
  // Urspruenglich 13 ms je SVG-Einheit; jetzt um Faktor 3 langsamer.
  const duration=Math.max(900,Math.abs(to-from)*39);
  if(Math.abs(to-from)<0.1){
    offset=to;completed=true;
    $('actionLabel').textContent=referenceRun?'Referenz (SIM) abgeschlossen':'BG'+target+' bereits erreicht';
    referenceRun=false;show();return;
  }
  completed=false;
  $('actionLabel').textContent=referenceRun?'Referenzfahrt (SIM) läuft':'Fahrt nach BG'+target+' (SIM) läuft';
  function tick(t){
    const f=Math.min(1,(t-start)/duration);
    const ease=f<.5?2*f*f:1-Math.pow(-2*f+2,2)/2;
    offset=from+(to-from)*ease;
    if(f<1){frame=requestAnimationFrame(tick);show();}
    else{
      frame=null;offset=to;completed=true;
      $('actionLabel').textContent=referenceRun?'Referenz (SIM) abgeschlossen':'BG'+target+' erreicht (SIM)';
      referenceRun=false;show();
    }
  }
  frame=requestAnimationFrame(tick);
  show();
}
$('go').addEventListener('click',()=>moveTo(selectedTrack,false));
// Nur Demonstration der UI: reale kurze Referenzfahrt wird hier NICHT nachgebildet.
// Bis zur HTTP-Anbindung dient BG3 ausschliesslich als grafischer Demo-Zielpunkt.
$('reference').addEventListener('click',()=>{
  if(frame!==null)return;
  selectedTrack=3;
  moveTo(3,true);
});
$('stop').addEventListener('click',stop);
show();
</script>
</body>
</html>)FYHTML";
