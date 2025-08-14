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
- Spannungswandler [TSR-1 2433 (WaNdlung Eingangsspannung 24V auf 3,3V](https://www.conrad.de/de/p/tracopower-tsr-1-2433-dc-dc-wandler-print-24-v-dc-3-3-v-dc-1-a-75-w-anzahl-ausgaenge-1-x-inhalt-1-st-156671.html)
- Relais [36.11 Schaltspannung 3V](https://www.conrad.de/de/p/finder-36-11-9-003-4011-printrelais-3-v-dc-10-a-1-wechsler-1-st-3323202.html)
- Steckerleisten 14 polig
- 
