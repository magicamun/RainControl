// #include <FlashAsEEPROM.h>

#include <Arduino.h>
#include <avr/dtostrf.h>

/* -------------------------------------------- General Definitions ---------------------------------------- */
#define VERSION "RainControl V0.1a"
// ##############################################################################################################################
// ---- HIER die Anpassungen vornehmen ----
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

int percent = 0;
int liter;
int sonde = 0;                  // Durchschnittswert
int pointer = 0;                // Pointer für Messung

boolean state = false;

/* -------------------------------------------- Modus -------------------------------------- */
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

/* -------------------------------------------- Ventil -------------------------------------- */
#define VALVE_PIN 14
#define VALVE_ZISTERNE LOW
#define VALVE_HAUSWASSER HIGH
int valve = VALVE_ZISTERNE;
int new_valve = VALVE_ZISTERNE;
int reason = 0;
char *valves[] = {"Zisterne", "Hauswasser"};
char *ventil = valves[valve];
char *reasons[] = {"Manuell", "Zisterne voll", "Zisterne leer"};

// Interval-Settings
unsigned long pubMillis;
unsigned long startMillis;  // some global variables available anywhere in the program
unsigned long updateMillis;     
#define SEKUNDE 1000  // one second

unsigned long pinMillis;
unsigned long lastMillis = 0;
boolean publish = false;

char uptime[20];
char *up = &uptime[0];
char timestamp[80];
char *datzeit = &timestamp[0];
char booted[80];
char *boot = &booted[0];
time_t boot_t = 0;
time_t timestamp_t = 0;

char buf[255];

double current;
double power;

/*
SD - Card - Definitions */
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
    double curFactor;
} sddata = {"RC", ANALOG_MIN, ANALOG_MAX, MAX_LITER, LIMIT_LOW, LIMIT_HIGH, 15.0};
char *sdini_format = "%2s;%d;%d;%d;%d;%d;%s";

/*
Ethernet
*/
#include <Ethernet.h>
#include <EthernetUdp.h>
// MAC-Addresse bitte anpassen! Sollte auf dem Netzwerkmodul stehen. Ansonsten eine generieren.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x0C };

// IP Adresse, falls kein DHCP vorhanden ist. Diese Adresse wird nur verwendet, wenn der DHCP-Server nicht erreichbar ist.
IPAddress ip(192, 168,   3,  95);
IPAddress ns(192, 168,   3, 254);
IPAddress gw(192, 168,   3, 254);
IPAddress sn(255, 255, 255,   0);

EthernetClient ethClient;
EthernetUDP Udp;

/* ------------------------------------------- NTP and Time ----------------------------------------- */
#include <NTP.h>
#define NTPSERVER "pool.ntp.org"
#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR (60 * SECONDS_PER_MINUTE)
#define SECONDS_PER_DAY (24 * SECONDS_PER_HOUR)
bool ntp_time = false;
NTP ntp(Udp);


/* ------------------------------------------ Display ----------------------------------------------- */
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1309_128X64_NONAME2_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 2, /* dc=*/ 3, /* reset=*/ 4);


/* ------------------------------------------- Analog Input ----------------------------------------- */

int sondePin = A0;
int sctPin = A1;

/*
Class for handling analog Input
*/
class AI {
    protected:
        int pin;          // Analog Pin
        int count;        // Number of Values added
        long aggregate;   // All Values summed up
        long offset;      // For Values oszillating an offset (e.g. AC-Current)
    public:
        AI(int _pin) {
            pin = _pin;
            count = 0;
            aggregate = 0;
            offset = 0;
        };
        int Count() { return count;};
        void Read() {                                               // Aggregate the Values
            int val = analogRead(pin);
            aggregate += val;
            count++;
        }; 
        void ReadAC(int _avg = 512) {                               // Aggregate the Value as abs difference to the offset - for oszillating Values (AC-Current)
            int val = analogRead(pin);
            aggregate += abs(val - _avg);
            offset += val;
            count++;
        };
        int Value() { return aggregate / count;};                   // Return the median value
        int Offset() {return offset / count;};                      // Return a derived offset from the values
        void Restart() { count = 0; aggregate = 0; offset = 0;};    // Restart the measuring
};

