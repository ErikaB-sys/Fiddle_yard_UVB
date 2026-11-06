#!/usr/bin/env python3
"""Lokaler PlatformIO-Uploader fuer das FY-ESP32-HTTP-OTA-Protokoll.

Nur im vertrauten LAN verwenden. Ein Upload erfordert zuvor eine
physische Freigabe per roter STOP-Taste an der Schiebebuehne.
"""
from __future__ import annotations

import argparse
import http.client
import json
import re
import sys
import time
import uuid
from pathlib import Path
from urllib.parse import quote, urlencode, urlsplit

CHUNK_SIZE = 8192
MAX_REPLY = 8192


class OtaError(Exception):
    """OTA-Vorbereitung oder -Uebertragung fehlgeschlagen."""


def parse_host(raw: str) -> tuple[str, int]:
    value = raw.strip()
    if not value or value.upper() in {"SET_FY_IP", "YOUR_IP", "0.0.0.0"}:
        raise OtaError(
            "Bitte in platformio.ini unter [env:esp32dev] bei "
            "'custom_ota_host' die IP der echten FY-Buehne eintragen."
        )
    uri = urlsplit(value if "://" in value else "http://" + value)
    if (
        uri.scheme != "http"
        or not uri.hostname
        or uri.username is not None
        or uri.password is not None
        or uri.path not in ("", "/")
        or uri.query
        or uri.fragment
    ):
        raise OtaError("OTA-Ziel muss eine lokale HTTP-IP/Hostname ohne Pfad sein.")
    try:
        port = uri.port or 80
    except ValueError as exc:
        raise OtaError("Ungueltige Portnummer") from exc
    return uri.hostname, port


def request(host: str, port: int, method: str, path: str,
            data: bytes | None = None,
            headers: dict[str, str] | None = None,
            timeout: float = 20.0) -> tuple[int, bytes]:
    connection = http.client.HTTPConnection(host, port, timeout=timeout)
    try:
        connection.request(method, path, body=data, headers=headers or {})
        response = connection.getresponse()
        if response.length is not None and response.length > MAX_REPLY:
            raise OtaError("Antwort der FY ist unerwartet gross.")
        body = response.read(MAX_REPLY + 1)
        if len(body) > MAX_REPLY:
            raise OtaError("Antwort der FY ist unerwartet gross.")
        return response.status, body
    finally:
        connection.close()


def get_json(host: str, port: int, path: str) -> dict:
    status, body = request(host, port, "GET", path,
                           headers={"Accept": "application/json"})
    if status != 200:
        raise OtaError(f"{path}: HTTP {status}: {body[:300]!r}")
    try:
        decoded = json.loads(body)
    except (json.JSONDecodeError, UnicodeDecodeError) as exc:
        raise OtaError("FY liefert keine gueltige JSON-Antwort.") from exc
    if not isinstance(decoded, dict):
        raise OtaError("FY meldet einen unerwarteten Status.")
    return decoded


def check_firmware(path: Path) -> int:
    if path.suffix.lower() != ".bin" or not path.is_file():
        raise OtaError("Es wird eine existierende PlatformIO firmware.bin benoetigt.")
    size = path.stat().st_size
    if size < 4096:
        raise OtaError("Firmware-Datei ist zu klein.")
    with path.open("rb") as firmware:
        if firmware.read(1) != b"\xe9":
            raise OtaError("Keine gueltige ESP32-Image-Kennung (0xE9).")
    return size


def prepare(host: str, port: int, csrf: str, size: int) -> None:
    form = urlencode({"csrf": csrf, "size": str(size)}).encode("ascii")
    status, body = request(
        host, port, "POST", "/api/v1/ota/prepare", form,
        {"Content-Type": "application/x-www-form-urlencoded",
         "Content-Length": str(len(form))}
    )
    if status != 200:
        raise OtaError(
            f"OTA-Freigabe abgelehnt (HTTP {status}): "
            + body.decode("utf-8", errors="replace")[:400]
        )


