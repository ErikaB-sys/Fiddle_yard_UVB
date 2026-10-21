# ESP32 NodeMCU / AZ-Delivery – 38 Pin, CP2102 (HW3-Quellen)

**Recherchiert 21.10.2026.** Originale des Herstellers/Anbieters, passend zur angebotenen **ESP32 NodeMCU Dev Kit C V2 / CP2102 / 38 Pin**-Variante. **Vor Übernahme physisches Board mit Abbildung und Bestückung vergleichen** (AZ bietet auch CH340 und V4 an).

## Originaldokumente

| Dokument | Quelle | Lokal im Repository? |
| --- | --- | --- |
| [AZ-Delivery Schaltplan – „Schematic korr.“ (PDF)](https://cdn.shopify.com/s/files/1/1509/1638/files/ESP-32_NodeMCU_Developmentboard_Schematic_korr.pdf?v=1666000508) | [Produktseite](https://www.az-delivery.de/products/esp32-developmentboard) | **Nein** – Original-URL gesichert |
| [AZ-Delivery Pinout (PDF)](https://cdn.shopify.com/s/files/1/1509/1638/files/ESP-32_NodeMCU_Developmentboard_Pinout.pdf?v=1609851295) | [Produktseite](https://www.az-delivery.de/products/esp32-developmentboard) | **Nein** – Original-URL gesichert |
| [AZ-Delivery Datenblatt (PDF)](https://cdn.shopify.com/s/files/1/1509/1638/files/ESP_-_32_NodeMCU_Developmentboard_Datenblatt_AZ-Delivery_Vertriebs_GmbH_10f68f6c-a9bb-49c6-a825-07979441739f.pdf?v=1598356497) | [Produktseite](https://www.az-delivery.de/products/esp32-developmentboard) | **Nein** – Original-URL gesichert |
| [Espressif ESP32 DevKitC V4, Power Supply Options](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-devkitc/user_guide.html#power-supply-options) | Espressif (Vergleichsreferenz; **anderes Board-Layout**) | Web-Referenz |

**Archivierungsstatus:** Bei der Recherche konnten die Links und die PDF-Seiten online eingesehen werden, jedoch keine PDF-Binärkopien ins Repository übertragen werden. Diese Markdown-Datei ist **eine gesicherte Quellenliste, kein Ersatz für offline archivierte PDF-Dateien**. Die drei Original-PDFs bei Gelegenheit hier ablegen; Dateinamen z.B. `AZ_ESP32_NodeMCU_{Schematic,Pinout,Datenblatt}.pdf`.

## Elektrischer Befund für HW3

Im AZ-Delivery-Schaltplan: **AMS1117-3.3**, Eingang von der 5-V-Schiene, Ausgang auf dem mit `3.3V` benannten Netz; der `3V3`-Header ist direkt damit verbunden. USB-VBUS wird über die im Schaltplan eingezeichnete Diode zum 5-V-Versorgungspfad geführt. **Auf dem 3,3-V-Ausgang ist keine eigenständige Versorgungsentkopplung für parallelen DC/DC-Betrieb erkennbar.**

Daraus folgt: **Externe 3,3 V am Header und USB-Versorgung nicht gleichzeitig ohne zusätzliche geeignete Entkopplung/Versorgungsumschaltung betreiben.** Auch bei ausgeschalteten 12 V kann USB den unbestromten externen DC/DC-Wandler über dessen *Ausgang* rückspeisen, sofern beide Ausgänge fest verbunden bleiben. Ob dies im konkreten Wandler zulässig wäre, ist nicht nachgewiesen.

Espressif warnt für sein DevKitC ausdrücklich, immer **nur eine** Versorgung aus USB, 5 V oder 3V3 zu verwenden. Die AZ-Delivery-V2-Schaltung ist nicht identisch zur DevKitC-V4-Schaltung; der Hinweis stützt die Vorsicht, ersetzt aber keinen Vergleich.

## Hardware-Entscheidung noch offen

[HW3 Versorgung und Programmierbetrieb](../../FY_ESP32_Bringup/docs/pinouts/HW3_STROMVERSORGUNG.md) und [#161](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/161). Wir planen eine mechanisch eindeutig umschaltbare Einspeisung mit **USB-only bei Flash/Reset und externer Versorgung only im Betriebsmodus**. Trennung der Motor-VMOT ist ein *zusätzlicher*, unabhängiger Schutz.
