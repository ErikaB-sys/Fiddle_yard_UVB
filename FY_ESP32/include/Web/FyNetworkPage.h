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
<p><a href="/diagnostics">Diagnose</a> · <a href="/">Bedienpult</a> · <a href="/ota">OTA</a></p>
<section>
<h2>Verbindung</h2>
<p id="state">Prüfe Netzwerk …</p>
<p id="identity" class="muted"></p>
<p class="muted">WLAN-Client-MAC (STA): <code id="staMac">Wird ermittelt …</code></p>
<p><small>Für eine feste IP-Adresse diese MAC im Router für eine DHCP-Reservierung verwenden.</small></p>
<p id="setup" class="muted"></p>
<details>
<summary>fy-uvb.local nicht erreichbar?</summary>
<p><small>Falls <code>http://fy-uvb.local/</code> nicht funktioniert, die oben angezeigte IP-Adresse verwenden.
Für mDNS müssen Rechner und FY normalerweise im selben lokalen Netz sein und Multicast darf nicht gesperrt werden.
Gegebenenfalls mDNS-/Multicast-Einstellungen, Gastnetz oder Client-Isolation am Router bzw. Access Point prüfen.
Zwischen getrennten Netzen wird mDNS nicht automatisch weitergeleitet.</small></p>
</details>
<details>
<summary>Notfallzugang über Einrichtungs-WLAN</summary>
<p><small>Nur nötig, wenn das Heim-WLAN nicht erreichbar ist.
Das Einrichtungs-WLAN ist passwortgeschützt; das einmalige Passwort
erscheint im seriellen Monitor. Zugriff dann über
<code>http://192.168.4.1/network</code>.</small></p>
<button id="activate" type="button" disabled>Einrichtungs-WLAN aktivieren</button>
</details>
</section>
<section>
<h2>WLAN-Zugangsdaten ändern</h2>
<p>Neue SSID und Passwort einfach hier eingeben. Die Bühne muss dabei stehen.
Das bisherige WLAN bleibt gespeichert, bis die neue Verbindung funktioniert.</p>
<p><small>Hinweis: Diese Konfiguration verwendet HTTP ohne Verschlüsselung.
Nur in einem privaten, vertrauenswürdigen WLAN verwenden.</small></p>
<form id="config">
<label for="ssid">Netzwerkname (SSID)</label>
<input id="ssid" name="ssid" maxlength="32" autocomplete="off" required>
<label for="psk">WLAN-Passwort</label>
<input id="psk" name="psk" type="password" minlength="8" maxlength="63" autocomplete="new-password" required>
<button id="save" type="submit" disabled>Verbindung testen und übernehmen</button>
</form>
<p id="result" role="status"></p>
<div id="handoff" hidden>
<p><strong>Verbindung erfolgreich eingerichtet!</strong></p>
<p id="handoffInfo">Falls du das WLAN gewechselt hast: Rechner oder Handy mit dem neuen WLAN verbinden.</p>
<p>Neue FY-Adresse: <strong id="newIp"></strong></p>
<p><a id="homeLink" href="/">Das FY-Bedienpult öffnen</a></p>
<p><small>Alternativ: <code>http://fy-uvb.local/</code> nach dem WLAN-Wechsel aufrufen.</small></p>
</div>
</section>
<p><small>Es wird jeweils nur ein WLAN gespeichert. Neue Zugangsdaten ersetzen nach erfolgreichem Verbindungstest die bisherige Verbindung.</small></p>
</main>
<script>
const el=id=>document.getElementById(id);
let wasSubmitted=false;
let csrfToken='';
let allowSsidPrefill=true;
el('ssid').addEventListener('input',()=>{allowSsidPrefill=false;});
async function update(){
 try{
  const r=await fetch('/api/v1/network/status',{cache:'no-store'});
  if(!r.ok) throw Error('HTTP '+r.status);
  const n=await r.json();
  csrfToken=n.csrf||'';
  el('state').textContent=n.connected?'Verbunden · IP '+n.ip:'Keine Verbindung zum Heimnetz';
  el('identity').textContent='SSID: '+(n.ssid||'nicht eingerichtet')+(n.connected?' · '+n.rssi+' dBm':'');
  el('staMac').textContent=n.sta_mac||'nicht verfügbar';
  el('setup').textContent=n.setup_active?'Einrichtungs-WLAN: '+n.setup_ssid:'Einrichtungs-WLAN: aus';
  el('activate').disabled=n.setup_active||n.pending||!csrfToken;
  el('save').disabled=n.pending||!csrfToken||(!n.setup_client&&!n.connected);
  if(allowSsidPrefill&&n.ssid)el('ssid').value=n.ssid;
  el('handoff').hidden=!(n.result==='saved'&&n.connected);
  if(!el('handoff').hidden){
    el('homeLink').href='http://'+n.ip+'/';
    el('newIp').textContent=n.ip;
    el('handoffInfo').textContent=n.setup_client
      ? 'Bitte mit dem neu eingerichteten WLAN verbinden.'
      : 'Bei einem WLAN-Wechsel den Rechner oder das Handy mit dem neuen WLAN verbinden.';
  }
  if(n.pending)el('result').textContent='Neue WLAN-Verbindung wird geprüft …';
  else if(n.result==='failed')el('result').textContent='Verbindung fehlgeschlagen; alte Zugangsdaten bleiben erhalten.';
  else if(n.result==='saved')el('result').textContent='Neue Zugangsdaten gespeichert.';
  else if(n.result==='storage_error')el('result').textContent='Speichern fehlgeschlagen; bitte erneut versuchen.';
  else if(!n.setup_client&&!n.connected)el('result').textContent='Zum Einrichten mit dem FY-Einrichtungs-WLAN verbinden.';
 }catch(e){
   el('state').textContent='Keine Verbindung zum FY.';
   if(wasSubmitted)el('result').textContent=
     'Die Verbindung zum bisherigen WLAN ist unterbrochen. Bitte mit dem neuen WLAN verbinden und http://fy-uvb.local/network erneut öffnen. Die neue IP wird auch seriell ausgegeben. Falls die Verbindung nicht klappt, kehrt der FY zum bisherigen WLAN zurück.';
 }
}
el('activate').addEventListener('click',async()=>{
 try{
  const r=await fetch('/api/v1/network/setup',{
    method:'POST',
    body:new URLSearchParams({csrf:csrfToken})
  });
  el('result').textContent=r.ok
    ?'Einrichtungs-WLAN aktiviert. Passwort im seriellen Monitor ablesen.'
    :'Einrichtungs-WLAN konnte nicht aktiviert werden (Bühne muss stehen).';
 }catch(e){el('result').textContent='Keine Verbindung zum FY.';}
 update();
});
el('config').addEventListener('submit',async e=>{
 e.preventDefault();
 if(!confirm('Neues WLAN jetzt testen? Die Verbindung zu dieser Seite kann während des Wechsels abbrechen.'))return;
 const body=new URLSearchParams({
   ssid:el('ssid').value,psk:el('psk').value,csrf:csrfToken
 });
 el('psk').value='';
 wasSubmitted=true;
 el('result').textContent='Verbindungstest angefordert …';
 try{
  const r=await fetch('/api/v1/network/apply',{method:'POST',body});
  el('result').textContent=r.ok
    ?'Verbindungstest läuft. Bei einem WLAN-Wechsel bitte auf das neue WLAN umschalten und diese Seite erneut öffnen.'
    :'Änderung abgelehnt. Nur im lokalen Netzwerk bei stillstehender Bühne möglich.';
 }catch(e){
  el('result').textContent='Verbindung abgebrochen. Eventuell wurde das WLAN bereits gewechselt. Bitte im neuen WLAN http://fy-uvb.local/network öffnen.';
 }
 update();
});
update();setInterval(update,2000);
</script>
</body></html>)FYNET";
