# RainControl
Füllstandsmessung einer Zisterne, (Über-)steuerung des Schwimmerschalters der Regenwasserpumpe in Abhängigkeit des Füllstands. 
Messung Strom, Verbrauch der Pumpe, Ableitung an/aus. Anbindung an einen MQTT-Broker zur Werteerfassung und Steuerung

Das Projekt basiert auf der Arbeit aus diesem Repo:

https://github.com/Eisbaeeer/Arduino.Ethernet.Zisterne

Hardware:
Arduino MKR Zero (https://store.arduino.cc/collections/mkr-family/products/arduino-mkr-zero-i2s-bus-sd-for-sound-music-digital-audio-data) - Die Leistung des Nano (insbesondere Speicher) hat nicht ausgereicht die Steuerung ohne Abstriche so erweitert umzusetzen

MKR Ethernet-Shield (https://store.arduino.cc/collections/mkr-family/products/arduino-mkr-eth-shield) - Zur Anbindung an eine Hausatomation, bzw. an einen MQTT-Broker (IOBroker)

Pegelsonde zur Messung des Füllstands mittels Druck - Voraussetzung zur korrekten Füllstandsermittlung ist ein Höhensymmetrisches Volumen (aufrecht stehender Zylinder, Quader), Liegende Zylinderformen sind nicht geeignet. Z.B. so eine hier:

Füllstandssensor Pegelsensor Drucksensor Wasserstandsensor 4-20mA 0-5m DE

Stromsensor SCT013 - 5A, 1V (https://www.amazon.de/HUABAN-Nicht-invasiver-Split-Core-Stromwandlersensor/dp/B089FNQ5GX?th=1)

Bauteile für die Schaltung (https://www.conrad.de/de/service/wishlist.html?sharedId=8f9d7077-cee6-41a0-b912-274e74244384)

Steckernetzteil 24V DC
- Spannungswandler [TSR-1 2433 (Wandlung Eingangsspannung 24V auf 3,3V](https://www.conrad.de/de/p/tracopower-tsr-1-2433-dc-dc-wandler-print-24-v-dc-3-3-v-dc-1-a-75-w-anzahl-ausgaenge-1-x-inhalt-1-st-156671.html) zur Versorgung der Schaltung mit 3,3V
- Strom/Spannungswandler zur Wandlung des Stroms der Sonde in Spannung inkl. Kalibrierung von 0-Punkt und Max (Ali - Express: Strom-Spannungs-Modul 0-20 mA/4-20 mA bis 0-3,3 V/0 -5V/0 -10V Spannungssender-Signal wandler modul)

- Relais [36.11 Schaltspannung 3V](https://www.conrad.de/de/p/finder-36-11-9-003-4011-printrelais-3-v-dc-10-a-1-wechsler-1-st-3323202.html)
- Steckerleisten 14 polig
- Transistor BC337 (Schaltung Relais und LED
- Diode (1N 4148)
- Widerstände
-   2x 10k
-   2x 330 Ohm
Elko:
- 10uF
LED:
- Rot, 3mm

Display
- 2,42" 128X64-OLED-Anzeigemodul SPI - 2.42 Zoll OLED Bildschirm kompatibel mit Arduino UNO R3 - Weiße Schrift OLED :
https://www.az-delivery.de/products/oled-2-4-white?variant=44762703986955&utm_source=google&utm_medium=cpc&utm_campaign=16964979024&utm_content=166733588295&utm_term=&gad_source=1&gbraid=0AAAAADBFYGXj7s2C_h3TASz0DupxomgUw&gclid=EAIaIQobChMI-YCAy9LmjAMVH5CDBx0H8zKnEAQYAyABEgLpafD_BwE

Schraubklemmblöcke zum Anschluss von 
- 3 x 2 polig (Sonde Zisterne, Spannungsversorgung 24V, Stromsensor SCT 013)
- 1 x 3-polig (Umschaltung Regenwasserpumpe, Übersteuerung Schwimmerschaltung - ACHTUNG - 230V)

Zur Verbindung von Controllerplatine mit der Displayplatine:

2 x Wannenstecker 8polig
2 x Buchsenstecker 8 polig 
1 x Flachbandkabel 8 polig


Platinenlyout mittels KiCad - 2 Projekte - eines für das Display
