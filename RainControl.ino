#include <Arduino.h>
#include <avr/dtostrf.h>
#include <PubSubClient.h>
#include <Time.h>

/* ---------------------------- allgemeine Settings ----------------------------- */
#define VERSION "RainControl V0.9a"

// For Debugging with no Ethernet and no Display connecte, comment out for real-world
#define DEBUG 1

// ##############################################################################################################################
// ---- HIER die Initialen Einstellungen vornehmen, die Werte sind über MQTT anpassbar
// ##############################################################################################################################
// Hier die maximale Füllmenge des Behälters angeben. Dies gilt nur für symmetrische Behälter.
// Deckelung bei 100%
// #define MAX_LITER = 7280;
// keine Deckelung bei "Übervoll", sondern Max-Wert ist maximal nutzbarer (Überlauf)
#define MAX_LITER 6500

// Analoger Wert bei maximalem Füllstand (wird alle 30 Sekungen auf dem LCD angezeigt oder in der seriellen Konsole mit 9600 Baud.
// Maxwerte mit 100% - Deckelung
// const int analog_max = 972;
// const int analog_min = 50;
// Maxwerte ohne Deckelung bei 100%
#define ANALOG_MIN 50
#define ANALOG_MAX 774
#define LIMIT_LOW 500
#define LIMIT_HIGH 1000

// Dichte der Flüssigkeit - Bei Heizöl bitte "1.086" eintragen, aber nur wenn die Kalibrierung mit Wasser erfolgt ist!
// Bei Kalibrierung mit Wasser bitte "1.0" eintragen
const float dichte = 1.0;

#define MODE_PIN 13
#define MODE_ZISTERNE 0
#define MODE_HAUSWASSER 1
#define MODE_AUTO 2
#define MODE_COUNT 10
bool setmode = false;
char *modes[] = {"Zisterne", "Hauswasser", "Auto"};
int mode_count = MODE_COUNT;
int mode = MODE_AUTO;
char *modus = modes[mode];

int old_mode = MODE_AUTO;
int pin_stat;
int old_stat;

#define VALVE_PIN 14
#define VALVE_ZISTERNE LOW
#define VALVE_HAUSWASSER HIGH
int new_valve = VALVE_ZISTERNE;
char *valves[] = {"Zisterne", "Hauswasser"};
char *reasons[] = {"Manuell", "Zisterne voll", "Zisterne leer"};

// Status HyaRain (Currently unused)
#define ERROR_PIN 1
char *hyaraintext[] = {"Off", "On", "Failure", "Invalid"};

// No more delay
unsigned long pubMillis;        // MQTT - Refresh
unsigned long updateMillis;     
#define SEKUNDE 1000  // one second
#define HSEKUNDE 500  // half second

unsigned long pinMillis;
unsigned long lastMillis = 0;
boolean publish = false;

char uptime[20];
char *up = &uptime[0];

char buf[255];
/* ---------------------------- Topics -----------------------------------
   Basisklassen für ein MQTT - Topic. Die Klasse speichert ein Topic bestehend aus
   dem Topic-Namen, einem Wert des Topics, (INteger, float, double, bool oder char*
   sowie den Änderungszustand des Topics.
   Basisklasse TopicBase
   Die Klasse Topic überlagert TopicBase und implementiert die Methoden zur Kommunikation mit
   dem Broker oder zum setzen, lesen von Werten.
   Die Klasse Topic überlagert auch den Operator "=" - Das Topic lässt sich benutzen wie eine Variable.
   ----------------------------------------------------------------------- */
// Ersatz für std::is_same (Arduino-kompatibel)
template<typename A, typename B>
struct is_same_type { static const bool value = false; };

template<typename A>
struct is_same_type<A, A> { static const bool value = true; };

class TopicBase {
    protected:
        bool changed;
    public:
        virtual void Publish(PubSubClient *_mqttclient) = 0;
        virtual char *Name() = 0;
        virtual char *toString() = 0;
        virtual ~TopicBase() {};
        void Changed() {changed = true;};
};

template <typename Type>
class Topic : public TopicBase {
    private:
        Type data;
        char topic[128];
        char tostring[255];

    public:
        Topic(const char *_name, Type value) : data(value) {
            strcpy(&topic[0], _name);
        }

        char *Name() {
            return (char *)&topic[0];
        }