def stream_image(host: str, port: int, csrf: str, path: Path, size: int) -> None:
    boundary = "FYOTA" + uuid.uuid4().hex
    head = (
        "--" + boundary + "\r\n"
        'Content-Disposition: form-data; name="firmware"; filename="firmware.bin"\r\n'
        "Content-Type: application/octet-stream\r\n\r\n"
    ).encode("ascii")
    tail = ("\r\n--" + boundary + "--\r\n").encode("ascii")
    length = len(head) + size + len(tail)
    url = "/api/v1/ota/upload?csrf=" + quote(csrf, safe="")
    connection = http.client.HTTPConnection(host, port, timeout=90)
    try:
        connection.putrequest("POST", url)
        connection.putheader("Content-Type", "multipart/form-data; boundary=" + boundary)
        connection.putheader("Content-Length", str(length))
        connection.putheader("Connection", "close")
        connection.endheaders()
        connection.send(head)
        sent = 0
        last_step = -1
        with path.open("rb") as image:
            while True:
                block = image.read(CHUNK_SIZE)
                if not block:
                    break
                connection.send(block)
                sent += len(block)
                step = int(sent * 100 / size) // 5
                if step != last_step:
                    last_step = step
                    print(f"\rFY OTA: {min(100, step * 5):3d}% "
                          f"({sent:,}/{size:,} Bytes)", end="", flush=True)
        connection.send(tail)
        print("\nFirmware uebertragen. Warte auf Flash-/Image-Pruefung ...", flush=True)
        response = connection.getresponse()
        body = response.read(MAX_REPLY + 1)
        if len(body) > MAX_REPLY:
            raise OtaError("OTA-Antwort ist unerwartet gross.")
        if response.status != 200:
            raise OtaError(
                f"OTA fehlgeschlagen: HTTP {response.status}: "
                + body.decode("utf-8", errors="replace")[:400]
            )
        try:
            outcome = json.loads(body)
        except (json.JSONDecodeError, UnicodeDecodeError) as exc:
            raise OtaError("OTA-Erfolgsantwort hat ein ungueltiges Format.") from exc
        if not isinstance(outcome, dict) or outcome.get("result") != "ACK":
            raise OtaError("FY hat den Firmware-Upload nicht positiv bestaetigt.")
    finally:
        connection.close()


def main() -> int:
    parser = argparse.ArgumentParser(description="FY ESP32 – PlatformIO HTTP OTA")
    parser.add_argument("--host", required=True, help="IP des FY (nicht Setup-AP)")
    parser.add_argument("--file", required=True, type=Path, help="PlatformIO firmware.bin")
    args = parser.parse_args()

    try:
        host, port = parse_host(args.host)
        size = check_firmware(args.file)
        print(f"FY OTA: Ziel http://{host}:{port}, Firmware {size:,} Bytes", flush=True)
        state = get_json(host, port, "/api/v1/ota/status")
        version = str(state.get("firmware", ""))
        match = re.fullmatch(r"(\d+)\.(\d+)\.(\d+)", version)
        if not match or tuple(int(p) for p in match.groups()) < (3, 2, 0):
            raise OtaError(
                f"Ziel-Firmware '{version}' unterstuetzt den FY-HTTP-OTA-Workflow nicht."
            )
        if not state.get("supported") or int(state.get("slot_bytes", 0)) < size:
            raise OtaError("Kein passender OTA-App-Slot oder Datei zu gross.")
        if state.get("active") or state.get("prepared"):
            raise OtaError("Am FY laeuft bereits ein OTA-Vorgang.")
        if not state.get("armed"):
            print(
                "Firmware gebaut. Jetzt an der FY die rote STOP-Taste "
                "3 Sekunden halten (Motor steht). Warte auf Freigabe ...",
                flush=True,
            )
            for _ in range(45):
                time.sleep(2)
                state = get_json(host, port, "/api/v1/ota/status")
                if state.get("armed"):
                    print("Physische OTA-Freigabe erkannt.", flush=True)
                    break
                if state.get("active") or state.get("prepared"):
                    raise OtaError("FY meldet einen parallelen OTA-Vorgang.")
            else:
                raise OtaError(
                    "Innerhalb von 90 Sekunden keine physische STOP-Freigabe. "
                    "Erneut OTA Upload starten."
                )
        csrf = state.get("csrf")
        if not isinstance(csrf, str) or len(csrf) != 16:
            raise OtaError("FY liefert kein gueltiges OTA-Formular-Token.")
        try:
            network = get_json(host, port, "/api/v1/network/status")
            print(f"FY WLAN-STA-MAC: {network.get('sta_mac', '?')}"
                  f" | IP: {network.get('ip', '?')}", flush=True)
        except (OtaError, OSError):
            print("Hinweis: FY-Netzwerkdetails nicht abrufbar.", flush=True)

        print(f"Freigabe erkannt; Firmware wird an FY {version} gesendet ...",
              flush=True)
        prepare(host, port, csrf, size)
        stream_image(host, port, csrf, args.file, size)
        print("FY OTA: ACK erhalten. Der ESP32 startet in wenigen Sekunden neu.")
        return 0
    except (OtaError, OSError, TimeoutError, ValueError) as exc:
        print(f"\nFY OTA FEHLER: {exc}", file=sys.stderr, flush=True)
        print("Kein automatischer Wiederholversuch. Bei Verbindungsverlust "
              "Firmware-/OTA-Status am FY kontrollieren.", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
