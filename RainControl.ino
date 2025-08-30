#include <Arduino.h>
#include <avr/dtostrf.h>
#include <PubSubClient.h>
#include <Time.h>

/* ---------------------------- allgemeine Settings ----------------------------- */
#define VERSION "RainControl V1.0"

// For Debugging with no Display connected, comment out for real-world
#define HAS_DISPLAY
// Minimize Serial Output
// #define DEBUG
// ##############################################################################################################################
// ---- HIER die Initialen Einstellungen vornehmen, die Werte sind über MQTT anpassbar
// ##############################################################################################################################
// Hier die maximale Füllmenge des Behälters angeben. Dies gilt nur für symmetrische Behälter.
// Deckelung bei 100%
// keine Deckelung bei "Übervoll", sondern Max-Wert ist maximal nutzbarer (Überlauf)
// Analoger Wert bei maximalem Füllstand (wird alle 30 Sekungen auf dem LCD angezeigt oder in der seriellen Konsole mit 9600 Baud.
// Maxwerte mit 100% - Deckelung
// const int analog_max = 972;
// const int analog_min = 50;
// Maxwerte ohne Deckelung bei 100%
#define ANALOG_MIN 50       // Analoger Messwert A0 (alles was kleiner ist, wird als "Sonde defekt oder nicht angeschlossen" gewertet)
#define ANALOG_MAX 1000     // Analoger Messwert A0 (bei 100% Füllgrad)
#define LITER_MIN 600       // Restfüllmenge bei Minimum (wird nur erreicht, wenn Übersteuert wird / wurde) - der Saugschlauch wird in der Regel nicht restlos leer pumpen
#define LITER_MAX 6500      // Maximale Füllmenge
#define LIMIT_LOW 700       // Liter
#define LIMIT_HIGH 1000     // Liter

#define MODE_PIN 13
#define MODE_ZISTERNE 0
#define MODE_HAUSWASSER 1
#define MODE_AUTO 2
#define MODE_COUNT 10
bool setmode = false;
char *modes[] = {"Zisterne", "Hauswasser", "Auto"};
int mode_count = MODE_COUNT;

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

unsigned long pinMillis;
unsigned long lastMillis = 0;

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
        const char *name;
        const char *fmt;
        bool changed;
    
    public:
        TopicBase(const char *_name, const char *_fmt = nullptr) : name(_name), fmt(_fmt), changed(true) {};
        virtual ~TopicBase() {};

        const char * getName() const { return name; };
        void setFormat(const char *_fmt) { fmt = _fmt; };
        void Changed() {changed = true;};

        // Von abgeleiteten Klassen (Topic) zu Implementieren
        virtual void Publish(PubSubClient *_mqttclient, char * _mqtt_id) = 0;
        virtual char *toString() = 0;
};

// ---------------------------------------------------------
// Hilfsfunktion für time_t → String
// ---------------------------------------------------------
inline void timeToString(const time_t* t, const char* fmt, char* buffer, size_t size) {
    struct tm *timeinfo = localtime(t);
    if (timeinfo) {
        strftime(buffer, size, fmt ? fmt : "%d.%m.%Y %H:%M:%S", timeinfo);
    } else {
        snprintf(buffer, size, "[invalid time]");
    }
}

template <typename Type>
class Topic : public TopicBase {
    private:
        Type data;

    public:
        Topic(const char *_name, Type value, const char *_fmt = nullptr) : TopicBase(_name, _fmt), data(value) { };

        // Zuweisungsoperator überladen
        Topic<Type>& operator=(const Type& value) {
            setData(value);
            return *this;
        }

        // Typ-Konvertierungsoperator (Lesen)
        operator Type() const {
            return getData();
        }
    
        // Getter, Setter
        Type getData() const { return data; }

        void setData(Type value) {
            if (value != data) {
                data = value;
                changed = true;
            }
        };