        char *toString() {
            tostring[0] = 0;

            if (is_same_type<Type, int>::value) {
                snprintf(tostring, sizeof(tostring), "%d", data);
            } else if (is_same_type<Type, float>::value || is_same_type<Type, double>::value) {
                snprintf(tostring, sizeof(tostring), "%.3f", data);
            } else if (is_same_type<Type, const char*>::value || is_same_type<Type, char*>::value) {
                snprintf(tostring, sizeof(tostring), "%s", data);
            } else if (is_same_type<Type, const bool>::value || is_same_type<Type, bool>::value) {
                snprintf(tostring, sizeof(tostring), "%s", data ? "true" : "false");
            } else if (is_same_type<Type, const time_t>::value || is_same_type<Type, time_t>::value) {
                struct tm *timeinfo;
                time ((time_t *)&data);
                timeinfo = localtime((time_t *)&data);
                
                strftime(tostring, sizeof(tostring), "%d.%m.%Y %H:%M:%S", timeinfo);
                // snprintf(tostring, sizeof(tostring), "%s", "2.1.2025");
            } else {
                snprintf(tostring, sizeof(tostring), "[unsupported type]");
            }
            return tostring;
        }
        // Zuweisungsoperator überladen
        Topic<Type>& operator=(const Type& value) {
            setData(value);
            return *this;
        }

        // Typ-Konvertierungsoperator (Lesen)
        operator Type() const {
            return getData();
        }

        Type getData() const {
            return data;
        };

        void setData(Type value) {
            if (value != data) {
                data = value;
                changed = true;
            }
        };

        void Publish(PubSubClient *_mqttclient) {
            if (changed) {
                if (_mqttclient->connected()) {
                    changed = false;
                    _mqttclient->publish(topic, toString());
                }
#ifdef DEBUG
                changed = false;
#endif
                Serial.print(topic);
                Serial.print(" ");
                Serial.println(toString());
            }
        };
};

/* Definition der Topics zur Speicherung der Daten sowhol hier im Code, als auch in Richtung Broker */
Topic<time_t>   Timestamp("Timestamp", 0);
Topic<int>      Mode("Mode", 0);
Topic<int>      Valve("Valve", VALVE_ZISTERNE);
Topic<int>      Analog("Analog", 0);
Topic<int>      Liter("Liter", 0);
Topic<int>      LimitLow("LimitLow", 0);
Topic<int>      LimitHigh("LimitHigh", 0);
Topic<int>      LiterMax("LiterMax",0);
Topic<float>    Prozent("Prozent", 0.0);
Topic<char *>   Uptime("Uptime", "");
Topic<bool>     StatusSonde("StatusSonde", true);
Topic<char *>   Ventil("Ventil", valves[VALVE_ZISTERNE]);
Topic<char *>   Modus("Modus", "");
Topic<char *>   HyaRain("HyaRain", "");
Topic<time_t>   Booted("Booted", 0);
Topic<int>      AnalogMin("AnalogMin", 0);
Topic<int>      AnalogMax("AnalogMax", 0);
Topic<double>   CurrentFactor("CurrentFactor", 1.0);
Topic<double>   Current("Current", 0.0);
Topic<double>   Power("Power", 0.0);
Topic<double>   KWh("KWh", 0.0);
Topic<int>      Reason("Reason", 0);
Topic<char *>   ReasonText("ReasonText", reasons[0]);

TopicBase* topics[] = { &Timestamp, &Mode, &Valve, &Analog, &Liter, &LimitLow, &LimitHigh, &LiterMax, &Prozent, &Valve, 
                        &Uptime, &StatusSonde, &Ventil, &Modus, &HyaRain, &Booted, &AnalogMin, &AnalogMax, &CurrentFactor,
                        &Current, &Power, &KWh, &Reason, &ReasonText};


/* ------------------------------ Analog Input ---------------------------
   Basisklasse zur Behandlung von analogen Inputs
   Instantiiert für einen Analogen Eingang, optional wird ein Offset/Average
   für den Messwert mitgegeben - nützlich für Messwerte, die um einen Mittelpunkt schwanken
   ----------------------------------------------------------------------- */
