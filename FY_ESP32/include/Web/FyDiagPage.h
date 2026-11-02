#pragma once

/**
 * @file FyDiagPage.h
 * @brief Reine HTTP-Diagnoseansicht ohne Steuerbefehle.
 */
#include <Arduino.h>

static const char FY_DIAG_PAGE[] PROGMEM = R"FYDIAG(<!doctype html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>FY UVB · Diagnose</title>
<style>
:root{color-scheme:dark;--bg:#0b1b26;--panel:#142a38;--edge:#3f6174;--ink:#edf6fb;--mute:#9bb0bf;--good:#85ec93;--warn:#ffcd8c;--bad:#ff8f96}
*{box-sizing:border-box}body{background:linear-gradient(150deg,#142d3c,#091520);color:var(--ink);font:15px/1.55 system-ui,-apple-system,Segoe UI,sans-serif;margin:0}
main{max-width:1100px;margin:auto;padding:22px 18px 50px}
header{display:flex;align-items:center;flex-wrap:wrap;gap:13px;margin-bottom:16px}
h1{font-size:clamp(24px,3vw,32px);margin:0}h2{font-size:17px;margin:0 0 9px}
a{color:#56dbf1}.small{font-size:13px;color:var(--mute)}
.grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:12px}
.card{background:var(--panel);border:1px solid var(--edge);border-radius:11px;padding:15px;margin:0 0 12px;min-width:0}
.wide{grid-column:span 3}.metric{display:flex;justify-content:space-between;align-items:baseline;gap:12px;border-bottom:1px solid #304b5b;padding:6px 0}
.metric:last-child{border-bottom:none}.metric span{color:var(--mute)}.metric strong{font-variant-numeric:tabular-nums;text-align:right;overflow-wrap:anywhere}
.good{color:var(--good)}.warn{color:var(--warn)}.bad{color:var(--bad)}
.pills{display:grid;grid-template-columns:repeat(4,1fr);gap:8px}.sensor{padding:12px 8px;border:1px solid #496379;border-radius:8px;text-align:center;background:#193241}
.sensor.on{border-color:#8fdb98;background:#17472d}.sensor strong{display:block;font-size:20px}.sensor label{display:block;color:var(--mute);font-size:12px}
pre{white-space:pre-wrap;overflow-wrap:anywhere;max-height:350px;overflow:auto;background:#091b27;border-radius:7px;padding:12px;font-size:12px;color:#d6edf2}
footer{margin-top:8px;color:var(--mute);font-size:13px}
@media(max-width:720px){main{padding:15px 10px}.grid{grid-template-columns:1fr 1fr}.wide{grid-column:span 2}}
@media(max-width:480px){.grid{grid-template-columns:1fr}.wide{grid-column:span 1}.pills{grid-template-columns:repeat(2,1fr)}}
</style>
</head>
<body><main>
<header><h1>FY UVB · Diagnose</h1><a href="/">← Bedienpult</a><span class="small" id="connection">Verbinde …</span></header>
<div class="grid">
<section class="card"><h2>Motor und Bühne</h2>
<div class="metric"><span>Zustand</span><strong id="state">–</strong></div>
<div class="metric"><span>Treiber</span><strong id="driver">–</strong></div>
<div class="metric"><span>Referenz</span><strong id="ref">–</strong></div>
<div class="metric"><span>Position</span><strong id="pos">–</strong></div>
<div class="metric"><span>Gleis (bestätigt / Ziel)</span><strong id="track">–</strong></div>
<div class="metric"><span>Richtung</span><strong id="direction">–</strong></div>
</section>
<section class="card"><h2>System</h2>
<div class="metric"><span>Firmware</span><strong id="firmware">–</strong></div>
<div class="metric"><span>Uptime</span><strong id="uptime">–</strong></div>
<div class="metric"><span>Flash</span><strong id="flash">–</strong></div>
<div class="metric"><span>Freier Heap</span><strong id="heap">–</strong></div>
<div class="metric"><span>Minimum freier Heap</span><strong id="heapMin">–</strong></div>
<div class="metric"><span>NVM Fehler / Gültigkeit</span><strong id="nvm">–</strong></div>
</section>
<section class="card"><h2>Kommunikation</h2>
<div class="metric"><span>Betriebsart</span><strong id="mode">LOCAL</strong></div>
<div class="metric"><span>Websteuerung</span><strong id="control">–</strong></div>
<div class="metric"><span>WLAN / RSSI</span><strong id="wifi">–</strong></div>
<div class="metric"><span>IP</span><strong id="ip">–</strong></div>
<div class="metric"><span>HTTP RX / TX</span><strong id="counts">–</strong></div>
<div class="metric"><span>Letzter Befehl</span><strong id="last">–</strong></div>
<div class="metric"><span>Letzte Antwort</span><strong id="reply">–</strong></div>
</section>
<section class="card wide"><h2>Lichtschranken und Sensoren</h2>
<p class="small">Rohpegel (HIGH / LOW). Die Bedeutung „aktiv“ hängt von der Sensorbeschaltung ab; hier wird bewusst keine Endlage interpretiert.</p>
<div class="pills">
<div id="lsL" class="sensor"><strong>–</strong><label>LSL · G36</label></div>
<div id="lsR" class="sensor"><strong>–</strong><label>LSR · G39</label></div>
<div id="lsRef" class="sensor"><strong>–</strong><label>LSREF · G34</label></div>
<div id="lsBelt" class="sensor"><strong>–</strong><label>LS4 · G35</label></div>
</div>
</section>
<section class="card"><h2>FreeRTOS-Queues</h2>
<div class="metric"><span>Befehle</span><strong id="commands">–</strong></div>
<div class="metric"><span>STOP-Priorität</span><strong id="priority">–</strong></div>
<div class="metric"><span>UART-Ereignisse</span><strong id="uart">–</strong></div>
<div class="metric"><span>Keyboard-Ereignisse</span><strong id="keyboard">–</strong></div>
<div class="metric"><span>HTTP-Ereignisse</span><strong id="http">–</strong></div>
</section>
<section class="card wide"><h2>Rohdaten</h2>
<pre id="raw" aria-live="off">Warte auf /api/v1/diagnostics …</pre>
</section>
</div>
<footer>Nur lesende Diagnose · Aktualisierung 1× pro Sekunde · keine HTTP-Befehle und kein UART-Trace durch diese Seite</footer>
</main>
<script>
'use strict';
const $=id=>document.getElementById(id);
function show(id,v){$(id).textContent=v===null||v===undefined?'–':String(v);}
function bytes(n){return Number.isFinite(n)?Math.round(n/1024)+' KiB':'–';}
function sensor(id,value){
  const e=$(id); e.querySelector('strong').textContent=value===1?'HIGH':'LOW';
  e.className='sensor'+(value===1?' on':'');
}
let polling=false,lastOk=0;
async function refresh(){
  if(polling)return;
  polling=true;
  try{
    const response=await fetch('/api/v1/diagnostics',{cache:'no-store'});
    if(!response.ok)throw Error('HTTP '+response.status);
    const d=await response.json(),m=d.motor,s=d.system,n=d.network,h=d.http,q=d.queues;
    show('state',m.state);
    show('driver',m.driver_enabled?'EIN':'AUS');
    show('ref',m.referenced?'GÜLTIG':'UNGÜLTIG');
    show('pos',m.position_steps===null?'unbekannt':m.position_steps+' Steps');
    show('track',(m.confirmed_track||'–')+' / '+(m.target_track||'–'));
    show('direction',['Stillstand','LINKS','RECHTS'][m.direction]||'unbekannt');
    show('firmware',s.firmware);
    show('uptime',Math.floor(s.uptime_ms/1000)+' s');
    show('flash',bytes(s.flash_bytes));
    show('heap',bytes(s.heap_free_bytes));
    show('heapMin',bytes(s.heap_min_free_bytes));
    show('nvm','0x'+s.nvm_faults.toString(16).toUpperCase()+' / 0x'+s.nvm_valid.toString(16).toUpperCase());
    show('mode',m.mode);
    show('control',h.commands_enabled?'TEST AKTIV':'GESPERRT');
    show('wifi',n.connected?'verbunden · '+n.rssi+' dBm':'getrennt');
    show('ip',n.ip);
    show('counts',h.rx_count+' / '+h.tx_count);
    show('last',h.last_command);
    show('reply',h.last_result+' · '+h.last_reason);
    sensor('lsL',m.sensor_levels.LSL);sensor('lsR',m.sensor_levels.LSR);
    sensor('lsRef',m.sensor_levels.REF);sensor('lsBelt',m.sensor_levels.BELT);
    show('commands',q.commands);show('priority',q.priority);
    show('uart',q.uart_events);show('keyboard',q.keyboard_events);show('http',q.http_events);
    $('raw').textContent=JSON.stringify(d,null,2);
    lastOk=performance.now();
    $('connection').textContent='ESP32 live · '+new Date().toLocaleTimeString();
    $('connection').className='small good';
  }catch(err){
    $('connection').textContent='Verbindung gestört · '+err.message;
    $('connection').className='small bad';
  }finally{polling=false}
}
setInterval(refresh,1000);refresh();
</script>
</body></html>)FYDIAG";
