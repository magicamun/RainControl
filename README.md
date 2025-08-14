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
  - Widerstände (2x 10k, 2x 330 Ohm)
  - 10uF Elko
  - LED - Rot, 3mm
  
  Display
  - 2,42" 128X64-OLED-Anzeigemodul SPI - 2.42 Zoll OLED Bildschirm kompatibel mit Arduino UNO R3 - Weiße Schrift OLED :
  (https://www.az-delivery.de/products/oled-2-4-white?variant=44762703986955&utm_source=google&utm_medium=cpc&utm_campaign=16964979024&utm_content=166733588295&utm_term=&gad_source=1&gbraid=0AAAAADBFYGXj7s2C_h3TASz0DupxomgUw&gclid=EAIaIQobChMI-YCAy9LmjAMVH5CDBx0H8zKnEAQYAyABEgLpafD_BwE)
  
  Schraubklemmblöcke zum Anschluss von 
  - 3 x 2 polig (Sonde Zisterne, Spannungsversorgung 24V, Stromsensor SCT 013)
  - 1 x 3-polig (Umschaltung Regenwasserpumpe, Übersteuerung Schwimmerschaltung - ACHTUNG - 230V)
  
  Zur Verbindung von Controllerplatine mit der Displayplatine:
  
  2 x Wannenstecker [8polig](https://www.conrad.de/de/p/bkl-electronic-10120552-stiftleiste-ohne-auswurfhebel-rastermass-2-54-mm-polzahl-gesamt-8-anzahl-reihen-2-1-st-741552.html?hk=SEM&WT.mc_id=google_pla&utm_source=google&utm_medium=cpc&utm_campaign=DE+-+PMAX+-+Nonbrand+-+Learning&utm_id=20402606569&gad_source=1&gad_campaignid=20392975386&gbraid=0AAAAAD1-3H7MMs8aUj1DVyRg7EW9yTu3l&gclid=CjwKCAjwkvbEBhApEiwAKUz6--gW1qskdMad7NtT7sFPgiCQdok8dU4I2qlc02XzfEHAeNsRRdUxyhoCCR4QAvD_BwE)
  2 x Pfostensteckverbinder mit Zugentlastung https://www.conrad.de/de/p/tru-components-1580952-pfosten-steckverbinder-mit-zugentlastung-rastermass-2-54-mm-polzahl-gesamt-8-anzahl-reihen-2-1-1580952.html?hk=SEM&WT.mc_id=google_pla&utm_source=google&utm_medium=cpc&utm_campaign=DE+-+PMAX+-+Nonbrand+-+Learning&utm_id=20402606569&gad_source=1&gad_campaignid=20392975386&gbraid=0AAAAAD1-3H7MMs8aUj1DVyRg7EW9yTu3l&gclid=CjwKCAjwkvbEBhApEiwAKUz6--gW1qskdMad7NtT7sFPgiCQdok8dU4I2qlc02XzfEHAeNsRRdUxyhoCCR4QAvD_BwE
  1 x Flachbandkabel 8 polig
  
  
Platinenlyout mittels KiCad - 2 Projekte - eines für das Display, eines für den Controller.

Funktionsweise technisch:

Füllstand:
  Die Sonde wird mit 24V betrieben und da bietet sich ein Netzteil handelsübluch an und eine Spannungswandlung (TSR-1 2433) von den 24V DC auf 3,3V für den Rest der Schaltung (Arduino MKR Zero, Relais, Display).
  Die Sonde liefert einen Strom in Abhängigkeit vom Druck (Füllstandshöhe). Der Wandler macht aus dem Strom eine Ausgangsspannung von 0 bis Vmax (3,3V).
  Diese Spannung wird am A0 (AnalogInput 0) des Arduino erfasst und in einen Füllstand umgerechnet - der Füllstand (Volumen) ist proportional zum gemessenen Wert an A0. (Voraussetzung ist ein Behälter, dessen Füllstand (Volumen) sich linear proportional zum Druck verhält (Senkrechter Zylinder).
  
  Der Wandler für den Messwert an A0 muss vor Inbetriebnahme des Systems kalibiriert werden.
  ACHTUNG: Die Kalibrierung muss ohne angeschlossenen Arduino erfolgen um Beschädigungen durch Überspannung am Eingang des Arduino zu vermeiden.
  -> Siehe Kalibrierung und Inbetriebnahme.
  
  In Abhängigkeit von der gemessenen Füllhöhe und zwei einzustellenden Werten (LimitLow, LimitHigh) schaltet der Arduino das Relais sowie eine LED über PIN D14 an oder aus. Damit bei unterem Schwellwert / Minimum nicht sofort bei erneutem überschreiten (Nachlauf Regen) wieder zurück geschaltet wird, gibt es einen weiteren Wert (Oberer Schwellwert) der erreicht werden muss, damkt zurück auf Zisterne geschlatet wird.

Strom, Pumpe an oder aus:
  Über den Stromsensor SCT013 kann der Zustand der Pumpe (Regenwassernutzung) abgefragt werden, ohne in die Anlage eingreifen zu müssen. Dazu wird der Stromsensor unter der Beachtung der Stromflussrichtung um die Phase (nur die Phase) zur Pumpe gelegt. Am Ausgang des Sensors wird über Induktion eine Wechselspannung abgegeben. Dabei sind die Werte des Sensors zur korrekten Berechnung des Stroms und der Leistung in der Software einzustellen. Wichtig ist eine korrekte Dimensionierung des Sensors - es gibt unterschiedliche Typen. Für eine möglichst feine Auflösung ist der Ampere-Wert des Sensors möglichst klein zu wählen - aber groß genug, dass bei Vollast der Pumpe der Wert der Leistung ausreicht. Der hier benutzte Sensor misst bis 5A -> 1,1kw. Meine Pumpe hat 800 Watt Leistungsaufnahme. Gleichzeitig soll der Sensor möglichst große Spannungswerte am Ausgang produzieren, damit die Auflösung des Wertes möglichst fein/genau ist. Es gibt die Sensoren mit 0,33V und 1V Ausgangsspannung -> ich nutze hier 1V. Da die Spannung eine Wechselspannung ist (mit diesem Sensor im Bereich von +1V und -1V) wird über den Spannungsteiler am Sensor  die Wechselspannung um +1,65V angehoben. Der analoge Wert an A1 wird in einen Wert für den Stromverbrauch umgerechnt. Dazu ist es wichtig, den Messbereich des Sensors, die Ausgangsspannung des Sensors und den Referenzwert am Arduino (Zero 3,3V) zu kennen und in der Software einzustellen.

Am Pin D13 ist ein Taster von der Displayplatine angeschlossen, mit dem der Betriebsmodus der Steuerung manuell gesetzt werden kann:
  - Auto - Der Arduino übernimmt die Ventilsteuerung (Relais und Ansteuerung Schwimmerschalter der Pumpe) in abhängigkeit des Füllstandes
  -   Füllstand < LimitLow -> Relais auf "Hauswasser"
  -   Füllstand > LimitHigh -> Relais auf "Zisterne"
  -   Wenn der Füllstand kleiner als LimitLow war und immernoch kleiner als LimitHigh ist -> Ventil auf Hauswasser
  - Manuell Zisterne -> Relais auf "Zisterne"
  - Manuell Hauswasser -> Relais auf "Hauswasser"

  Per Tasterdruck wird zyklisch durch die Betriuebsmodi geschaltet, wobei die tatsächliche Umschaltung erst nach 10Sekunden ohne erneute Tasterbetätigung erfolgt

Das Display zeigt Datum, Uhrzeit, Füllstand und Betriebsmodus an

Mit dem an RST angeschlossenen Taster lässt sich der Controller resetten.

Funktionsweise Software:

Sämtliche Systemzustände werden bei Veränderung alle Sekunde an einen MQTT-Broker gesendet, spätestens alle 30 Sekunden. Das System führt folgende Werte:

- Timestamp  (Datum, Uhrzeit)
- Mode       (0 = Zisterne, 1 = Hauswasser, 2 = Auto) -> Betriebsmodus
- Valve      (0 = Zisterne, 1 = Hauswasser) -> Ventilstellung
- Analog     (0-1023) -> Analoger Messwert der Pegelsonde (0-1023)
- Liter      -> Füllung in Liter
- LimitLow   -> Unterer Schwellwert in Liter
- LimitHigh  -> Oberer Schwellwert in Liter
- LiterMax   -> maximales Füllvolumen wird benötigt zur Umrechnung des analogen Wertes in Liter (Analog = LiterMax / AnalogMax)
- Prozent    -> Füllstand in %
- Uptime     -> Zeit seit l. Boot/Neustart
- StatusSonde-> Ist eine Sonde korrekt angeschlosen (Wenn der Analoge Wert unter einem Minimum liegt, dann ist davon auszugehen, dass die Sonde nicht korrekt angeschlossen ist
- Ventil     -> Ventilstellung als Text ("Zisterne", "Hauswasser")
- Modus      -> Modus als Text
- HyaRain    -> Pumpe läuft (ON), oder nicht (OFF)
- Booted     -> Boot-Zeitpunkt
- AnalogMin  -> mindestwert, den die Sonde liefern muss, damit Sonde als vorhanden erkannt wird
- AnalogMax  -> Maximaler analoger Wert an A0 für volle Zisterne
- Current    -> Stromverbrauch in Ampere
- Power      -> Watt (Volt * Leistung pro Sekunde
- KWh        -> KLilowattstunden
- Reason     -> Grund für Ventilstellung (0 = "Manuell", 1 = "Zisterne voll", 2 = "Zisterne leer")
- Reasontext -> Reason als Text

Per MQTT kann der Controller aber auch gesteuert werden:
 - cmd/Mode [0,1,2] -> Es kann der Modus umgestellt werden (nicht nur per Taster)
 - cmd/Limit [low|high]=<Number> -> Einstellen der Schwellwerte 
 - cmd/Calibrate [analogmin|analogmax|litermax]=<number> -> Einstelen der Kalibrierung in der Software nach dem die Sonde und der Wandler kalibriert wurden

Bei eingelegter SD-Card werden die Daten über einen Reset hinaus auf der SD-Card gespeichert und nach Neustart gelesen, wenn man darauf verzichtet, die Werte nach dem Aufspielen der Software und der Einstellung in der Software zu verändern ist eine SD-Card nicht nötig

Nach dem Zusammenbau, der Bestückung der Platine erfolgt schrittweise die Inbetriebnahme - noch ohne Arduino. Buchsenleisten sind nützlich für den Arduino (quasi ein muss) und für den Strom-/Spannungswandler. Den Strom/Spannungswandler befreie ich von den Schraubklemmen (auslöten) und ersetze die durch Stiftleisten - einfacher Tausch. Dazu bekommt die Platine je Gegenstück Buchsenleiste 2 polig und 3 polig mit 5er Raster.

1. Prüfung der Spannungen
   An Pin 3 des Strom/Spannungswandlers müssen 24V anliegen (auf dem Board rechte Buchsenleiste oberer Pin/Buchse)
   Am Pin 3 der Stiftleiste für den Arduino müssen gegenüber GND (Pin 4) 3,3Volt anliegen
2. Einstellen des Strom/Spannungswandlers (idealerweise bei voller Zisterne)
   Zum Einstellen des Stromspannungswandlers wird an der Stiftleiste "Probe" ein Multimeter zur Spannungsmessung angeschlossen. Ohne Sonde liefert das Multimeter dann 0V. Dann wird die Sonde angeschlossen und noch nicht in die Zisterne herabgelassen - Nullpunkteinstellung für "leer". Mit dem linken Poti dreht man jetzt solange nach linke (kleiner) oder nach rechtes (größer), bis die gemessene Spannung verlässlich nah an 0V liegt aber eben noch größer als 0V ist. Jetzt kann die Sonde in die Zisterne gelassen werden - so tief, wie möglich. Zur korrekten Kalibrierung des Maximalwertes ist es am einfachsten den maximalen Füllstand auch zur EInstellung der maximalen Spannung zu nutzen. Dazu dreht man jetzt am rechten Poti des Wandlers (links kleiner, rechts größer) bis die gemessene Spannung nah bei 3,3V ist. Es empfiehlt sich etwas "Luft" zu lassen. Wenn die Zisterne nicht voll ist, sollte man abschätzen können, wie voll (in Prozent) sie ist, man kann dann ausgehend vom Zielwert bei "Zisterne Voll" nah bei 3,3V und dem unteren Messwert für leer den Spannungswert, den man jetzt messen möchte im Dreisatz ableiten.
4. Anpassen der Grundeinstellungen in der Software
5. Aufspielen der Software auf den Arduino
6. Board stromlos machen / Spannungsversorgung trennen
7. Aufstecken des Arduino und des Ethernet-Shields auf die Buchsenleisten 14 polig
8. Verbinden des Displays mit dem Controller
9. Netzteil anschließen
10. Der Arduino bootet und auf dem Display werden die Daten angezeigt
11. Die Daten werden zyklisch an den MQTT-Broker verteilt.
12. Prüfung der Messwerte am Broker
13. Es empfiehlt sich, dass nach ein paar Wochen Betrieb und Leer, Voll-Zyklen nochmal die Messwerte der Kalibrierung geprüft werden und notfalls nochmal in der Software neu eingestellt werden - entweder per MQTT oder "hart" per Software