class AI {
    protected:
        int pin;
        int count;
        long aggregate;
        long average;
    public:
        AI(int _pin, int _average = 0) {
            pin = _pin; count = 0; aggregate = 0; average = _average;
        };
        // Anzahl Messungen
        int Count() {
            return count;
        };
        // Messwert lesen am Eingang, Aufsummieren, Anzahl Messungen erhöhen und Average mitlaufen lassen (aufaddieren), der Messwert wird als absolute Differenz zum Average/Mittelwert betrachtet
        void Read(int _average = 0) {
            int val = analogRead(pin);
            aggregate += abs(val - _average);
            average += val;
            count++;
        };
        // Wert zurückgeben, dabei die Summe der Messwerte durch die Anzahl teilen
        int Value() {
            if (count > 0 && aggregate > 0) {
                return max(aggregate / count - 1, 0);
            } else {
                return 0;
            }
        };
        // Average zurückgeben (das ist der Mittelwert der Messungen - bei )
        int Average() {
            if (count > 0) {
                return average / count;
            } else {
                return 0;
            }
        };
        // Messreihe neu starten, alle Werte zurücksetzen
        void Restart() {
            aggregate = 0;
            if (count > 0) {
                average = average / count;
            } else {
                average = 0;
            }
            count = 0;
        };
};

/* ------------------------------ Analog Input ---------------------------
   Energy - Klasse für die Messwertermittlung von Werten des Stromsensors SCT013
   die Werte am Eingang A1 sind über den Spannungsteiler an A1 um 1,65V angehoben
   und oszillieren um 512 herum - Mittelpunkt, Average wird mit 512 begonnen
   ----------------------------------------------------------------------- */
class Energy : public AI {
    private:
        double ampmax;  // Maximale Ampere bei maximaler Amplitude -> 1, 5, 10, 30, 100 Amp
        double vmax;    // Maximale Spannung Analogamplitude Signal (muss kleiner Referenzspannung sein) 
        
        double volt;    // Netzspannung
        double vrefmax; // Referenzspannung Arduino (3,3 oder 5V)
        double vrefmin; // Unterer Spannungswert (i.d.R. 0V)

        int imin;       // Integer - Min am Analogen Input -> i.d.R. 0
        int imax;       // Integer - Max am analogen Input -> i.d.R. 1024

        double scale;   // Umrechnungsfaktor
        
        double amp;
        double watt;
        double wattseconds;
        unsigned long wattmillis;

    public:
        Energy(int _pin, double _ampmax, double _vmax, double _volt = 230, double _vrefmax = 3.3, double _vrefmin = 0, int _imax = 1024, int _imin = 0) : AI(_pin, 512) {
            ampmax = _ampmax;
            vmax = _vmax;
            volt = _volt;
            vrefmax = _vrefmax;
            vrefmin = _vrefmin;
            imax = _imax;
            imin = _imin;
            wattseconds = 0;
            wattmillis = millis();
            watt = 0;
            // Scale ist der Faktor mit dem der analoge Messwert multipliziert werden muss. Er ergibt sich aus:
            // maximalem Messbereich des Sensors in Ampere , maximaler Referenzspannung (Arduino Zero = 3.3)
            // minimaler Referenzspannung (0.0), dem Maximalen Wert des Analogen Inputs (1024) dem minimalen Wert (0)
            // und der maximalen Spannung des Sensors SCT013 (1V)
            scale = ampmax * (vrefmax - vrefmin) / (imax - imin) / vmax;
        };

        double Scale() {
            return scale;
        };

        double Amp() {
            if (count > 0 && aggregate > 2 * count) {
                amp = max(((double) aggregate / (double)count - 1.0), 0) * scale;
            } else {
                amp = 0;
            }
            return amp;
        };
        double Volt() {
            return volt;
        };
        double Power() {
            return Volt() * Amp();
        };

        void ResetKwh() {
            wattseconds = 0;
            wattmillis = millis();
            watt = 0;
        };

        double Watt() {
            return watt;
        };

        double Seconds() {
            return wattseconds;
        };

        double KWh() {
            return watt / 3600 / 1000;
        };

        void Restart() {
            if (wattseconds > 0) {
                watt += (Volt() * Amp() / wattseconds);
            }
            wattseconds += ((double) millis() - (double) wattmillis) / 1000;

            aggregate = 0;
            if (count > 0) {
                average = average / count;
            } else {
                average = 0;
            }
            count = 0;
            wattmillis = millis();
        };
};

AI Sonde(A0);                               // Drucksonde Zisterne an A0
Energy sct013(A1, 5.0, 1.0, 230, 3.3);      // Energiesensor SCT013 - 5A, 1V, 230V Netzspannung, 3.3V VCC Arduino (MKR Zero) an A1
int average = 512;                           // Mittelpunkts-Wert selbstkalibrierend für Stromsensor SCT013

/* ----------------------------- Ethernet ---------------------------- */
#include <Ethernet.h>
#include <EthernetUdp.h>
EthernetClient ethClient;
EthernetUDP Udp;