        char *toString() override {
            static char tostring[64];
            tostring[0] = 0;

            if constexpr (is_same_type<Type, const time_t>::value || is_same_type<Type, time_t>::value) {
                timeToString(&data, fmt, tostring, sizeof(tostring));
                return tostring;
            }
            
            if (fmt) {
                snprintf(tostring, sizeof(tostring), fmt, data);
                return tostring;
            }
            if (is_same_type<Type, int>::value) {
                snprintf(tostring, sizeof(tostring), "%d", data);
            } else if (is_same_type<Type, float>::value || is_same_type<Type, double>::value) {
                snprintf(tostring, sizeof(tostring), "%.3f", data);
            } else if (is_same_type<Type, const char*>::value || is_same_type<Type, char*>::value) {
                snprintf(tostring, sizeof(tostring), "%s", data);
            } else if (is_same_type<Type, const bool>::value || is_same_type<Type, bool>::value) {
                snprintf(tostring, sizeof(tostring), "%s", data ? "true" : "false");
            } else {
                snprintf(tostring, sizeof(tostring), "[unsupported type]");
            }
            return tostring;
        }


        void Publish(PubSubClient *_mqttclient, char * _mqtt_id) override {
            char _t[256];

            sprintf(_t, "%s/%s", _mqtt_id, name);
            if (changed) {
                if (_mqttclient->connected()) {
                    changed = false;
                    _mqttclient->publish(_t, toString());
                }
#ifdef DEBUG
                changed = false;
#endif
                Serial.print(_mqtt_id);
                Serial.print("/");
                Serial.print(name);
                Serial.print(" ");
                Serial.println(toString());
            }
        };
};

/* Definition der Topics zur Speicherung der Daten sowhol hier im Code, als auch in Richtung Broker */

#define TOPIC_LIST                                  \
    X(Timestamp, time_t, 0, "%d.%m.%Y %H:%M:%S")    \
    X(Mode, int, MODE_AUTO, "%d")                   \
    X(Modus, char *, modes[MODE_AUTO], "%s")        \
    X(Valve, int, VALVE_ZISTERNE, "%d")             \
    X(Ventil, char *, valves[VALVE_ZISTERNE], "%s") \
    X(Analog, int, 0, "%d")                         \
    X(Liter, int, 0, "%4d")                         \
    X(LimitLow, int, LIMIT_LOW, "%d")               \
    X(LimitHigh, int, LIMIT_HIGH, "%d")             \
    X(LiterMin, int, LITER_MIN, "%d")               \
    X(LiterMax, int, LITER_MAX, "%d")               \
    X(Prozent, double, 0.0, "%5.1f%%")              \
    X(Uptime, char*, "", "%s")                      \
    X(StatusSonde, bool, true, nullptr)             \
    X(HyaRain, char*, "", "%s")                     \
    X(Booted, time_t, 0, "%d.%m.%Y %H:%M:%S")       \
    X(AnalogMin, int, ANALOG_MIN, "%d")             \
    X(AnalogMax, int, ANALOG_MAX, "%d")             \
    X(CurrentFactor, double, 1.0, nullptr)          \
    X(Ampere, double, 0.0, "%.2f")                  \
    X(Power, double, 0.0, "%.2f")                   \
    X(KWh, double, 0.0, "%.2f")                 \
    X(Reason, int, 0, "%d")                         \
    X(ReasonText, char*, reasons[0], nullptr)


#define X(name, type, init, format)    Topic<type> name(#name, init, format);
TOPIC_LIST
#undef X

#define X(name, type, init, format) &name,
TopicBase* topics[] = { TOPIC_LIST };
#undef X

#define NUM_TOPICS (sizeof(topics) / sizeof(topics[0]))

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
#ifdef DEBUG
            Serial.print("C: ");
            Serial.print(count);
            Serial.print(", A: ");
            Serial.print(aggregate);
            Serial.print(", S:  ");
            Serial.println(scale);
