# RainControl
Füllstandsmessung einer Zisterne, (Über-)steuerung des Schwimmerschalters der Regenwasserpumpe in Abhängigkeit des Füllstands. 
Messung Strom, Verbrauch der Pumpe, Ableitung an/aus. Anbindung an einen MQTT-Broker zur Werteerfassung und Steuerung

Das Projekt basiert auf der Arbeit aus diesem Repo:

https://github.com/Eisbaeeer/Arduino.Ethernet.Zisterne

Der Nano war aber am Limit was den Speicherplatz angeht und selbst kleinste Optimierungen führten zu zu großen Images oder liefen im Betrieb instabil, daher habe ich das Projekt auf anderer Hardware aufgesetzt. Zugegeben - das ist jetzt deutlich teurer aber macht so auch mehr Spaß (finde ich).

Hardware:
  [Arduino MKR Zero](https://store.arduino.cc/collections/mkr-family/products/arduino-mkr-zero-i2s-bus-sd-for-sound-music-digital-audio-data) 
  
  [MKR Ethernet-Shield](https://store.arduino.cc/collections/mkr-family/products/arduino-mkr-eth-shield) - Zur Anbindung an eine Hausatomation, bzw. an einen MQTT-Broker (IOBroker)
  
  Pegelsonde zur Messung des Füllstands mittels Druck - Voraussetzung zur korrekten Füllstandsermittlung ist ein Höhensymmetrisches Volumen (aufrecht stehender Zylinder, Quader), Liegende Zylinderformen sind nicht geeignet. Z.B. so eine hier:
  
  [Füllstandssensor Pegelsensor Drucksensor Wasserstandsensor 4-20mA 0-5m DE](https://www.ebay.de/itm/177255784278?chn=ps&_ul=DE&_trkparms=ispr%3D1&amdata=enc%3A10OVVLQ0TSaWD2zqcidwIyQ23&norover=1&mkevt=1&mkrid=707-134425-41852-0&mkcid=2&mkscid=101&itemid=177255784278&targetid=2381626844604&device=c&mktype=pla&googleloc=9113680&poi=&campaignid=21674357977&mkgroupid=167855164998&rlsatarget=pla-2381626844604&abcId=10011854&merchantid=5588616187&gad_source=1&gad_campaignid=21674357977&gbraid=0AAAAAD_G4xa4yudELXTO8UWhM-5WBP-aM&gclid=CjwKCAjwkvbEBhApEiwAKUz6-0uRbySDVDx1-_a3FiXcqrZ4Tvtlft7q06sarqXWDDbmrJJIIFPkExoCPcMQAvD_BwE)
  
  [Stromsensor SCT013 - 5A, 1V](https://www.amazon.de/HUABAN-Nicht-invasiver-Split-Core-Stromwandlersensor/dp/B089FNQ5GX?th=1)

  [Steckernetzteil 24V DC](https://www.conrad.de/de/p/dehner-elektronik-saw-30-240-1000g-steckernetzteil-festspannung-24-v-dc-1000-ma-24-w-1549855.html)

Bauteile für die Schaltung, Platine
  
  Controller
  - [Spannungswandler TSR-1 2433 (Wandlung Eingangsspannung 24V auf 3,3V](https://www.conrad.de/de/p/tracopower-tsr-1-2433-dc-dc-wandler-print-24-v-dc-3-3-v-dc-1-a-75-w-anzahl-ausgaenge-1-x-inhalt-1-st-156671.html) zur Versorgung der Schaltung mit 3,3V
  - [Strom/Spannungswandler](https://de.aliexpress.com/item/1005001604138590.html) zur Wandlung des Stroms der Sonde in Spannung inkl. Kalibrierung von 0-Punkt und Maximum
  - [Relais Finder 36.11 Schaltspannung 3V](https://www.conrad.de/de/p/finder-36-11-9-003-4011-printrelais-3-v-dc-10-a-1-wechsler-1-st-3323202.html)
  - [Steckerleisten 14 polig](https://www.conrad.de/de/p/econ-connect-buchsenleiste-standard-anzahl-reihen-1-polzahl-je-reihe-14-blg1x14-1-st-1492306.html)
  - [Transistor BC337](https://www.conrad.de/de/p/diotec-transistor-bjt-diskret-bc337-25-to-92-anzahl-kanaele-1-npn-155900.html) zur Schaltung von Relais und LED
  - D1 [Diode (1N 4148)](https://www.conrad.de/de/p/diotec-ultraschnelle-si-diode-1n4148-sod-27-75-v-150-ma-162280.html)
  - R1 [1x Widerstand 330 Ohm](https://www.conrad.de/de/p/yageo-cfr25j330rh-cfr-25jt-52-330r-kohleschicht-widerstand-330-axial-bedrahtet-0207-0-25-w-5-1-st-1417730.html)
  - R2, R3 [2x Widerstand 10k Ohm](https://www.conrad.de/de/p/yageo-cfr25j10kh-cfr-25jt-52-10k-kohleschicht-widerstand-10-k-axial-bedrahtet-0207-0-25-w-5-1-st-1417697.html)
  - [10uF Elko](https://www.conrad.de/de/p/frolyt-e-rf3058-elektrolyt-kondensator-radial-bedrahtet-2-5-mm-10-f-16-v-20-o-x-l-5-mm-x-12-mm-1-st-3046377.html)
  - 3 x [Schraubklemmblock 2 polig](https://www.conrad.de/de/p/deca-1216197-schraubklemmblock-1-50-mm-polzahl-3-blau-1-st-1216197.html) (Sonde Zisterne, Spannungsversorgung 24V, Stromsensor SCT 013)
  - 1 x [Schraubklemmblock 3-polig](https://www.conrad.de/de/p/deca-1282826-schraubklemmblock-1-50-mm-polzahl-2-blau-1-st-1282826.html) (Umschaltung Regenwasserpumpe, Übersteuerung Schwimmerschaltung - ACHTUNG - 230V)
  
  Displaypanel
  - R1 [1x Widerstand 390 Ohm](https://www.conrad.de/de/p/yageo-mf0207f390rh-mf0207fte52-390r-metallschicht-widerstand-390-axial-bedrahtet-0207-0-6-w-1-1-st-1417596.html)
  - [LED - Rot, 3mm](https://www.conrad.de/de/p/quadrios-2111o155-led-bedrahtet-rot-rund-3-mm-700-mcd-30-20-ma-2-v-2583896.html)
  - [2 Taster (Reset und Mode)](https://www.conrad.de/de/p/weltron-t604-t604-drucktaster-24-v-dc-0-05-a-1-x-aus-ein-tastend-l-x-b-x-h-6-x-6-x-9-5-mm-1-st-700479.html)
  Display
  - [2,42" 128X64-OLED-Anzeigemodul SPI - 2.42 Zoll OLED Bildschirm kompatibel mit Arduino UNO R3 - Weiße Schrift OLED :](https://www.az-delivery.de/products/oled-2-4-white?variant=44762703986955&utm_source=google&utm_medium=cpc&utm_campaign=16964979024&utm_content=166733588295&utm_term=&gad_source=1&gbraid=0AAAAADBFYGXj7s2C_h3TASz0DupxomgUw&gclid=EAIaIQobChMI-YCAy9LmjAMVH5CDBx0H8zKnEAQYAyABEgLpafD_BwE)
  

  Zur Verbindung von Controllerplatine mit der Displayplatine:
  
  2 x [Wannenstecker 8polig](https://www.conrad.de/de/p/bkl-electronic-10120552-stiftleiste-ohne-auswurfhebel-rastermass-2-54-mm-polzahl-gesamt-8-anzahl-reihen-2-1-st-741552.html?hk=SEM&WT.mc_id=google_pla&utm_source=google&utm_medium=cpc&utm_campaign=DE+-+PMAX+-+Nonbrand+-+Learning&utm_id=20402606569&gad_source=1&gad_campaignid=20392975386&gbraid=0AAAAAD1-3H7MMs8aUj1DVyRg7EW9yTu3l&gclid=CjwKCAjwkvbEBhApEiwAKUz6--gW1qskdMad7NtT7sFPgiCQdok8dU4I2qlc02XzfEHAeNsRRdUxyhoCCR4QAvD_BwE)
  
  2 x [Pfostensteckverbinder mit Zugentlastung](https://www.conrad.de/de/p/tru-components-1580952-pfosten-steckverbinder-mit-zugentlastung-rastermass-2-54-mm-polzahl-gesamt-8-anzahl-reihen-2-1-1580952.html?hk=SEM&WT.mc_id=google_pla&utm_source=google&utm_medium=cpc&utm_campaign=DE+-+PMAX+-+Nonbrand+-+Learning&utm_id=20402606569&gad_source=1&gad_campaignid=20392975386&gbraid=0AAAAAD1-3H7MMs8aUj1DVyRg7EW9yTu3l&gclid=CjwKCAjwkvbEBhApEiwAKUz6--gW1qskdMad7NtT7sFPgiCQdok8dU4I2qlc02XzfEHAeNsRRdUxyhoCCR4QAvD_BwE)
  
  1 x [Flachbandkabel 8 polig](https://www.conrad.de/de/p/econ-connect-28awg8gr-flachbandkabel-rastermass-1-27-mm-8-x-0-08-mm-grau-30-50-m-1656444.html)

  Sonstiges Material:
  - Bopla Gehäuse](https://www.conrad.de/de/p/bopla-euromas-m-231-02231000-industrie-gehaeuse-160-x-80-x-85-polycarbonat-lichtgrau-ral-7035-1-st-531785.html?hk=SEM&WT.mc_id=google_pla&utm_source=google&utm_medium=cpc&utm_campaign=DE+-+PMAX+-+Nonbrand+-+Learning&utm_id=20402606569&gad_source=1&gad_campaignid=20392975386&gbraid=0AAAAAD1-3H4SrPRlOXxmb1LvpAf_dMIkF&gclid=Cj0KCQjwnovFBhDnARIsAO4V7mCVA2QiVP_xAGgSJM6bk3FJjILpeI0nTC0QpojrSp9-wGSnNphEn0QaArxKEALw_wcB)
  - [Passende Einbaubuchse für das Netzteil](https://www.conrad.de/de/p/cliff-scd-026-niedervolt-steckverbinder-buchse-einbau-vertikal-5-5-mm-2-1-mm-1-st-735640.html)
  - [Einbaubuchse für Klinkenstecker vom SCT013](https://www.conrad.de/de/p/tru-components-718574-klinken-steckverbinder-3-5-mm-buchse-einbau-vertikal-polzahl-3-stereo-schwarz-1-st-1564526.html)
    
Das Platinenlyout wurde mit KiCad in 2 Projekten erstellt - eines für das Display, eines für den Controller.

Funktionsweise technisch:

Füllstandsmessung:
  Die Sonde wird mit 24V betrieben und da bietet sich ein Netzteil handelsüblich an und eine Spannungswandlung (TSR-1 2433) von den 24V DC auf 3,3V für den Rest der Schaltung (Arduino MKR Zero, Relais, Display).
  Die Sonde liefert einen Strom in Abhängigkeit vom Druck (Füllstandshöhe). Der Wandler macht aus dem Strom eine Ausgangsspannung von 0 bis Vmax (3,3V).
  Diese Spannung wird am A0 (AnalogInput 0) des Arduino erfasst und in einen Füllstand umgerechnet - der Füllstand (Volumen) ist proportional zum gemessenen Wert an A0.
  Voraussetzung ist ein Behälter, dessen Füllstand (Volumen) sich linear proportional zum Druck verhält (Senkrechter Zylinder).
  
  Der Wandler für den Messwert an A0 muss vor Inbetriebnahme des Systems kalibiriert werden.
  ACHTUNG: Die Kalibrierung muss im ersten Schritt grob ohne angeschlossenen Arduino erfolgen um Beschädigungen durch Überspannung am Eingang des Arduino zu vermeiden.
  -> Siehe Kalibrierung und Inbetriebnahme.
  
  In Abhängigkeit von der gemessenen Füllhöhe und zwei einzustellenden Werten (LimitLow, LimitHigh) schaltet der Arduino das Relais sowie eine LED über PIN D14 an oder aus.
  Damit bei unterem Schwellwert / Minimum nicht sofort bei erneutem überschreiten (Nachlauf Regen) wieder zurück geschaltet wird, gibt es einen weiteren Wert (Oberer Schwellwert),
  der erreicht werden muss, damkt zurück auf Zisterne geschaltet wird.

Strom, Pumpe an oder aus:
  Über den Stromsensor SCT013 kann der Zustand der Pumpe (Regenwassernutzung) abgefragt werden, ohne in die Anlage eingreifen zu müssen.
  Dazu wird der Stromsensor unter der Beachtung der Stromflussrichtung um die Phase (nur die Phase) zur Pumpe gelegt.
  Am Ausgang des Sensors wird über Induktion eine Wechselspannung abgegeben. Dabei sind die Werte des Sensors zur korrekten Berechnung des Stroms und der Leistung in der Software einzustellen.
   Wichtig ist eine korrekte Dimensionierung des Sensors - es gibt unterschiedliche Typen. Für eine möglichst feine Auflösung ist der Messbereich (Ampere) des Sensors möglichst klein zu wählen, 
   aber groß genug, dass bei Vollast der Pumpe der Wert der Leistung ausreicht. Der hier benutzte Sensor misst bis 5A -> 1,1kw. Meine Pumpe hat 800 Watt Leistungsaufnahme.
   Gleichzeitig soll der Sensor möglichst große Spannungswerte am Ausgang produzieren, damit die Auflösung des Wertes möglichst fein/genau ist.
   Es gibt die Sensoren mit 0,33V und 1V Ausgangsspannung -> ich nutze hier 1V. Da die Spannung eine Wechselspannung ist, wird über den Spannungsteiler am Sensor die Wechselspannung um +1,65V angehoben.
   Der analoge Wert an A1 wird in einen Wert für den Stromverbrauch umgerechnt. Dazu ist es wichtig, den Messbereich des Sensors, die Ausgangsspannung des Sensors und den Referenzwert am Arduino (Zero 3,3V)
   zu kennen und in der Software einzustellen.

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

Ventilsteuerung generell:
Der Controller misst den Füllstand und (kann) in Abhängigkeit davon einen Schwimmerschalter für handelsübliche Zisternenpumpen ersetzen. Dazu ist die Steuerung der Pumpe auf "Automatik" zu stellen
und statt des Schwimmerschalters ein 230V - Kabel vom Anschluss des Schwimmerschalters zum Controller (3 - polige Klemme) zu führen.
Am Schraubklemmenblock 3 polig ist in jedem der mittlere Anschluss zu belegen und dann entweder NO (Normally open) oder NC (normally connected) - sonst schaltet am Ventil der Regenwasserpump nichts.

Funktionsweise Software:

Sämtliche Systemzustände werden bei Veränderung alle Sekunde an einen MQTT-Broker gesendet, spätestens alle 30 Sekunden auch unverändert. Das System führt folgende Werte:

- Booted     -> Boot-Zeitpunkt
- Timestamp  (Datum, Uhrzeit)
- Uptime     -> Zeit seit l. Boot/Neustart
- Mode       (0 = Zisterne, 1 = Hauswasser, 2 = Auto) -> Betriebsmodus
- Modus      -> Modus als Text
- Valve      (0 = Zisterne, 1 = Hauswasser) -> Ventilstellung
- Ventil     -> Ventilstellung als Text ("Zisterne", "Hauswasser")
- Reason     -> Grund für Ventilstellung (0 = "Manuell", 1 = "Zisterne voll", 2 = "Zisterne leer")
- Reasontext -> Reason als Text
- AnalogMin  -> Mindestwert, den die Sonde liefern muss, damit Sonde als vorhanden erkannt wird
- AnalogMax  -> Maximaler analoger Wert an A0 für volle Zisterne
- Analog     (0-1023) -> Analoger Messwert der Pegelsonde (0-1023)
- Liter      -> Füllung in Liter
- Prozent    -> Füllstand in %
- LimitLow   -> Unterer Schwellwert in Liter
- LimitHigh  -> Oberer Schwellwert in Liter
- LiterMin   -> Restfüllstand bei "leer" - unterer Grenzwert -> AnalogMin.
- LiterMax   -> maximales Füllvolumen wird benötigt zur Umrechnung des analogen Wertes in Liter (Analog = LiterMax / AnalogMax)
- StatusSonde-> Ist eine Sonde korrekt angeschlosen (Wenn der Analoge Wert unter einem Minimum liegt, dann ist davon auszugehen, dass die Sonde nicht korrekt angeschlossen ist
- HyaRain    -> Pumpe läuft (ON), oder nicht (OFF)
- Current    -> Stromverbrauch in Ampere
- Power      -> Watt (Volt * Leistung pro Sekunde
- KWh        -> KLilowattstunden

Es gilt:
LiterMin <= LimitLow <= LimitHigh <= LiterMax
Bei mir mit 6500l Zisterne und Restmenge von knapp 500l: 
LiterMin = 500, LimitLow = 600, LimitHigh = 1000, LiterMax = 6500. Bei unterschreiten von 600l schaltet das Relais auf Hauswassernachspeisung,
sobald wieder mehr als 1000l in der Zisterne sind, wird zurück auf Zisterne geschaltet.

Per MQTT kann der Controller aber auch gesteuert werden:
 - cmd/Mode [0,1,2,Hauswasser,Zisterne,Auto] -> Es kann der Modus umgestellt werden (nicht nur per Taster)
 - cmd/Limit [low|high]=<Number> -> Einstellen der Schwellwerte 
 - cmd/Calibrate [analogmin|analogmax|litermax]=<number> -> Einstellen der Kalibrierung in der Software nach dem die Sonde und der Wandler kalibriert wurden

Bei eingelegter SD-Card werden die Daten über einen Reset hinaus auf der SD-Card gespeichert und nach Neustart gelesen.
Wenn man darauf verzichtet die Werte nach dem Aufspielen der Software und der Einstellung in der Software zu verändern ist eine SD-Card nicht nötig

Aufbau, Inbetriebnahme
1. Platinenlayout, Produktion der Platinen
  Im Repository sind in den Unterordnern Controller und Displaypanel je ein KiCad-Projekt mit dem Schema und dem Platinenlayout - wer mag, kann damit andere Platinenformate erzeugen
  oder aber die Schaltung anpassen.
  Die zwei Zip-Archive im Hauptverzeichnis enthalten die Gerberdateien um PCB's produzieren zu lassen. [JLCPCB](https://jlcpcb.com) hat mir gute Dienste geleistet. Der Preis für jeweils 5 Stück (kleinste Einheit) Controler und 
  Display liegt bei 13.50$US - zzgl Versand, Zoll ist man dann bei knapp 30$. Lieferzeit etwa 1 Woche (je nach Zollabfertigung und lokalem Shipment)

2. Aufbau der Platine
  Die Platinen mit den Bauteilen bestücken. Dabei mit den flachen anfangen (Widerstände, Diode, Transistor), dann Buchsenleisten, Schraubklemmenblöcke, Relais, Spannungswandler

  Nach dem Zusammenbau, der Bestückung der Platine erfolgt schrittweise die Inbetriebnahme - noch ohne Arduino und ohne den Strom/Spannungswandler.
  Buchsenleisten sind nützlich für den Arduino (quasi ein muss) und für den Strom-/Spannungswandler. 
  Den Strom/Spannungswandler befreie ich von den Schraubklemmen (auslöten) und ersetze die durch Stiftleisten - einfacher Tausch.
  Dazu bekommt die Platine je Gegenstück eine Buchsenleiste 2 polig und 3 polig mit 5er Raster.

  Bei der Displayplatine darauf achten, dass der Wannenstecker auf die Rückseite kommt und die Polung korrekt ist - siehe Bilder

3. Prüfung der Spannungen
   An Pin 3 des Strom/Spannungswandlers müssen 24V anliegen (auf dem Board rechte Buchsenleiste oberer Pin/Buchse)
   Am Schraubklemmenblock für die Sonde müssen 24V anliegen
   Am Pin 3 der Stiftleiste für den Arduino müssen gegenüber GND (Pin 4) 3,3Volt anliegen

3. Einstellen des Strom/Spannungswandlers (idealerweise bei voller Zisterne)
   Erster Durchgang des Einstellens OHNE Arduino
      Zum Einstellen des Stromspannungswandlers wird selbiger in die Buchsenleisten gesteckt. Ebenso wird an der Stiftleiste "Probe" ein Multimeter zur Spannungsmessung angeschlossen.
      Ohne Sonde liefert das Multimeter dann 0V. Dann wird die Sonde angeschlossen und noch nicht in die Zisterne herabgelasse. Es erfolgt die Nullpunkteinstellung für "leer".
      Mit dem linken Poti dreht man jetzt solange nach links (kleiner) oder nach rechts (größer), bis die gemessene Spannung verlässlich nah an 0V liegt aber eben noch größer als 0V ist.
      Jetzt kann die Sonde in die Zisterne gelassen werden - so tief wie möglich. Zur korrekten Kalibrierung des Maximalwertes ist es am einfachsten den maximalen Füllstand auch zur Einstellung
      der maximalen Spannung zu nutzen. Dazu dreht man jetzt am rechten Poti des Wandlers (links kleiner, rechts größer) bis die gemessene Spannung nah bei 3,3V ist. Es empfiehlt sich etwas "Luft" zu lassen.
   Zweiter Durchgang mit Arduino - die Werte sind verändert, die Vorgehensweise ist die gleiche
      Wichtig ist jetzt, dass jetzt die Analogen Messwerte angeschaut werden.
      Messwert 1 "Sonde angeschlossen, aber nicht unter Druck" -> Dies wird mit dem Poti links auf einen möglichst kleinen aber stabil kleinen Wert über Null eingestellt (bei mir 50).
      Dieser Wert wird in der Software im #DEFINE ANALOG_MIN abgelegt.
      Messwert 2 "Sonde angeschlossen und Maximaler Füllstand der Zisterne und Sonde am tiefsten Punkt (maximaler Druck)
      Den Messwert stellen wir mit dem rechten Poti so ein, dass er möglichst groß ist (nah an 1023), aber noch etwas Luft ist, falls die Zisterne mal "übervoll" ist - Mehr Zulauf als Ablauf (kommt schonmal vor bei mir)
      Den Wert dann in #DEFINE ANALOG_MAX ablegen.
      Die Defines für LITER_MIN mit einem Wert für die Restmenge in der Zisterne bevor die Pumpe auf Störung laufen würde (bei mir sind dann noch 15cm in der Zisterne drin und damit rund 470 Liter.
      LITER_MAX einstellen mit dem maximalen regulären Füllvolumen der Zisterne.

   Wenn die Zisterne nicht voll ist, sollte man abschätzen können, wie voll (in Prozent) sie ist, man kann dann ausgehend vom Zielwert bei "Zisterne Voll" nah bei 3,3V und
   dem unteren Messwert für "leer" den Spannungswert, den man jetzt messen möchte im Dreisatz ableiten und dann einstellen am Poti.
4. Aufspielen der Software auf den Arduino
5. Board stromlos machen / Spannungsversorgung trennen
6. Aufstecken des Arduino und des Ethernet-Shields auf die Buchsenleisten 14 polig
7. Verbinden des Displays mit dem Controller (Wenn das Display NICHT angeschlossen ist, dann wird der Arduino NICHTS tun.)
8. Netzteil anschließen
9. Der Arduino bootet und auf dem Display werden die Daten angezeigt
10. Die Daten werden zyklisch an den MQTT-Broker verteilt (Bei Änderungen sekündlich, ansonsten alle 30 Sekunden)
11. Prüfung der Messwerte am Broker
12. Es empfiehlt sich, dass nach ein paar Wochen Betrieb und Leer, Voll-Zyklen nochmal die Messwerte der Kalibrierung geprüft werden und notfalls nochmal in der Software neu eingestellt werden - entweder per MQTT oder "hart" per Software