// MAC-Addresse bitte anpassen! Sollte auf dem Netzwerkmodul stehen. Ansonsten eine generieren.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x0C };

// IP Adresse, falls kein DHCP vorhanden ist. Diese Adresse wird nur verwendet, wenn der DHCP-Server nicht erreichbar ist.
IPAddress ip(192, 168,   3,  95);
IPAddress ns(192, 168,   3, 254);
IPAddress gw(192, 168,   3, 254);
IPAddress sn(255, 255, 255,   0);

/* ---------------------------- SD Card ---------------------------- */
#include <SD.h>
const int chipSelect = SDCARD_SS_PIN;
boolean SDStatus = true;
File SDSettings;
#define SETTINGSFILE "RC.ini"
File SDLogger;
#define LOGFILE "RC.csv"

struct SDData {
    char *id;
    int analogMin;
    int analogMax;
    int literMax;
    int limitLow;
    int limitHigh;
    double currentFactor;
} sddata = {"RC", ANALOG_MIN, ANALOG_MAX, MAX_LITER, LIMIT_LOW, LIMIT_HIGH, 15.0};
char *sdini_format = "%2s;%d;%d;%d;%d;%d;%s;";

/* ----------------------------- MQTT ---------------------------- */
// MQTT definitions
#define MQTT_ID "RainControl"
void MqttCallback(char *topic, byte *payload, unsigned int length);

// IP Adresse und Port des MQTT Servers
IPAddress mqttserver(192, 168, 3, 33);
const int mqttport = 1883;

// Wenn der MQTT Server eine Authentifizierung verlangt, bitte folgende Zeile aktivieren und Benutzer / Passwort eintragen
const char *mqttuser = "mqttiobroker";
const char *mqttpass = "Waldweg3";
PubSubClient mqttclient(ethClient);

#define SEND_INTERVAL 30             // the sending interval of indications to the server, by default 30 seconds

struct Callback {
   String callback;
   void (*_function)(char*);
   unsigned long pointer;
};

void MqttSetLimit(char *payload);
void MqttSetMode(char *payload);
void MqttSetCalibrate(char *payload);


typedef Callback t_Callback;
t_Callback callbacks[] = {
    {String(MQTT_ID) + String("/cmd/Limit"), MqttSetLimit},
    {String(MQTT_ID) + String("/cmd/Mode"), MqttSetMode},
    {String(MQTT_ID) + String("/cmd/Calibrate"), MqttSetCalibrate}
};

String mqttid = MQTT_ID;

void MqttConnect() {
    if (!mqttclient.connected()) {
        Serial.print("Connecting to MQTT-Server as ");
        Serial.print(mqttid.c_str());
        Serial.print(" ... ");
        int ret = mqttclient.connect(mqttid.c_str(), mqttuser, mqttpass);
        if (ret) {
            Serial.println("Connected to Mqtt-Server");
            for (int i = 0; i < (sizeof(callbacks) / sizeof(callbacks[0])); i++) {
                mqttclient.subscribe(callbacks[i].callback.c_str());
                Serial.print("subscribing to ");
                Serial.println(callbacks[i].callback.c_str());
            }
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttclient.state());
            Serial.println(" try again");
        }
    } else {
        Serial.println("already connected!");
    }
}

void MqttPublish(void) {
    char topic[30];
    char payload[255];
    
    if (!mqttclient.connected()) {
        MqttConnect();
    }
    Serial.println("MQTT - publishing...");
    for (int i = 0; i < (sizeof(topics) / sizeof(topics[0])); i++) {
        topics[i]->Publish(&mqttclient);
    }
}


void toLower(char *string) {
    for (uint8_t cnt = 0; cnt < strlen(string); cnt++) {
      *(string + cnt) = tolower(*(string + cnt));
    }
}

void  MqttSetLimit(char *payload) {
    char *key;
    char *value;
    char *eq;
    
    toLower(payload);
    
    Serial.print("SetLimit: ");
    Serial.println(payload);
    key = payload;
    eq = strchr(payload, '=');
    if (eq != NULL) {
        *(eq) = 0;
        value = ++eq;
        Serial.print("Key: ");
        Serial.print(key);
        Serial.print(" , Value: ");
        Serial.println(value);
        if (strcmp(key, "low") == 0) {
            LimitLow = sddata.limitLow = max(atoi(value), 0);
            SDWriteSettings();
            publish = true;
        } else if (strcmp(key, "high") == 0) {
            LimitHigh = sddata.limitHigh = min(atoi(value), sddata.literMax);
            SDWriteSettings();
            publish = true;
        }
    }
}