/*
Class for gathering Ampere-Equivalents for AC - Current , Energymonitor SCT013 - based
*/
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
    public:
        Energy(int _pin, double _ampmax, double _vmax, double _volt = 230, double _vrefmax = 3.3, double _vrefmin = 0, int _imax = 1024, int _imin = 0) : AI(_pin) {
            ampmax = _ampmax;
            vmax = _vmax;
            volt = _volt;
            vrefmax = _vrefmax;
            vrefmin = _vrefmin;
            imax = _imax;
            imin = _imin;

            scale = ampmax * (vrefmax - vrefmin) / (imax - imin) / vmax;
        };
        double Scale() { return scale; };
        double Amp() { amp = (double) aggregate / (double) count * scale; return amp;};
        double Volt() { return volt; };
        double Watt() { return Volt() * Amp(); };
};

Energy SCT013(sctPin, 5.0, 1.0, 230);
AI Sonde(sondePin);

/* -------------------------------------- MQTT definitions ---------------------------------------- */

#include <PubSubClient.h>
// MQTT global vars
#define SEND_INTERVAL 30               // the sending interval of indications to the server, by default 10 seconds
#define MQTT_ID "RainControl"

boolean mqttconnected = false;

void MqttCallback(char *topic, byte *payload, unsigned int length);

// IP Adresse und Port des MQTT Servers
IPAddress mqttserver(192, 168, 3, 33);
const int mqttport = 1883;

// Wenn der MQTT Server eine Authentifizierung verlangt, bitte folgende Zeile aktivieren und Benutzer / Passwort eintragen
const char *mqttuser = "mqttiobroker";
const char *mqttpass = "Waldweg3";
PubSubClient mqttclient(ethClient);

// Topics

struct Topic {
   char *topic;
   char type;
   unsigned long pointer;
};

typedef Topic t_Topic;

t_Topic topics[] = {
    {"Timestamp",   'C', (unsigned long) &datzeit},
    {"Mode",        'I', (unsigned long) &mode},
    {"Valve",       'I', (unsigned long) &valve},
    {"Sonde",       'I', (unsigned long) &sonde},
    {"Liter",       'I', (unsigned long) &liter},
    {"LimitLow",    'I', (unsigned long) &sddata.limitLow},
    {"LimitHigh",   'I', (unsigned long) &sddata.limitHigh},
    {"LiterMax",    'I', (unsigned long) &sddata.literMax},
    {"Prozent",     'I', (unsigned long) &percent},
    {"Mode",        'I', (unsigned long) &mode},
    {"Valve",       'I', (unsigned long) &valve},
    {"Uptime",      'C', (unsigned long) &up},
    {"Status",      'B', (unsigned long) &state},
    {"Ventil",      'C', (unsigned long) &ventil},
    {"Modus",       'C', (unsigned long) &modus},
    {"Booted",      'C', (unsigned long) &boot},
    {"AnalogMin",   'I', (unsigned long) &sddata.analogMin},
    {"AnalogMax",   'I', (unsigned long) &sddata.analogMax}, 
    {"Calibration", 'F', (unsigned long) &sddata.curFactor},
    {"Current",     'F', (unsigned long) &current},
    {"Power",       'F', (unsigned long) &power},
};

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
    } else {
        Serial.println("MQTT - publishing...");
        for (int i = 0; i < (sizeof(topics) / sizeof(topics[0])); i++) {
            sprintf(topic, "%s/%s", mqttid.c_str(), topics[i].topic);
            if (topics[i].type == 'I') {
                sprintf(payload, "%d", *(int *)topics[i].pointer);
            } else if (topics[i].type == 'F') {
                dtostrf(*(double *)topics[i].pointer, 5, 2, payload);
                // sprintf(payload, "%5.2f", *(double *)topics[i].pointer);
            } else if (topics[i].type == 'C') {
                sprintf(payload, "%s", *(char **)topics[i].pointer);
            } else if (topics[i].type == 'B') {
                sprintf(payload, "%s", *(boolean *)topics[i].pointer ? "true" : "false");
            }
            mqttclient.publish(topic, payload);
            Serial.print(topic);
            Serial.print(" ");
            Serial.println(payload);
        }
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
            sddata.limitLow = max(atoi(value), 0);
            SDWriteSettings();
            publish = true;
        } else if (strcmp(key, "high") == 0) {
            sddata.limitHigh = min(atoi(value), sddata.literMax);
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
        mode = MODE_ZISTERNE;
    } else if (strcmp(payload, "1") == 0 || strcmp(payload, "hauswasser") == 0) {
        mode = MODE_HAUSWASSER;
    } else if (strcmp(payload, "2") == 0 || strcmp(payload, "auto") == 0) {
        mode = MODE_AUTO;
    }
    modus = modes[mode];
    Serial.print("Cmd - Mode: ");
    Serial.println(modus);
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
            sddata.analogMin = max(atoi(value), 0);
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
            sddata.curFactor = atof(value);
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
/* -------------------------------------- MQTT ---------------------------------------- */

