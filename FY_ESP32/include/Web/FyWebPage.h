#pragma once

/**
 * @file FyWebPage.h
 * @brief Integrierte FY-Weboberflaeche mit echten Muninn-Snapshots.
 *
 * Die Seite liest echte Statuswerte. HTTP-Motorbefehle bleiben gesperrt!
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
#stop:hover{background:#81313b}#stop:disabled{opacity:.48;cursor:not-allowed}
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
<header><h1>FY UVB · Web-Prüfstand</h1><span class="tag">LOCAL · ANZEIGE</span><span id="netLabel" class="sub" aria-live="polite">Echter Muninn-Status wird geladen · HTTP-Fahrbefehle gesperrt</span></header>
<section class="console" aria-label="FY-Bedienpult">
<div class="console-part">
<div class="console-heading">Gleiswahl · historischer Fahrgeber</div>
<svg id="selector" class="dial" viewBox="0 0 380 230" role="group" aria-label="Brückengleis BG1 bis BG5 vorwählen">
<path class="dial-arc" d="M65 190 A125 125 0 0 1 315 190"/>
<g id="dialPositions"></g>
<line id="dialLever" class="dial-lever" x1="190" y1="190" x2="190" y2="80"/>
<circle class="dial-hub" cx="190" cy="190" r="18"/>
<circle cx="190" cy="190" r="6" fill="#2c3540"/>
<text id="selectedLabel" class="dial-center" x="190" y="224">BG3 gewählt</text>
</svg>
<div class="console-info">Vorwahl auf allen Browsern gemeinsam. FAHRT, REFERENZ und STOP per HTTP noch gesperrt.</div>
</div>
<div class="console-side">
<div class="console-part">
<div class="console-heading">Kontrolllampen</div>
<div class="lamp-row"><span id="lampLocal" class="lamp green" aria-hidden="true"></span><span>LOCAL · Webanzeige</span></div>
<div class="lamp-row"><span id="lampJmri" class="lamp" aria-hidden="true"></span><span>JMRI · nicht verbunden</span></div>
<div class="lamp-row"><span id="lampTravel" class="lamp" aria-hidden="true"></span><span id="lampTravelLabel" aria-live="polite">Warte auf Status …</span></div>
</div>
<div class="console-part">
<div class="console-heading">Fahrtauftrag</div>
<div class="console-actions">
<button id="go" type="button" disabled>FAHRT</button>
<button id="reference" type="button" disabled>REFERENZ</button>
</div>
<div id="actionLabel" class="console-info" role="status">Lade gemeinsamen ESP32-Status …</div>
</div>
</div>
</section>
<section class="metrics" aria-label="Simulierter Zustand">
<div class="metric"><div class="cap">Betriebsart</div><div class="val">LOCAL · LESEN</div></div>
<div class="metric"><div class="cap">Einfahrt fluchtet mit</div><div id="entryLabel" class="val green">–</div></div>
<div class="metric"><div class="cap">Vorwahl</div><div id="selectedMetric" class="val cyan">BG3</div></div>
<div class="metric"><div class="cap">Bühne</div><div id="motorLabel" class="val">Verbinde …</div></div>
</section>
<section class="card">
<h2>Schiebebühne · Gleisbild</h2>
<svg id="plan" class="schematic" viewBox="0 0 980 453" role="img" aria-label="Links drei gerade Gleise, Mitte fünf gemeinsam bewegliche Gleise, rechts neun gerade Abstellgleise. Alle Gleise haben identischen Abstand. Buehnenposition nur bei gueltiger Referenz.">
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
<button id="stop" type="button" disabled title="HTTP-STOP noch nicht freigegeben" aria-label="HTTP-STOP noch nicht freigegeben">■ STOP</button>
</div>
<div class="foot">
<span id="alignment">Warte auf ESP32-Snapshot …</span>
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
let selectedTrack=3;
let fy=null,connected=false,polling=false,sending=false,selecting=false,firstSnapshot=true;
let lastSeen=0,displayedPosition=3,positionFrame=null;
let dialAngle=angles[2],dialTarget=3,dialFrame=null;
for(let n=1;n<=5;n++){
  const angle=angles[n-1],x=190+125*Math.cos(angle),y=190-125*Math.sin(angle);
  const group=el('g',dialPositions,{'class':'dial-position',role:'button',tabindex:'0','aria-label':'BG'+n+' vorwählen'});
  el('circle',group,{cx:x.toFixed(2),cy:y.toFixed(2),r:23});
  el('text',group,{x:x.toFixed(2),y:y.toFixed(2)},'BG'+n);
  function choose(){
    if(!connected||sending||selecting||(fy&&fy.busy))return;
    selectedTrack=n;
    updateDial();show();
    sendSelection(n);
  }
  group.addEventListener('click',choose);
  group.addEventListener('keydown',e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();choose();}});
  dialEntries.push(group);
}
// Der Fahrgeber muss jede durchfahrene Raste kurz erreichen und halten.
// Eine neue Vorwahl waehrend der Bewegung beginnt von der aktuellen Zeigerlage.
function renderLever(angle){
  $('dialLever').setAttribute('x2',(190+110*Math.cos(angle)).toFixed(2));
  $('dialLever').setAttribute('y2',(190-110*Math.sin(angle)).toFixed(2));
}
function snapLever(target){
  if(dialFrame!==null){cancelAnimationFrame(dialFrame);dialFrame=null;}
  const from=dialAngle,delta=target-from;
  if(Math.abs(delta)<0.0001){dialAngle=target;renderLever(target);return;}
  const direction=Math.sign(delta);
  // Rastpunkte zwischen Start und Ziel in Drehrichtung durchlaufen.
  const waypoints=angles.filter(a=>direction>0
    ? a>from+0.0001&&a<target-0.0001
    : a<from-0.0001&&a>target+0.0001
  ).sort((a,b)=>direction*(a-b));
  waypoints.push(target);
  const segments=[];
  let at=0,last=from;
  for(let i=0;i<waypoints.length;i++){
    const end=waypoints[i];
    const duration=Math.max(70,180*Math.abs(end-last)/(Math.PI/4));
    segments.push({from:last,to:end,start:at,end:at+duration});
    at+=duration;
    if(i<waypoints.length-1)at+=105; // 105 ms Rastpause in jeder Zwischenstellung
    last=end;
  }
  const total=at,start=performance.now();
  function tick(time){
    const elapsed=Math.max(0,time-start);
    let angle=target;
    for(let i=0;i<segments.length;i++){
      const seg=segments[i];
      if(elapsed<seg.end){
        const t=Math.max(0,(elapsed-seg.start)/(seg.end-seg.start));
        // Kurzes Ueberfedern an jeder Raste, dann exakt einruecken.
        const u=t-1;
        const eased=1+2.2*u*u*u+1.2*u*u;
        angle=seg.from+(seg.to-seg.from)*eased;
        break;
      }
      const holdEnd=i+1<segments.length?segments[i+1].start:seg.end;
      if(elapsed<holdEnd){angle=seg.to;break;}
    }
    dialAngle=elapsed>=total?target:angle;
    renderLever(dialAngle);
    if(elapsed<total)dialFrame=requestAnimationFrame(tick);
    else dialFrame=null;
  }
  dialFrame=requestAnimationFrame(tick);
}
function updateDial(){
  if(dialTarget!==selectedTrack){
    dialTarget=selectedTrack;
    snapLever(angles[selectedTrack-1]);
  }
  $('selectedLabel').textContent='BG'+selectedTrack+' gewählt';
  $('selectedMetric').textContent='BG'+selectedTrack;
  dialEntries.forEach((e,i)=>{
    e.setAttribute('class',i+1===selectedTrack?'dial-position selected':'dial-position');
    e.setAttribute('aria-pressed',i+1===selectedTrack?'true':'false');
    e.setAttribute('aria-disabled',(!connected||sending||selecting||(fy&&fy.busy))?'true':'false');
  });
}
function lampState(name){
  const classes={
    moving:'lamp red blink',
    arrived:'lamp green',
    stopped:'lamp red',
    unknown:'lamp'
  };
  const labels={
    moving:'Fahrt läuft …',
    arrived:'Zielstellung erreicht',
    stopped:'STOP / nicht referenziert',
    unknown:'Verbindung / Status unbekannt'
  };
  $('lampTravel').className=classes[name];
  $('lampTravelLabel').textContent=labels[name];
}
function renderBridge(){
  const offset=(3-displayedPosition)*25;
  bridge.setAttribute('transform','translate(0,'+offset.toFixed(3)+')');
}
function animateReportedPosition(next,busy){
  if(positionFrame!==null){cancelAnimationFrame(positionFrame);positionFrame=null;}
  if(!Number.isFinite(next)||next<0.9||next>5.1)return;
  if(!busy || firstSnapshot){
    displayedPosition=next;
    renderBridge();
    return;
  }
  // Nur die Darstellung weich interpolieren; der ESP32 ist die Zustandsquelle.
  const start=performance.now(),from=displayedPosition;
  function tick(time){
    const t=Math.min(1,(time-start)/230);
    displayedPosition=from+(next-from)*t;
    renderBridge();
    if(t<1)positionFrame=requestAnimationFrame(tick);
    else positionFrame=null;
  }
  positionFrame=requestAnimationFrame(tick);
}
function show(){
  const fresh=connected&&fy!==null&&(performance.now()-lastSeen<1300);
  const aligned=fresh&&!fy.busy&&fy.referenced&&Number.isInteger(fy.track)
                &&fy.track>=1&&fy.track<=5;
  const active=aligned?fy.track:0;
  $('netLabel').textContent=fresh?'Muninn live · HTTP nur Status/Vorwahl · Motorbefehle gesperrt'
                                   :'Verbindung gestört · Status veraltet · keine HTTP-Motorbefehle';
  $('entryLabel').textContent=aligned?'BG'+active:'–';
  $('alignment').textContent=!fresh?'Verbindung verloren · Bühnenlage nicht bestätigt'
    :aligned?'Einfahrt ↔ BG'+active+' · BG1 ↔ AB'+(4+active)
    :fy.busy?'Bühne in Fahrt · Gleisfluchtung nicht bestätigt'
    :'Zwischenposition oder Referenz ungültig';
  for(let i=0;i<5;i++){
    rails[i].setAttribute('class',i===active-1?'rail rail-active':'rail');
    boxes[i].setAttribute('class',i===active-1?'active-bg':'badge-bg');
    texts[i].setAttribute('fill',i===active-1?'#bcffcb':'#edf5fa');
  }
  $('motorLabel').textContent=!fresh?'Status veraltet'
    :fy.busy?(fy.state==='REFERENCING'?'Referenzfahrt':'in Fahrt')
    :aligned?'Stillstand':fy.state==='STOPPED'?'STOP'
    :fy.state==='ERROR'?'FEHLER':'Nicht referenziert';
  $('motorLabel').className=fresh&&fy.busy?'val cyan':'val';
  // Erst nach gemeinsamer Command-Router-/Bedienhoheitsintegration freigeben.
  $('go').disabled=true;
  $('reference').disabled=true;
  $('stop').disabled=true;
  lampState(!fresh?'unknown':fy.busy?'moving':aligned?'arrived':'stopped');
  updateDial();
}
async function pollStatus(){
  if(polling)return;
  polling=true;
  try{
    const resp=await fetch('/api/v1/status',{cache:'no-store'});
    if(!resp.ok)throw new Error('HTTP '+resp.status);
    const snapshot=await resp.json();
    if(snapshot.device!=='FY-UVB'||snapshot.simulation!==false||
       snapshot.readonly!==true||
       !(snapshot.position===null||Number.isFinite(snapshot.position))){
      throw new Error('Ungueltiger FY-Status');
    }
    // Vorwahl ist ESP32-Zustand, nicht mehr pro Browser.
    // Waehrend des laufenden SELECT_TRACK-POST optimistische Vorwahl nicht ueberschreiben.
    if(!selecting && Number.isInteger(snapshot.selected_track) &&
       snapshot.selected_track>=1 && snapshot.selected_track<=5){
      selectedTrack=snapshot.selected_track;
    }
    if(firstSnapshot){
      $('actionLabel').textContent='LOCAL aktiv · gemeinsame Vorwahl bereit · HTTP-Fahrt gesperrt';
    }
    connected=true;
    fy=snapshot;
    lastSeen=performance.now();
    animateReportedPosition(snapshot.position,snapshot.busy);
    firstSnapshot=false;
    show();
  }catch(err){
    connected=false;
    if(positionFrame!==null){cancelAnimationFrame(positionFrame);positionFrame=null;}
    $('actionLabel').textContent='HTTP-Status nicht erreichbar';
    show();
  }finally{
    polling=false;
  }
}
// Der Fahrgeber aendert nur die gemeinsame Vorwahl – die Buehne bleibt stehen.
async function sendSelection(track){
  if(!connected||sending||selecting||(fy&&fy.busy))return;
  selecting=true;
  show();
  const data=new URLSearchParams();
  data.set('command','SELECT_TRACK');
  data.set('value',String(track));
  try{
    const response=await fetch('/api/v1/command',{
      method:'POST',
      headers:{'Content-Type':'application/x-www-form-urlencoded;charset=UTF-8'},
      body:data.toString(),
      cache:'no-store'
    });
    const reply=await response.json();
    if(!response.ok||reply.result!=='ACK'){
      $('actionLabel').textContent='Vorwahl NACK: '+(reply.reason||'HTTP '+response.status);
    }else{
      $('actionLabel').textContent='BG'+track+' gemeinsam vorgewählt · keine Fahrt';
    }
  }catch(err){
    connected=false;
    $('actionLabel').textContent='Vorwahl nicht bestätigt · Verbindung prüfen';
  }finally{
    selecting=false;
    await pollStatus();
    show();
  }
}
async function sendCommand(command,track){
  if(!connected||sending)return;
  if(command!=='STOP'&&(!fy||fy.busy))return;
  sending=true;
  show();
  const stop=command==='STOP';
  const url=stop?'/api/v1/stop':'/api/v1/command';
  const data=new URLSearchParams();
  if(!stop){data.set('command',command);if(command==='SET_TRACK')data.set('value',String(track));}
  $('actionLabel').textContent=stop?'STOP angefordert …':command+' angefordert …';
  try{
    const response=await fetch(url,{
      method:'POST',
      headers:{'Content-Type':'application/x-www-form-urlencoded;charset=UTF-8'},
      body:data.toString(),
      cache:'no-store'
    });
    const reply=await response.json();
    if(!response.ok||reply.result!=='ACK'){
      $('actionLabel').textContent='NACK: '+(reply.reason||'HTTP '+response.status);
    }else{
      $('actionLabel').textContent=stop?'STOP angenommen (SIM)'
        :command==='REFERENCE'?'Referenz angenommen'
        :'BG'+track+' angenommen – Ziel noch nicht bestätigt';
    }
    await pollStatus();
  }catch(err){
    connected=false;
    $('actionLabel').textContent='Keine HTTP-Bestätigung · Status prüfen';
    show();
  }finally{
    sending=false;
    show();
  }
}
$('go').addEventListener('click',()=>sendCommand('SET_TRACK',selectedTrack));
$('reference').addEventListener('click',()=>sendCommand('REFERENCE'));
$('stop').addEventListener('click',()=>sendCommand('STOP'));
// Gemeinsame Position aus ESP32-RAM: alle Clients fragen denselben Snapshot ab.
setInterval(pollStatus,250);
pollStatus();

</script>
</body>
</html>)FYHTML";