void  MqttSetMode(char *payload) {
    char *key;
    char *value;
    char *eq;

    toLower(payload);
    Serial.print("Mode: ");
    Serial.println(payload);
    
    key = payload;
    eq = strchr(payload, '=');

    if (strcmp(payload, "0") == 0 || strcmp(payload, "zisterne") == 0) {
        Mode = MODE_ZISTERNE;
    } else if (strcmp(payload, "1") == 0 || strcmp(payload, "hauswasser") == 0) {
        Mode = MODE_HAUSWASSER;
    } else if (strcmp(payload, "2") == 0 || strcmp(payload, "auto") == 0) {
        Mode = MODE_AUTO;
    }
    Modus = modes[Mode];
    Serial.print("Cmd - Mode: ");
    Serial.println(Modus.toString());
}

void  MqttSetCalibrate(char *payload) {
    char *key;
    char *value;
    char *eq;

    toLower(payload);
    Serial.print("Calibrate: ");
    Serial.println(payload);
    
    key = payload;
    eq = strchr(payload, '=');

    if (eq != NULL) {
        *(eq) = 0;
        value = ++eq;
        Serial.print("Key: ");
        Serial.print(key);
        Serial.print(" , Value: ");
        Serial.println(value);
        if (strcmp(key, "analogmin") == 0) {
            AnalogMin = sddata.analogMin = max(atoi(value), 0);
            SDWriteSettings();
            publish = true;
        } else if (strcmp(key, "analogmax") == 0) {
            sddata.analogMax = min(atoi(value), 1023);
            SDWriteSettings();
            publish = true;
        } else if (strcmp(key, "litermax") == 0) {
            sddata.literMax = atoi(value);
            SDWriteSettings();
            publish = true;
        } else if (strcmp(key, "factor") == 0) {
            sddata.currentFactor = atof(value);
            SDWriteSettings();
            publish = true;
        }
    }
}

void MqttCallback(char *topic, byte *payload, unsigned int length) {
    char *payloadvalue;
    char *payloadkey;
    String s_payload;
    String s_key;
    String s_value;

    payload[length] = 0;
    for (int i = 0; i < (sizeof(callbacks) / sizeof(callbacks[0])); i++) {
        if (strcmp(callbacks[i].callback.c_str(), topic) == 0) {
            Serial.print("MqttCallback: ");
            Serial.println(callbacks[i].callback.c_str());
            callbacks[i]._function((char *)payload);
        }
    }
}

/* ------------------------------- NTP ------------------------------- */
#include <NTP.h>
#define NTPSERVER "pool.ntp.org"
bool ntp_time = false;

#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR (60 * SECONDS_PER_MINUTE)
#define SECONDS_PER_DAY (24 * SECONDS_PER_HOUR)

NTP ntp(Udp);

/* ------------------------------- Display ---------------------------- */
// Display 
#include <U8g2lib.h>
U8G2_SSD1309_128X64_NONAME2_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 2, /* dc=*/ 3, /* reset=*/ 4);

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

/*-------------------------------- Functions ---------------------------- */
bool SDLogData() {
    char logbuf[256];

    if (SDStatus) {
        Serial.print("Logging to SD Card...");
        if (SDLogger = SD.open(LOGFILE, FILE_WRITE)) {
            if (SDLogger.size() == 0) {
                for (int i = 0; i < (sizeof(topics) / sizeof(topics[0])); i++) {
                    // SDLogger.write(topics[i].topic);
                    SDLogger.write(topics[i]->Name());
                    SDLogger.write(';');
                }
                SDLogger.write('\n');
            }

            for (int i = 0; i < (sizeof(topics) / sizeof(topics[0])); i++) {
                SDLogger.write(topics[i]->toString());
                /*
                if (topics[i].type == 'I') {
                    sprintf(logbuf, "%d", *(int *)topics[i].pointer);
                } else if (topics[i].type == 'F') {
                    sprintf(logbuf, "%5.2f", *(float *)topics[i].pointer);
                } else if (topics[i].type == 'C') {
                    sprintf(logbuf, "%s", *(char **)topics[i].pointer);
                } else if (topics[i].type == 'B') {
                    sprintf(logbuf, "%s", *(boolean *)topics[i].pointer ? "true" : "false");
                }
                SDLogger.write(logbuf);
                */
                SDLogger.write(';');
            }
            SDLogger.write('\n');
            SDLogger.close();
            Serial.println("ok");
        } else {
            Serial.println("failed");
        }
    }
}