void Uptime() {
  int days, secs, hours, mins;

    secs = timestamp_t - boot_t;

    days = secs / (SECONDS_PER_DAY);
    secs = secs - (days * SECONDS_PER_DAY);
    hours = secs / (SECONDS_PER_HOUR);
    secs = secs - (hours * SECONDS_PER_HOUR);
    mins = secs / SECONDS_PER_MINUTE;
    secs = secs - mins * SECONDS_PER_MINUTE;

    sprintf(uptime, "%4dd %2dh %2dm", days, hours, mins);
    uptime[14]= 0;
    // Serial.print("Uptime: ");
    // Serial.print(uptime);
    if (setmode) {
        Serial.print(setmode ? " Settings-Mode" : "");
        sprintf(buf, " (%d)secs ", mode_count);
        Serial.println(buf);
        // Serial.print(" ");
    }
    // Serial.println("");
}

void CheckEthernet() {
    if (Ethernet.hardwareStatus() != EthernetNoHardware && Ethernet.linkStatus() == LinkON) {
        /*--------------------------------------------------------------
           check ehternet services
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

/*
void ReadSonde() {
    float calc;
    // read the analog value and build floating middle
    myArray[pointer++] = analogRead(sondePin);      // read the input pin

    pointer = pointer % messungen;

    // Werte aufaddieren
    sonde = 0;
    for (int i = 0; i < messungen; i++) {
        sonde = sonde + myArray[i];
    }
    // Summe durch Anzahl - geglättet
    sonde = sonde / messungen;

    percent = sonde * 100 / sddata.analogMax;
    
    liter = sonde * sddata.literMax / sddata.analogMax;      // calculate liter
}
*/

bool SDLogData() {
    char logbuf[256];

    if (SDStatus) {
        Serial.print("Logging to SD Card...");
        if (SDLogger = SD.open(LOGFILE, FILE_WRITE)) {
            if (SDLogger.size() == 0) {
                for (int i = 0; i < (sizeof(topics) / sizeof(topics[0])); i++) {
                    SDLogger.write(topics[i].topic);
                    SDLogger.write(';');
                }
                SDLogger.write('\n');
            }

            for (int i = 0; i < (sizeof(topics) / sizeof(topics[0])); i++) {
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
    char cFactor[64];

    if (SDStatus) {
        Serial.print("read Settings from SD Card...");
        if (SDSettings = SD.open(SETTINGSFILE, FILE_READ)) {
            while (SDSettings.available() && i < 256) {
                sdbuf[i++] = SDSettings.read();
            }
            SDSettings.close();

            sscanf(sdbuf, sdini_format, &id[0], &aMin, &aMax, &lMax, &lLow, &lHigh, &cFactor[0]);
            Serial.println("ok");
            if (id[0] == 'R' && id[1] == 'C') {
                sprintf(line, "AnalogMin: %d, AnalogMax: %d, LiterMax: %d, LimitLow: %d, LimitHigh: %d, cFactor: %5.2f", aMin, aMax, lMax, lLow, lHigh, cFactor);
                Serial.println(line);
                sddata.analogMin = aMin;
                sddata.analogMax = aMax;
                sddata.literMax = lMax;
                sddata.limitLow = lLow;
                sddata.limitHigh = lHigh;
                sddata.curFactor = atof(cFactor);
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
    char cFactor[64];

    if (SDStatus) {
        dtostrf(sddata.curFactor, 6, 2, cFactor);
        sprintf((char *)&sdbuf[0], sdini_format, sddata.id, sddata.analogMin, sddata.analogMax, sddata.literMax, sddata.limitLow, sddata.limitHigh, cFactor);
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
    // DisplayShow();
}

void setup() {
    int ret = 0;
  
      /*-------------------------------------------------------------------
     * Setup Pins for Valve and mode-setting
     */
    pinMode(VALVE_PIN, OUTPUT);

    pinMode(MODE_PIN, INPUT_PULLUP);
    pin_stat = old_stat = digitalRead(MODE_PIN);
    Serial.print("ModePin: ");
    Serial.println(pin_stat == LOW ? "Low" : "High");

    delay(1500);

    /* Prepare for NTP */
    timestamp_t = 0;
    boot_t = 0;
    ntp_time = false;
    ntp.ruleDST("CEST", Last, Sun, Mar, 2, 120); // last sunday in march 2:00, timetone +120min (+1 GMT + 1h summertime offset)
    ntp.ruleSTD("CET", Last, Sun, Oct, 3, 60); // last sunday in october 3:00, timezone +60min (+1 GMT)
    ntp.updateInterval(1000);

    /*--------------------------------------------------------------
       Milliseconds start
      --------------------------------------------------------------*/
    updateMillis = pubMillis = startMillis = millis();  //initial start time
    
    Serial.begin(115200);

    SDsetup();

    Serial.println("Initializing Ethernet TCP and UDP");
  
    if (Ethernet.hardwareStatus() != EthernetNoHardware && Ethernet.linkStatus() == LinkON) {
        ret = Ethernet.begin(mac);
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
        
        // Do NOT use localized Time form NTP - localization is done manually on display/printuing only
        Serial.println("Setup NTP");
        delay(2000);    // Wait 2 Seconds
        ntp.begin();
    } else {
        Serial.println("Either no Ethernet-Hardware-Shield or no Link");
    }

    /* Connect to MQTT-Server, it fails when Ethernet is not up, but will retry and not block */
    mqttclient.setServer(mqttserver, 1883);
    mqttclient.setCallback(MqttCallback);
    MqttConnect();

    /* Start Display */
  
    u8g2.begin();
    Display();
}

void Display() {
  
    u8g2.clearBuffer();					// clear the internal memory
    u8g2.setFont(u8g2_font_t0_11_tf);	// choose a suitable font

    sprintf(buf, "%s", VERSION);
    u8g2.drawStr(0,10, buf);	// write something to the internal memory
    sprintf(buf, "Level : %4d l, %3d%%", liter, percent);
    Serial.println(buf);
    u8g2.drawStr(0,20, buf);	// write something to the internal memory
    if (!setmode) {
        sprintf(buf, "Modus : %s", modus);
    } else {
        sprintf(buf, "Modus : %s(%d)", modus, mode_count);
    }
    u8g2.drawStr(0,30, buf);
    sprintf(buf, "Status: %s", reasons[reason]);
    u8g2.drawStr(0,40, buf);
    sprintf(buf, "Ventil: %s", ventil);
    u8g2.drawStr(0,50, buf);
    u8g2.drawStr(0,60, datzeit);

    u8g2.sendBuffer();					// transfer internal memory to the display
}


void loop() {
    // put your main code here, to run repeatedly:

    pin_stat = digitalRead(MODE_PIN);
    pinMillis = millis();
    publish = false;
    Sonde.Read();
    SCT013.ReadAC();

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
                mode--;
                if (mode < 0) {
                    mode = 2;
                }
                modus = modes[mode];
                sprintf(buf, "Button: Mode change to %d (%s)", mode, modes[mode]);
                Serial.println(buf);
            }
        }
    }

    if (mqttclient.connected()) {
        mqttclient.loop();
    }

    /******************************************************************************************
    SEND_INTERVAL (30 Sekunden) Sekunden Takt
    *****************************************************************************************/ 
    if ((millis() - pubMillis) > SEND_INTERVAL * SEKUNDE) { // Alle 30 Sekunden
        state = sonde >= sddata.analogMin;             // Sondenfehler ?
        // Erst nach 2 Minuten das erste mal senden - bis dahin hat sich der Wert eingeregelt
        if (timestamp_t - boot_t > 2 * SECONDS_PER_MINUTE) {
            publish = true;
        }
        pubMillis = millis();
    }
 
    /******************************************************************************************
    1 Sekunden Takt
    *****************************************************************************************/ 
    if (millis() - updateMillis >= SEKUNDE) { // Hier eine Sekunde warten
        //´Wenn noch keine Zeit geholt wurde oder neu geholt werden muss
        if (timestamp_t == 0 || !ntp_time && (Ethernet.hardwareStatus() != EthernetNoHardware && Ethernet.linkStatus() == LinkON)) {
            // GetNTPTime();
            ntp.update();
            timestamp_t = ntp.epoch();
        }

        // ReadAnalog();
        sonde = Sonde.Value();
        percent = sonde * 100 / sddata.analogMax;
        liter = sonde * sddata.literMax / sddata.analogMax;      // calculate liter
        Sonde.Restart();

        CheckEthernet();
        Uptime();
        Serial.print("Ampere : ");
        Serial.println(SCT013.Amp());
        dtostrf(SCT013.Amp(), 6, 2, buf);


        // sprintf(timestamp, "%d.%d.%d %02d:%02d:%02d", (int)rtc.getDay(), (int)rtc.getMonth(), (int)rtc.getYear(), (int)rtc.getHours(), (int)rtc.getMinutes(), (int)rtc.getSeconds());
        // sprintf(timestamp, "%s", PrintTimeStamp(localTimefromUTC(timestamp_t)));
        sprintf(timestamp, "%s", ntp.formattedTime("%d.%m.%Y %H:%M:%S"));


        
        if (boot_t == 0) {
            boot_t = timestamp_t;
            sprintf(booted, "%s", timestamp);
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
            if (mode == MODE_ZISTERNE) {
                new_valve = VALVE_ZISTERNE;
                reason = 0;
            } else if (mode == MODE_HAUSWASSER) {
                new_valve = VALVE_HAUSWASSER;
                reason = 0;
            } else if (mode == MODE_AUTO) {
                if (liter >= sddata.limitHigh) {
                    new_valve = VALVE_ZISTERNE;
                    reason = 1;
                }
                if (liter <= sddata.limitLow) {
                    new_valve = VALVE_HAUSWASSER;
                    reason = 2;
                }
            }
            modus = modes[mode];
        }
        /*-----------------------------------------------------------
         * Wenn Ventil umzuschalten ist
        */
        if (new_valve != valve) {
            publish = true;
            valve = new_valve;
            ventil = valves[valve];
            digitalWrite(VALVE_PIN, valve);
            sprintf(buf, "Mode: %s, set Valve to %s (%s)", modes[mode], ventil, reasons[reason]);
            Serial.println(buf);
        }
        // print out to Display
        Display();
        updateMillis = millis();
    }
    if (publish) {
        Serial.println(ntp.formattedTime("%d.%m.%Y %H:%M:%S"));
        MqttPublish();
        SDLogData();
    }
}