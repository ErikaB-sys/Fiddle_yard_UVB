#pragma once

/** @file FyOtaPage.h
 *  @brief Lokale Firmware-Datei per Browser an den ESP32 uebertragen.
 */
#include <Arduino.h>

static const char FY_OTA_PAGE[] PROGMEM = R"FYOTA(<!doctype html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>FY UVB · Firmware-Update</title>
<style>
:root{color-scheme:dark;--bg:#0c1c29;--card:#173343;--edge:#42677b;--text:#edf6fa;--mute:#aac1cc;--ok:#96ecaa;--warn:#ffda9e}
*{box-sizing:border-box}
body{background:var(--bg);color:var(--text);font:15px/1.55 system-ui,sans-serif;margin:0}
main{max-width:720px;margin:auto;padding:24px 18px 50px}
a{color:#67ddeb}h1{font-size:28px;margin:0 0 5px}h2{font-size:19px;margin:0 0 12px}
section{background:var(--card);border:1px solid var(--edge);border-radius:12px;padding:18px;margin:17px 0}
label{display:block;margin-bottom:8px}input[type=file]{width:100%;padding:12px;border:1px dashed #7393a7;border-radius:8px}
button{background:#278aa0;color:white;border:0;border-radius:7px;padding:12px 18px;font:inherit;cursor:pointer;margin-top:13px}
button:disabled{opacity:.45;cursor:default}
.small{font-size:13px;color:var(--mute)}.good{color:var(--ok)}.warn{color:var(--warn)}
progress{display:block;width:100%;height:22px;margin:12px 0;accent-color:#44bfce}
#message{white-space:pre-wrap;overflow-wrap:anywhere}
code{overflow-wrap:anywhere}
</style>
</head>
<body><main>
<h1>FY UVB · Firmware-Update</h1>
<p><a href="/">Bedienpult</a> · <a href="/network">Netzwerk</a> · <a href="/diagnostics">Diagnose</a></p>
<section>
<h2>Status</h2>
<p id="status">Prüfe OTA-Bereitschaft …</p>
<p id="slot" class="small"></p>
<p class="small">Die Bühne muss stehen. Treiber AUS, STOP verriegelt.
Danach die rote STOP-Taste am FY mindestens <strong>3 Sekunden halten</strong>.
Die OTA-Freigabe gilt 120 Sekunden.</p>
</section>
<section>
<h2>Neue Firmware laden</h2>
<form id="otaForm">
<label for="bin">PlatformIO-Firmware auswählen: <code>.pio/build/esp32dev/firmware.bin</code></label>
<input type="file" id="bin" accept=".bin,application/octet-stream" required>
<button id="go" type="submit" disabled>Firmware aktualisieren</button>
</form>
<label for="bar" id="progressLabel" class="small">Noch kein Upload</label>
<progress id="bar" value="0" max="100">0 %</progress>
<p id="message" role="status"></p>
<p class="small">Der Motor bleibt während des Updates gesperrt. STOP bleibt lokal wirksam.
Nach erfolgreichem Schreiben und Prüfen startet der FY neu.
WLAN-Konfiguration, Gleispositionen und Motorparameter bleiben erhalten.</p>
</section>
<p class="small">Nur im vertrauenswürdigen lokalen Netzwerk verwenden.
Die physische Freigabe am FY ist Voraussetzung für jedes Firmware-Update.</p>
</main>
<script>
const el=id=>document.getElementById(id);
let token='', ready=false, busy=false, slotBytes=0;
async function status(){
 try{
  const r=await fetch('/api/v1/ota/status',{cache:'no-store'});
  if(!r.ok)throw Error('HTTP '+r.status);
  const x=await r.json();
  token=x.csrf||'';
  slotBytes=x.slot_bytes||0;
  ready=!!x.armed&&!!x.supported&&!x.active&&!x.prepared;
  el('status').textContent=x.active?'Firmware-Update läuft – Motor gesperrt':
    x.armed?'OTA freigegeben – noch '+Math.ceil(x.arm_ms/1000)+' Sekunden':
    x.supported?'OTA gesperrt – rote STOP-Taste 3 Sekunden halten':
                'OTA mit dieser Partitionstabelle nicht verfügbar';
  el('slot').textContent='Aktuelle Firmware '+x.firmware+
    ' · App-Slot '+x.running_slot+
    ' · OTA-Slot '+(slotBytes?Math.floor(slotBytes/1024)+' KiB':'nicht vorhanden');
  const f=el('bin').files[0];
  el('go').disabled=busy||!ready||!f||f.size>slotBytes;
  if(f&&f.size>slotBytes)el('message').textContent='Firmware zu groß für die OTA-Partition.';
  if(!busy&&x.result==='failed')el('message').textContent='Letzter OTA-Versuch fehlgeschlagen. Bisherige Firmware bleibt aktiv.';
 }catch(e){
  if(!busy)el('status').textContent='OTA-Status nicht erreichbar';
  el('go').disabled=true;
 }
}
el('bin').addEventListener('change',status);
el('otaForm').addEventListener('submit',async e=>{
 e.preventDefault();
 const f=el('bin').files[0];
 if(!f||!ready||!token||f.size<=0||f.size>slotBytes||busy)return;
 if(!f.name.toLowerCase().endsWith('.bin')){el('message').textContent='Bitte eine .bin-Datei auswählen.';return;}
 if(!confirm('Firmware '+f.name+' ('+Math.ceil(f.size/1024)+' KiB) jetzt installieren?\nDer FY startet danach automatisch neu.'))return;
 busy=true;el('go').disabled=true;
 el('message').textContent='Prüfe OTA-Freigabe …';
 try{
  const prep=await fetch('/api/v1/ota/prepare',{
   method:'POST',
   body:new URLSearchParams({csrf:token,size:String(f.size)})
  });
  if(!prep.ok)throw Error(await prep.text());
  el('message').textContent='Übertrage Firmware …';
  const data=new FormData();
  data.append('firmware',f,f.name);
  const xhr=new XMLHttpRequest();
  xhr.open('POST','/api/v1/ota/upload?csrf='+encodeURIComponent(token));
  xhr.upload.onprogress=e=>{
   if(!e.lengthComputable)return;
   const pct=Math.min(99,Math.floor(e.loaded*100/e.total));
   el('bar').value=pct;
   el('progressLabel').textContent='Übertragung: '+pct+' %';
   if(pct>=99)el('message').textContent='Übertragung abgeschlossen. Prüfe Firmware …';
  };
  xhr.onload=()=>{
   if(xhr.status===200){
    el('bar').value=100;
    el('progressLabel').textContent='100 % – Firmware geprüft';
    el('message').textContent='Update erfolgreich! Der FY startet jetzt neu. Danach die Seite neu laden.';
   }else{
    el('message').textContent='Update fehlgeschlagen: '+xhr.responseText;
   }
   busy=false;
  };
  xhr.onerror=()=>{el('message').textContent='Verbindung abgebrochen. Bei unvollständigem Upload bleibt die bisherige Firmware aktiv.';busy=false;};
  xhr.send(data);
 }catch(err){el('message').textContent='OTA konnte nicht gestartet werden: '+err.message;busy=false;}
});
status();setInterval(()=>{if(!busy)status();},1500);
</script>
</body></html>)FYOTA";