#endif
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
    int literMin;
    int literMax;
    int limitLow;
    int limitHigh;
    double currentFactor;
} sddata = {"RC", ANALOG_MIN, ANALOG_MAX, LITER_MIN, LITER_MAX, LIMIT_LOW, LIMIT_HIGH, 15.0};
char *sdini_format = "%2s;%d;%d;%d;%d;%d;%d;%s;";

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
        Serial.print(MQTT_ID);
        Serial.print(" ... ");
        int ret = mqttclient.connect(MQTT_ID, mqttuser, mqttpass);
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
    for (int i = 0; i < NUM_TOPICS; i++) {
        topics[i]->Publish(&mqttclient, MQTT_ID);
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
        } else if (strcmp(key, "high") == 0) {
            LimitHigh = sddata.limitHigh = min(atoi(value), sddata.literMax);
            SDWriteSettings();
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
#ifdef DEBUG
        Serial.print("Key: ");
        Serial.print(key);
        Serial.print(" , Value: ");
        Serial.println(value);
#endif
        if (strcmp(key, "analogmin") == 0) {
            AnalogMin = sddata.analogMin = max(atoi(value), 0);
            SDWriteSettings();
        } else if (strcmp(key, "analogmax") == 0) {
            AnalogMax = sddata.analogMax = min(atoi(value), 1023);
            SDWriteSettings();
        } else if (strcmp(key, "litermax") == 0) {
            LiterMax = sddata.literMax = atoi(value);
            SDWriteSettings();
        } else if (strcmp(key, "factor") == 0) {
            CurrentFactor = sddata.currentFactor = atof(value);
            SDWriteSettings();
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
#ifdef DEBUG
            Serial.print("MqttCallback: ");
            Serial.println(callbacks[i].callback.c_str());
#endif
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
#include <U8g2lib.h>
U8G2_SSD1309_128X64_NONAME2_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 2, /* dc=*/ 3, /* reset=*/ 4);

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define FONT_HEIGHT 10
#define MAX_LINES (DISPLAY_HEIGHT / FONT_HEIGHT)

class DisplayConsole {
    U8G2 &display;
    String lines[MAX_LINES];        // Zeilenpuffer
    int currentLine = 0;
    
    public:
        DisplayConsole(U8G2 &disp) : display(disp) {};

        void begin() {
            display.clearBuffer();
            display.setFont(u8g2_font_t0_11_tf);
        }

        void println(const String &text) {
            lines[currentLine] = text;
            currentLine++;
            if(currentLine >= MAX_LINES) {
                scrollUp();
                currentLine = MAX_LINES - 1;
            }
            render();
        }

        // Überladung für IPAddress
        void println(const IPAddress &ip) {
            String ipStr = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
            println(ipStr); // ruft bestehenden println(String) auf
        }

        void print(const String &text) {
            lines[currentLine] += text;
            render();
        }

        void println(const char* text) {
            println(String(text)); // ruft das bestehende println(String) auf
        }

        void print(const char* text) {
            print(String(text)); // ruft das bestehende print(String) auf
        }

        void setLine(int index, const String &text) {
            if (index >= 0 && index < MAX_LINES) {
                lines[index] = text;
            }
        }

        String getLine(int index) {
            if (index >= 0 && index <= MAX_LINES) {
                return lines[index];
            }
            return "";
        }

        void scrollUp() {
            for (int i = 1; i< MAX_LINES; i++) {
                lines[i - 1] = lines[i];
            }
            lines[MAX_LINES - 1] = "";
        }

        void render() {
            display.clearBuffer();
            for (int i = 0; i < MAX_LINES; i++) {
                display.drawStr(0, (i + 1) * FONT_HEIGHT, lines[i].c_str());
            }
            display.sendBuffer();
        }

        void renderAll() {
            display.clearBuffer();

            for (int i = 0; i < MAX_LINES; i++) {
                display.drawStr(0, (i + 1) * FONT_HEIGHT, lines[i].c_str());
            }
            display.sendBuffer();
        }
};

DisplayConsole console(u8g2);

/*-------------------------------- Functions ---------------------------- */
bool SDLogData() {
    char logbuf[256];

    if (SDStatus) {
        Serial.print("Logging to SD Card...");
        if (SDLogger = SD.open(LOGFILE, FILE_WRITE)) {
            if (SDLogger.size() == 0) {
                for (int i = 0; i < (sizeof(topics) / sizeof(topics[0])); i++) {
                    SDLogger.write(topics[i]->getName());
                    SDLogger.write(';');
                }
                SDLogger.write('\n');
            }

            for (int i = 0; i < (sizeof(topics) / sizeof(topics[0])); i++) {
                SDLogger.write(topics[i]->toString());
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
    int aMin, aMax, lMin, lMax, lLow, lHigh;
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
                sprintf(line, "AnalogMin: %d, AnalogMax: %d, LiterMin: %d, LiterMax: %d, LimitLow: %d, LimitHigh: %d, cFactor: %s", aMin, aMax, lMin, lMax, lLow, lHigh, ccFactor[0]);
                Serial.println(line);
                AnalogMin       = sddata.analogMin = aMin;
                AnalogMax       = sddata.analogMax = aMax;
                LiterMin        = sddata.literMin = lMin;
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
        sprintf((char *)&sdbuf[0], sdini_format, sddata.id, sddata.analogMin, sddata.analogMax, sddata.literMin, sddata.literMax, sddata.limitLow, sddata.limitHigh, ccFactor);
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

        Serial.println(" * is a card inserted?");

        Serial.println(" * is your wiring correct?");

        Serial.println(" * did you change the chipSelect pin to matcjh your shield or module?");
        SDStatus = false;
    } else {
        Serial.println("Wiring is correct and a card is present.");

        SDReadSettings();

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
#ifdef DEBUG
    Serial.print("Uptime: ");
    Serial.println(uptime);
#endif
    if (setmode) {
        Serial.print(setmode ? " Settings-Mode" : "");
        sprintf(buf, " (%d)secs ", mode_count);
        Serial.println(buf);
    }
}

void CheckEthernet() {
#ifdef DEBUG
    Serial.println("Check Ethernet - Connection...");
#endif
    if (Ethernet.hardwareStatus() != EthernetNoHardware && Ethernet.linkStatus() == LinkON) {
        /*--------------------------------------------------------------
         *  check Ethernet services
         * --------------------------------------------------------------
         */
        switch (Ethernet.maintain()) {
            case 1:
                //renewed fail
                Serial.println(F("Error: renewed fail"));
                break;
    
            case 2:
#ifdef DEBUG
                //renewed success
                Serial.println(F("Renewed success"));
                //print your local IP address:
                Serial.print(F("My IP address: "));
                Serial.println(Ethernet.localIP());
#endif
                break;
        
            case 3:
                //rebind fail
                Serial.println(F("Error: rebind fail"));
                break; 
    
            case 4:
#ifdef DEBUG
                //rebind success
                Serial.println(F("Rebind success"));
                //print your local IP address:
                Serial.print(F("My IP address: "));
                Serial.println(Ethernet.localIP());
#endif
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
#ifdef DEBUG
    Serial.print("ModePin: ");
    Serial.println(pin_stat == LOW ? "Low" : "High");
#endif
    delay(1000);

    SDsetup();
    
    Serial.println("Initializing TCP");

#ifdef HAS_DISPLAY
    u8g2.begin();
    console.begin();
#endif

    ret = Ethernet.begin(mac);

    if (Ethernet.hardwareStatus() != EthernetNoHardware && Ethernet.linkStatus() == LinkON) {
        if (ret == 0) {
            Ethernet.begin(mac, ip, ns, gw, sn);
        }
        Udp.begin(8888);

#ifdef HAS_DISPLAY
        console.println(ret == 0 ? "Static" : "Dhcp");
        console.print("localIP: ");
        console.println(Ethernet.localIP());
        console.print("subnetMask: ");
        console.println(Ethernet.subnetMask());
        console.print("gatewayIP: ");
        console.println(Ethernet.gatewayIP());
        console.print("dnsServerIP: ");
        console.println(Ethernet.dnsServerIP());
#endif

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
        delay(1000);
        ntp.begin();
    } else {
        Serial.println("No Ethernet - either no Hardware attached or no Link!");
    }
    Serial.println("Setup Done!");
}

void CDisplay() {
#ifndef HAS_DISPLAY
    return;
#endif
    console.begin();
    console.setLine(0, VERSION);
    sprintf(buf, "Level : %s l %s", Liter.toString(), Prozent.toString());
    console.setLine(1, buf);
    if (!setmode) {
        sprintf(buf, "Modus : %s", Modus.toString());
    } else {
        sprintf(buf, "Modus : %s(%d)", Modus.toString(), mode_count);
    }
    console.setLine(2, buf);
    sprintf(buf, "Status: %s", ReasonText.toString());
    console.setLine(3, buf);
    sprintf(buf, "Ventil: %s", Ventil.toString());
    console.setLine(4, buf);
    console.setLine(5, Timestamp.toString());
    console.renderAll();
}

void loop() {
    // put your main code here, to run repeatedly:

    sct013.Read(average);
    Sonde.Read();

    pin_stat = digitalRead(MODE_PIN);
    pinMillis = millis();
    
    if (pin_stat == LOW) {
        if (pinMillis - lastMillis > 200) {
#ifdef DEBUG
            Serial.println("Impuls Low");
#endif
            lastMillis = pinMillis;
            mode_count = MODE_COUNT;
            if (!setmode) {
                setmode = true;
                old_mode = Mode;
#ifdef DEBUG
                sprintf(buf, "Button: Enter Settings-Mode %d (%s)", Mode, Modus);
                Serial.println(buf);
#endif
            } else {
                Mode = Mode - 1;
                if (Mode < 0) {
                    Mode = 2;
                }
                Modus = modes[Mode];
#ifdef DEBUG
                sprintf(buf, "Button: Mode change to %d (%s)", Mode, Modus.toString());
                Serial.println(buf);
#endif
            }
        }
    }


    if ((millis() - pubMillis) > SEND_INTERVAL * 1000) { // Alle 30 Sekunden
        for (int i = 0; i < (sizeof(topics) / sizeof(topics[0])); i++) {
            topics[i]->Changed();
        }
        // SDLogData();
        MqttPublish();
        pubMillis = millis();
    }
    if (millis() - updateMillis >= SEKUNDE) { // Hier eine Sekunde warten
        if (mqttclient.connected()) {
            mqttclient.loop();
        }
        StatusSonde = Analog >= sddata.analogMin;             // Sondenfehler ?
#ifdef DEBUG
        Serial.println("Analogen Wert lesen Sonde");
#endif
        /* Analoger Wert der Sonde geglättet */
        Analog = Sonde.Value();
        // Serial.println("Restart Sonde");
        Sonde.Restart();

        Prozent = (Analog - AnalogMin) * 100 / (AnalogMax - AnalogMin);                                 // Calculate Prozent
        Liter = LiterMin + (Analog - AnalogMin) * (LiterMax - LiterMin) / (AnalogMax - AnalogMin);      // calculate liter

        /* Analoger Wert des Stromsensors geglättet */
#ifdef DEBUG
        Serial.println("Analogen Wert lesen Stromsensor");
#endif
        Ampere = sct013.Amp();
        if (Ampere > 0) {
            HyaRain = "On";
        } else {
            HyaRain = "Off";
        }
        Power = sct013.Power();
        KWh = sct013.KWh();

        average = sct013.Average();
        sct013.Restart();

        if (Ethernet.hardwareStatus() != EthernetNoHardware && Ethernet.linkStatus() == LinkON) {
            ntp.update();

#ifdef DEBUG
            Serial.println("Get NTP-Time Epoch");
#endif
            Timestamp = ntp.epoch();
#ifdef DEBUG
            sprintf(buf, "%s", ntp.formattedTime("%d.%m.%Y %H:%M:%S"));
            Serial.println(buf);
#endif
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
                old_mode = Mode;
                setmode = false;
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
            Valve = new_valve;
            Ventil = valves[Valve];
            digitalWrite(VALVE_PIN, Valve);
#ifdef DEBUG
            sprintf(buf, "Mode: %s, set Valve to %s (%s)", Modus, Ventil, ReasonText);
            Serial.println(buf);
#endif
        }

        // print out to Display
        CDisplay();
        MqttPublish();
        updateMillis = millis();
    }
}