bool SDReadSettings() {
    char line[1024];
    char sdbuf[256];
    int i = 0;
    char id[4];
    int aMin, aMax, lMax, lLow, lHigh;
    double cFactor;
    char ccFactor[64];

    if (SDStatus) {
        Serial.print("read Settings from SD Card...");
        if (SDSettings = SD.open(SETTINGSFILE, FILE_READ)) {
            while (SDSettings.available() && i < 256) {
                sdbuf[i++] = SDSettings.read();
            }
            SDSettings.close();
            sdbuf[i] = 0;

            sscanf(sdbuf, sdini_format, &id[0], &aMin, &aMax, &lMax, &lLow, &lHigh, &ccFactor[0]);
            Serial.println("ok");
            if (id[0] == 'R' && id[1] == 'C') {
                sprintf(line, "AnalogMin: %d, AnalogMax: %d, LiterMax: %d, LimitLow: %d, LimitHigh: %d, cFactor: %s", aMin, aMax, lMax, lLow, lHigh, ccFactor[0]);
                Serial.println(line);
                AnalogMin       = sddata.analogMin = aMin;
                AnalogMax       = sddata.analogMax = aMax;
                LiterMax        = sddata.literMax = lMax;
                LimitLow        = sddata.limitLow = lLow;
                LimitHigh       = sddata.limitHigh = lHigh;
                CurrentFactor   = sddata.currentFactor = atof((char *)&ccFactor[0]);
            } else {
                Serial.println("Wrong format of settings!");
            }
        } else {
            Serial.println("failed");
        }
    }
}

bool SDWriteSettings() {
    char sdbuf[256];
    char ccFactor[64];

    if (SDStatus) {
        dtostrf(sddata.currentFactor,8,3, ccFactor);
        sprintf((char *)&sdbuf[0], sdini_format, sddata.id, sddata.analogMin, sddata.analogMax, sddata.literMax, sddata.limitLow, sddata.limitHigh, ccFactor);
        Serial.println((char *)&sdbuf[0]);
        SD.remove(SETTINGSFILE);
        Serial.print("writing Settings to SD Card...");
        if (SDSettings = SD.open(SETTINGSFILE, FILE_WRITE)) {
            SDSettings.seek(0);
            SDSettings.println((char *)&sdbuf[0]);
            SDSettings.close();
            Serial.println("ok.");
        } else {
            Serial.println("failed!");
            SDStatus = false;
        }
    }
}

bool SDsetup() {
    Serial.println("Initializing SD Card...");
    
    if (!SD.begin(chipSelect)) {
        Serial.println("Initialization failed. Things to check:");
        // DisplayPrint("Initialization failed. Things to check:");
        Serial.println(" * is a card inserted?");
        // DisplayPrint(" * is a card inserted?");
        Serial.println(" * is your wiring correct?");
        // DisplayPrint(" * is your wiring correct?");
        Serial.println(" * did you change the chipSelect pin to matcjh your shield or module?");
        SDStatus = false;
    } else {
        Serial.println("Wiring is correct and a card is present.");
        // DisplayPrint("Wiring is correct and a card is present.");
        SDReadSettings();
        // print the type of card
        SDStatus = SDWriteSettings();
    }
}


void CalcUptime() {
    int days, secs, hours, mins;

    secs = Timestamp - Booted;

    days = secs / (SECONDS_PER_DAY);
    secs = secs - (days * SECONDS_PER_DAY);
    hours = secs / (SECONDS_PER_HOUR);
    secs = secs - (hours * SECONDS_PER_HOUR);
    mins = secs / SECONDS_PER_MINUTE;
    secs = secs - mins * SECONDS_PER_MINUTE;

    sprintf(uptime, "%4dd %2dh %2dm", days, hours, mins);
    uptime[14]= 0;
    Uptime = uptime;
    // Serial.print("Uptime: ");
    // Serial.print(uptime);
    if (setmode) {
        Serial.print(setmode ? " Settings-Mode" : "");
        sprintf(buf, " (%d)secs ", mode_count);
        Serial.println(buf);
    }
}

