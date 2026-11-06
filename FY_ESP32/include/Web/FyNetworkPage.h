#pragma once

/** @file FyNetworkPage.h
 *  @brief WLAN-Einrichtung ohne fest einkompilierte Zugangsdaten.
 */
#include <Arduino.h>

static const char FY_NETWORK_PAGE[] PROGMEM = R"FYNET(<!doctype html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>FY UVB · Netzwerk</title>
<style>
:root{color-scheme:dark}
body{margin:0;background:#0d2030;color:#ecf5f9;font:16px/1.5 system-ui,sans-serif}
main{max-width:680px;padding:24px;margin:0 auto}
a{color:#76ddec}
section{background:#173344;border:1px solid #42657a;border-radius:12px;padding:20px;margin:18px 0}
h1{font-size:27px;margin:0}h2{font-size:19px;margin:0 0 12px}
p{margin:9px 0}label{display:block;margin:12px 0 5px}
input{display:block;width:100%;padding:12px;font:inherit;border:1px solid #68889a;border-radius:7px;background:#102532;color:white;box-sizing:border-box}
button{padding:11px 17px;margin:10px 8px 0 0;background:#267a8e;color:white;border:0;border-radius:7px;font:inherit;cursor:pointer}
button:disabled{opacity:.5;cursor:default}
small,.muted{color:#b4c7d4}
#result{white-space:pre-wrap;min-height:25px}
</style>
</head>
<body><main>
<h1>FY UVB · Netzwerk</h1>
<p><a href="/diagnostics">Diagnose</a> · <a href="/">Bedienpult</a></p>
<section>
<h2>Verbindung</h2>
<p id="state">Prüfe Netzwerk …</p>
<p id="identity" class="muted"></p>
<p id="setup" class="muted"></p>
<button id="activate" type="button">Einrichtungs-WLAN aktivieren</button>
<p><small>Das Einrichtungs-WLAN ist WPA2-geschützt. Das einmalige Passwort wird ausschließlich
am seriellen Monitor angezeigt. Zur Eingabe mit diesem WLAN verbinden und
<code>http://192.168.4.1/network</code> öffnen.</small></p>
</section>
<section>
<h2>WLAN-Zugangsdaten ändern</h2>
<p>Änderungen sind nur über das gesicherte Einrichtungs-WLAN und bei stillstehender Bühne möglich.
Das bisherige WLAN bleibt im Speicher, bis das neue erfolgreich verbunden wurde.</p>
<form id="config">
<label for="ssid">Netzwerkname (SSID)</label>
<input id="ssid" name="ssid" maxlength="32" autocomplete="off" required>
<label for="psk">WLAN-Passwort</label>
<input id="psk" name="psk" type="password" minlength="8" maxlength="63" autocomplete="new-password" required>
<button id="save" type="submit">Verbindung testen und übernehmen</button>
</form>
<p id="result" role="status"></p>
<div id="handoff" hidden>
<p><strong>Verbindung erfolgreich eingerichtet!</strong></p>
<p>Jetzt den Rechner oder das Handy wieder mit dem normalen Heim-WLAN verbinden.</p>
<p>Danach <a id="homeLink" href="/">das FY-Bedienpult im Heimnetz öffnen</a>.</p>
<p><small>Das Einrichtungs-WLAN wird nach einer kurzen Übergangszeit beendet. Die automatische Weiterleitung ist hier nicht sinnvoll, da dein Gerät zunächst das WLAN wechseln muss.</small></p>
</div>
</section>
<p><small>Es wird jeweils nur ein WLAN gespeichert. Neue Zugangsdaten ersetzen nach erfolgreichem Verbindungstest die bisherige Verbindung.</small></p>
</main>
<script>
const el=id=>document.getElementById(id);
async function update(){
 try{
  const r=await fetch('/api/v1/network/status',{cache:'no-store'});
  const n=await r.json();
  el('state').textContent=n.connected?'Verbunden · IP '+n.ip:'Keine Verbindung zum Heimnetz';
  el('identity').textContent='SSID: '+(n.ssid||'nicht eingerichtet')+(n.connected?' · '+n.rssi+' dBm':'');
  el('setup').textContent=n.setup_active?'Einrichtungs-WLAN: '+n.setup_ssid:'Einrichtungs-WLAN: aus';
  el('activate').disabled=n.setup_active;
  el('save').disabled=!n.setup_client||n.pending;
  el('handoff').hidden=!(n.result==='saved'&&n.connected);
  if(!el('handoff').hidden){
    el('homeLink').href='http://'+n.ip+'/';
  }
  if(!n.setup_client && n.result!=='saved') el('result').textContent='Zum Ändern bitte mit dem Einrichtungs-WLAN verbinden.';
  else if(n.pending) el('result').textContent='Neue WLAN-Verbindung wird geprüft …';
  else if(n.result==='failed') el('result').textContent='Verbindung fehlgeschlagen; alte Zugangsdaten bleiben erhalten.';
  else if(n.result==='saved') el('result').textContent='Zugangsdaten gespeichert. Bitte ins Heim-WLAN wechseln.';
  else if(n.result==='storage_error') el('result').textContent='Fehler beim Speichern. Bitte nochmals versuchen.';
 }catch(e){el('state').textContent='Netzwerkstatus nicht erreichbar';}
}
el('activate').addEventListener('click',async()=>{
 const r=await fetch('/api/v1/network/setup',{method:'POST'});
 el('result').textContent=r.ok?'Einrichtungs-WLAN aktiviert. Passwort im seriellen Monitor ablesen.':'Einrichtungs-WLAN konnte nicht aktiviert werden (Bühne muss stehen).';
 update();
});
el('config').addEventListener('submit',async e=>{
 e.preventDefault();
 const body=new URLSearchParams({ssid:el('ssid').value,psk:el('psk').value});
 el('psk').value='';
 const r=await fetch('/api/v1/network/apply',{method:'POST',body});
 el('result').textContent=r.ok?'Verbindungstest läuft; die bisherigen Daten bleiben zunächst erhalten.':'Eingabe abgelehnt (nur über Einrichtungs-WLAN bei Stillstand).';
 update();
});
update();setInterval(update,2000);
</script>
</body></html>)FYNET";
