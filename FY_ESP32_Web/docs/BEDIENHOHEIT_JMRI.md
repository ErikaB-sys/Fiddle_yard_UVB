# FY-UVB – Bedienhoheit und JMRI-Verbindungsverlust

**Stand:** 02.11.2026 · **Status:** Architekturentscheidung / Verhaltensvertrag, **noch nicht implementiert**  
**Geltungsbereich:** späterer gemeinsamer FY-ESP32 mit HTTP-Babelfisch, Huginn (Kommunikation), Muninn (Motor), Browser und JMRI.  
**Siehe auch:** [Issue #166](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/166), [Issue #160](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/160), bestehende STOP-/Referenz- und UART-Verträge.

## 1. Grundsatz

**Verlust des JMRI-Lebenszeichens ist kein STOP und kein Motorfehler.**

Die FY-Bühne vollendet einen **bereits angenommenen und begonnenen, weiterhin zulässigen Fahr-/Referenzauftrag** nach der bestehenden Motorlogik. Danach bleibt sie in ihrer erreichten Position stehen und wartet ohne automatisches Abschalten oder automatischen Folgeauftrag. Kommunikationsverlust allein verwirft **weder Referenz noch Position**. Diese Daten können weiterhin durch andere, eigenständige Ursachen ungültig werden.

Ausnahmen haben Vorrang: **örtlicher/ausgelöster STOP, Endlagen, Motor- und Sensorfehler sowie die vorhandenen Sicherheitsprüfungen**. Diese Regeln werden durch den Wunsch, die laufende Fahrt zu beenden, **nicht** aufgehoben.

## 2. Bedienhierarchie und Startverhalten

- **JMRI (AUTO)** ist nach authentifizierter, zulässiger Übernahme der reguläre Bedienführer und hat im Betrieb Vorrang gegenüber Browserbedienung.
- **Browser (FERN)** dient vorrangig der Visualisierung; Bedienung nur bei explizit erteilter Bedienhoheit. Sobald JMRI AUTO übernimmt, wechselt **jeder Browser** sichtbar in den **Anzeigemodus**. Nicht nur die Oberfläche sperrt die SET-Befehle: Der ESP32 prüft die Bedienhoheit zwingend selbst.
- **Örtliches Keyboard (LOCAL)** bleibt die netzunabhängige Rückfallbedienung, insbesondere nach Power-on-Reset (POR). **STOP** ist aus jedem Zustand immer vorrangig; reguläre lokale Motorbedienung bei aktiver JMRI-Führung erfordert ausdrücklich eine zulässige lokale Übernahme.
- **Nach jedem POR:** Start in **LOCAL**, ohne alte JMRI-Sitzung oder automatisch wiederhergestellte AUTO-/FERN-Bedienhoheit. Lokales Keyboard kann die nach POR zulässigen Funktionen (insbesondere STOP, Referenzfahrt, begrenzte manuelle Fahrt) ausführen; referenzpflichtige Fahrten bleiben bis gültiger REF gesperrt. Der Browser kann beobachten und eine zulässige Übernahme von FERN beantragen.
- **AUTO wird erst nach neu authentifizierter JMRI-Anmeldung aktiv**; bloßer WLAN-/HTTP-Kontakt reicht nicht.
- **Betriebsartwechsel** nur bei **bestätigtem Stillstand**. Keine laufende Fahrt durch gewöhnlichen Übernahmewunsch abbrechen.
- Der Wechsel der Bedienhoheit und der Kommunikationszustand sind **getrennte Zustandsgrößen**. `AUTO / JMRI_VERLOREN` ist **keine vierte Betriebsart**.

## 3. JMRI-Verbindungsverlust: verbindlicher Ablauf

1. JMRI besitzt in AUTO die Bedienhoheit. Ein laufender Auftrag wurde zuvor vom FY-Kern validiert und angenommen.
2. Heartbeat-/Sitzungs-Timeout: Kommunikationsstatus = **JMRI_VERLOREN**. Die bisherige Bedienhoheit wird **nicht automatisch** an Browser oder LOCAL übertragen.
3. Ein bereits **laufender** Auftrag darf – sofern kein unabhängiger STOP oder Sicherheitsfehler eintritt – **regulär bis zum Ziel** zu Ende fahren. Der FY führt **keine weitere, vormals vorgemerkte Folgefahrt** automatisch aus. Eventuelle noch nicht ausgeführte Aufträge/Queues werden gesperrt bzw. verworfen, damit nach Wiederkehr keine alten Bewegungsbefehle anlaufen.
4. Im Stillstand: Bühnenposition und gültige Referenz beibehalten; Motorverhalten wie bei normal beendetem Auftrag; **wartend, keine neue reguläre Fahrt**.
5. Browser bleiben im **sichtbaren Anzeigemodus**, zeigen `AUTO – JMRI-Verbindung verloren` und erlauben keine eigenmächtige Fahrt.
6. Nächste Aktion nur nach **erneut validierter Bedienhoheit**:
   - **JMRI kehrt zurück:** neu authentifizieren, aktuellen FY-Status abfragen, AUTO-Bedienhoheit erneut bestätigen, danach **neue** Befehle senden; kein automatisches Fortsetzen alter Sequenzen.
   - **LOCAL übernimmt:** expliziter zulässiger örtlicher Übernahmevorgang im bestätigten Stillstand.
   - **Browser übernimmt:** ausdrücklich autorisierte Übernahme nach FERN im bestätigten Stillstand; keine stillschweigende Freigabe allein durch Verbindungsverlust.
7. **POR** setzt die Sitzungs-/Bedienhoheitsinformation zurück und startet wie oben beschrieben in LOCAL.

### Übersicht

| Ereignis | Auswirkung auf Motor/Auftrag | Bedienhoheit / Anzeige |
| --- | --- | --- |
| JMRI authentifiziert sich und übernimmt zulässig | Kein Fahrbefehl allein durch Anmeldung | AUTO; Browser nur Anzeige |
| JMRI-Lebenszeichen fehlt, Bühne fährt | **Aktuellen Auftrag beenden**; vorhandene Sicherheitsregeln bleiben wirksam | AUTO / JMRI_VERLOREN; weitere SET gesperrt |
| JMRI-Lebenszeichen fehlt, Bühne steht | **Stehen bleiben und warten**; Referenz/Position behalten | AUTO / JMRI_VERLOREN; Browser nur Anzeige |
| JMRI startet neu / meldet sich wieder | **Kein** selbsttätiger Fahrbefehl | Neu authentifizieren, Zustand lesen, Übernahme bestätigen |
| JMRI meldet sich geordnet ab | Nur im bestätigten Stillstand übergeben | Bedienhoheit nach definierter Freigabe zurückgeben |
| Keyboard/Browser möchte übernehmen | Laufende Fahrt **nicht** wegen Übernahmeanfrage stoppen | Übernahme erst nach Autorisierung und bestätigtem Stillstand |
| STOP / Endlage / Motorfehler | Bestehende sicherheitsrelevante Reaktion, **unabhängig vom JMRI-Timeout** | STOP immer vorrangig, Referenzregeln gemäß Kernvertrag |
| POR | Keine alten Befehle / Sitzungen weiterverwenden | LOCAL; neue JMRI-Anmeldung erforderlich |

## 4. Architekturgrenze / Umsetzung

- Ein FY-Kern mit bestehenden GET-/SET-Kommandos, ACK/NACK und dem priorisierten STOP-Pfad; **kein zweites Bewegungsprogramm** in Browser oder JMRI.
- Ein HTTP-Server kann Browser und JMRI bedienen. Transport/URL-Pfad sind **keine verlässliche Identität**. Herkunft wird erst nach der jeweils vorgesehenen Autorisierung in der internen Command-Struktur festgelegt; Anmelde- und Fahrfreigabe sind getrennt zu prüfen.
- **Seed & Key / Challenge-Response** zur JMRI-Authentifizierung wird favorisiert, aber konkretes Verfahren (z. B. HMAC mit Einmal-Nonce, Sitzung, Timeout, Wiederholschutz) bleibt **Entwurfsarbeit**. Ein bloßes Schlüsselwort, IP-Adresse oder frei gesetzte `source=JMRI` genügt nicht; HTTP ohne TLS schützt nicht vor aktivem Mitlesen/Manipulieren durch Netzteilnehmer.
- Der FY selbst validiert `source`, `owner`, `mode`, REF, BUSY, STOP und den zulässigen Zielbefehl; auch direkt eingehende HTTP-SET-Aufrufe müssen die Freigabe erfüllen.
- JMRI-/Browser-Status muss zwischen `CONNECTED`, `LOST`, `BUSY`, `WAITING` und bestätigtem Stillstand unterscheiden. **HTTP-ACK** bedeutet angenommen, nicht Bewegung abgeschlossen.

## 5. Noch offen – vor der Implementation entscheiden

- Konkreter Heartbeat-/Lease-Zeitraum, Umgang mit kurzem Netzflattern und erneuter Authentifizierung.
- Verhalten bei Übernahme von aktivem FERN durch JMRI bzw. bewusstem Rückwechsel von AUTO zu LOCAL; Autorisierung am Keyboard, im Browser und bei geordneter JMRI-Abmeldung.
- Authentifizierungs-/Sitzungsverfahren inkl. Schutz gegen Replay und Zuordnung von parallelen HTTP-Anfragen.
- Exaktes Queue-Verhalten bei Timeout während laufendem Auftrag, Abbruch wartender/fremder Kommandos sowie Request-ID-Eindeutigkeit.
- Browseranzeige und Zustandsmeldungen, einschließlich veralteter/unbestätigter Daten; JMRI-Fahrstraßen- und Signalverriegelung bleibt vom FY-Status abhängig.

## 6. Abnahmetests (später)

1. POR ohne WLAN/JMRI: LOCAL bedienbar, keine alten Aufträge; REF-Pflicht korrekt.
2. JMRI angemeldet: Browser zeigt AUTO/Anzeigemodus; WEB-SET wird vom Kern abgelehnt.
3. Während `SET_TRACK` JMRI/Netz trennen: Auftrag fährt **zum gültigen Ziel**, Stillstand und Position bleiben erhalten; keine nachfolgenden Aufträge starten.
4. Nach Timeout: ungeprüfter Browser-SET / alter JMRI-Auftrag abgewiesen, Status GET weiterhin möglich.
5. JMRI-Neustart: neue Authentifizierung und Zustandsabfrage, **kein Replay**.
6. Explizite lokale oder Web-Übernahme: erst nach Stillstand und Freigabe.
7. Während dieser Fälle STOP/Endlagen/Fehler auslösen: bestehende Sicherheitslogik hat Vorrang.

> **Wichtig:** Dies dokumentiert den gemeinsam abgestimmten Soll-Vertrag, **keine bereits fertiggestellte oder hardwaregetestete Implementierung**.