void CheckEthernet() {
    Serial.println("Check Ethernet - Connection...");
    if (Ethernet.hardwareStatus() != EthernetNoHardware && Ethernet.linkStatus() == LinkON) {
        /*--------------------------------------------------------------
           check Ethernet services
          --------------------------------------------------------------*/
        switch (Ethernet.maintain()) {
            case 1:
                //renewed fail
                Serial.println(F("Error: renewed fail"));
                break;
    
            case 2:
                //renewed success
                Serial.println(F("Renewed success"));
                //print your local IP address:
                Serial.print(F("My IP address: "));
                Serial.println(Ethernet.localIP());
                break;
        
            case 3:
                //rebind fail
                Serial.println(F("Error: rebind fail"));
                break; 
    
            case 4:
                //rebind success
                Serial.println(F("Rebind success"));
                //print your local IP address:
                Serial.print(F("My IP address: "));
                Serial.println(Ethernet.localIP());
                break;
        
            default:
                //nothing happened
                break;
        }    
    }
}

void setup() {
    int ret = 0;
  
    Timestamp = 0;
    Booted = 0;
    // rtc.setEpoch(0);
    ntp_time = false;

    /*--------------------------------------------------------------
       Milliseconds start
      --------------------------------------------------------------*/
    updateMillis = pubMillis = millis();  //initial start time
    
    Serial.begin(115200);
    /*-------------------------------------------------------------------
     * Setup Pins for Valve and mode-setting
     */
    pinMode(ERROR_PIN, INPUT_PULLUP);
    pinMode(VALVE_PIN, OUTPUT);
    pinMode(MODE_PIN, INPUT_PULLUP);

    pin_stat = old_stat = digitalRead(MODE_PIN);
    Serial.print("ModePin: ");
    Serial.println(pin_stat == LOW ? "Low" : "High");

    delay(3000);

    SDsetup();
    
    Serial.println("Initializing TCP");
  
    u8g2.begin();

    ret = Ethernet.begin(mac);

    if (Ethernet.hardwareStatus() != EthernetNoHardware && Ethernet.linkStatus() == LinkON) {
        if (ret == 0) {
            Ethernet.begin(mac, ip, ns, gw, sn);
        }
        Udp.begin(8888);
        Serial.println(ret == 0 ? "Static" : "Dhcp");
        Serial.print("localIP: ");
        Serial.println(Ethernet.localIP());
        Serial.print("subnetMask: ");
        Serial.println(Ethernet.subnetMask());
        Serial.print("gatewayIP: ");
        Serial.println(Ethernet.gatewayIP());
        Serial.print("dnsServerIP: ");
        Serial.println(Ethernet.dnsServerIP());
        mqttclient.setServer(mqttserver, 1883);
        mqttclient.setCallback(MqttCallback);
        MqttConnect();
        // Do NOT use localized Time form NTP - localization is done manually on display/printuing only
        Serial.println("Setup NTP");
        ntp.ruleDST("CEST", Last, Sun, Mar, 2, 120); // last sunday in march 2:00, timetone +120min (+1 GMT + 1h summertime offset)
        ntp.ruleSTD("CET", Last, Sun, Oct, 3, 60); // last sunday in october 3:00, timezone +60min (+1 GMT)
        ntp.updateInterval(1000);
        delay(2000);
        ntp.begin();
    } else {
        Serial.println("No Ethernet - either no Hardware attached or no Link!");
    }

    Display();

    Serial.println("Setup Done!");
}


void Display() {
#ifdef DEBUG
    return;
#endif
    u8g2.clearBuffer();					// clear the internal memory
    u8g2.setFont(u8g2_font_t0_11_tf);	// choose a suitable font

    sprintf(buf, "%s", VERSION);
    u8g2.drawStr(0,10, buf);	// write something to the internal memory
    sprintf(buf, "Level : %4d l, %3d%%", Liter, Prozent);
    Serial.println(buf);
    u8g2.drawStr(0,20, buf);	// write something to the internal memory
    if (!setmode) {
        sprintf(buf, "Modus : %s", Modus);
    } else {
        sprintf(buf, "Modus : %s(%d)", Modus, mode_count);
    }
    u8g2.drawStr(0,30, buf);
    sprintf(buf, "Status: %s", ReasonText);
    u8g2.drawStr(0,40, buf);
    sprintf(buf, "Ventil: %s", Ventil);
    u8g2.drawStr(0,50, buf);
    u8g2.drawStr(0,60, Timestamp.toString());

    u8g2.sendBuffer();					// transfer internal memory to the display
}


void loop() {
    // put your main code here, to run repeatedly:

    sct013.Read(average);
    Sonde.Read();

    pin_stat = digitalRead(MODE_PIN);
    pinMillis = millis();
    publish = false;
    
    if (pin_stat == LOW) {
        if (pinMillis - lastMillis > 200) {
            Serial.println("Impuls Low");
            lastMillis = pinMillis;
            mode_count = MODE_COUNT;
            if (!setmode) {
                setmode = true;
                old_mode = mode;
                sprintf(buf, "Button: Enter Settings-Mode %d (%s)", mode, modes[mode]);
                Serial.println(buf);
            } else {
                Mode = Mode - 1;
                if (Mode < 0) {
                    Mode = 2;
                }
                Modus = modus = modes[Mode];
                sprintf(buf, "Button: Mode change to %d (%s)", Mode, Modus);
                Serial.println(buf);
            }
        }
    }


    if ((millis() - pubMillis) > SEND_INTERVAL * 1000) { // Alle 30 Sekunden
        for (int i = 0; i < (sizeof(topics) / sizeof(topics[0])); i++) {
            topics[i]->Changed();
        }
        SDLogData();
        MqttPublish();
        pubMillis = millis();
    }
    if (millis() - updateMillis >= SEKUNDE) { // Hier eine Sekunde warten
        if (mqttclient.connected()) {
            mqttclient.loop();
        }
        StatusSonde = Analog >= sddata.analogMin;             // Sondenfehler ?
        Serial.println("Analogen Wert lesen Sonde");
        /* Analoger Wert der Sonde geglättet */
        Analog = Sonde.Value();
        Serial.println("Restart Sonde");
        Sonde.Restart();

        Prozent = Analog * 100 / AnalogMax;
        Liter = Analog * LiterMax / AnalogMax;      // calculate liter

        /* Analoger Wert des Stromsensors geglättet */
        Serial.println("Analogen Wert lesen Stromsensor");
        Current = sct013.Amp();
        if (Current > 0) {
            HyaRain = "On";
        } else {
            HyaRain = "Off";
        }
        Power = sct013.Watt();
        KWh = sct013.KWh();

        average = sct013.Average();
        sct013.Restart();

        if (Ethernet.hardwareStatus() != EthernetNoHardware && Ethernet.linkStatus() == LinkON) {
            ntp.update();

            // Serial.println("Get NTP-Time Epoch");
            Timestamp = ntp.epoch();
            // sprintf(timestamp, "%d.%d.%d %02d:%02d:%02d", (int)rtc.getDay(), (int)rtc.getMonth(), (int)rtc.getYear(), (int)rtc.getHours(), (int)rtc.getMinutes(), (int)rtc.getSeconds());
            // sprintf(timestamp, "%s", PrintTimeStamp(localTimefromUTC(timestamp_t)));
            // sprintf(timestamp, "%s", ntp.formattedTime("%d.%m.%Y %H:%M:%S"));
        }
        
        // ReadAnalog();
        CheckEthernet();
        CalcUptime();

        if (Booted == 0) {
            Booted = Timestamp.getData();
        }

        if (setmode) {
            mode_count--;
            if (mode_count <= 0) {
                //sprintf(buf, "leaving Settings-Mode - change from %d to %d (%s)", old_mode, mode, modes[mode]);
                //USE_SERIAL.println(buf);
                if (old_mode != mode) {
                    publish = true;
                }
                old_mode = mode;
                setmode = false;
                publish = true;
            }
        }
        if (!setmode) {
            if (Mode == MODE_ZISTERNE) {
                new_valve = VALVE_ZISTERNE;
                Reason = 0;
            } else if (Mode == MODE_HAUSWASSER) {
                new_valve = VALVE_HAUSWASSER;
                Reason = 0;
            } else if (Mode == MODE_AUTO) {
                if (Liter >= sddata.limitHigh) {
                    new_valve = VALVE_ZISTERNE;
                    Reason = 1;
                }
                if (Liter <= sddata.limitLow) {
                    new_valve = VALVE_HAUSWASSER;
                    Reason = 2;
                }
            }
            ReasonText = reasons[Reason];

            Modus = modes[Mode];
        }
        /*-----------------------------------------------------------
        * Wenn Ventil umzuschalten ist
        */
        if (new_valve != Valve) {
            publish = true;
            Valve = new_valve;
            Ventil = valves[Valve];
            digitalWrite(VALVE_PIN, Valve);
            sprintf(buf, "Mode: %s, set Valve to %s (%s)", Modus, Ventil, ReasonText);
            Serial.println(buf);
        }

        // print out to Display
        Display();
        MqttPublish();
        updateMillis = millis();
    }
}
