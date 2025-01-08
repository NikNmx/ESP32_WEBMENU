//#include <DallasTemperature.h>
#include <EEPROM.h>
//#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <NTPClient.h>
//#include <OneWire.h>
//#include <RTClib.h>
//#include <SPI.h>
#include <WiFiUdp.h>
#include <pages.h>
#include <GyverDBFile.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#include <GyverDBFile.h>
#include <LittleFS.h>
GyverDBFile db(&LittleFS, "/data2.db");

//#include <SettingsGyver.h>
//SettingsGyver sett("My Settings", &db);

//Create DB_structure
DB_KEYS(
    keys,
    ssid,
    pass,
    ssid_AP,
    pass_AP,
    NTP_server,
    timezone);




#define MAX_EEPROM_STRING_SIZE 255


const char *ap_ssid = "AP_1";
const char *ap_password = "1234567890";
IPAddress apIP(192, 168, 4, 2);

// counters
byte div_step = 0;
unsigned long last_millis = 0;
unsigned long current_millis;
unsigned long temp_millis = 0;

// classes
DateTime now;
WiFiUDP ntpUDP;
AsyncWebServer server(80);

void setup() {
    Serial.begin(9600);
    handleWiFi();
}

void loop() {
    current_millis = millis();
    if (current_millis - last_millis >= 500) {
//        now = rtc.now();
//        calculateTime();
//        drawTime();
        div_step++;
        if (div_step > 3) {
            div_step = 0;
        }
        last_millis = current_millis;
    }
    if (current_millis - temp_millis >= 10000) {
        drawTemp();
        temp_millis = current_millis;
    }
    if (h_hours + h_minutes + h_seconds == "222300" &&
        WiFi.status() == WL_CONNECTED) {
        adjustRTCviaNTP();
    }
}

String unitHandler(int unit) {
    int a = unit / 10 % 10;
    int b = unit % 10;
    String complete_string = String(a) + String(b);
    return complete_string;
}

// byte place from 0x31 to 0x58 (1-40)
void setCursorVFD(byte place) {
    //                        EOT   SOH     P          ETB
    byte cursor_position[] = {0x04, 0x01, 0x50, place, 0x17};
    Serial.write(cursor_position, sizeof(cursor_position));
}


void calculateTime() {
    h_hours = unitHandler(now.hour());
    h_minutes = unitHandler(now.minute());
    h_seconds = unitHandler(now.second());
    h_month_day = unitHandler(now.day());
    h_month = unitHandler(now.month());
    year = now.year();
}

void adjustRTCviaNTP() {
    timeClient.setTimeOffset(seconds_offset);
    timeClient.setPoolServerName(NTP_URL.c_str());
    timeClient.begin();
    timeClient.update();
    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime((time_t *)&epochTime);
    rtc.adjust(DateTime(ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
                        timeClient.getHours(), timeClient.getMinutes(),
                        timeClient.getSeconds()));
    timeClient.end();
}

void adjustRTCviaPhone(int hours, int minutes, int seconds, int year, int month,
                       int day) {
    rtc.adjust(DateTime(year, month, day, hours, minutes, seconds));
}

void startWebServer() {
    // Main page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", main_page);
    });

    // Handler
    server.on("/set_timezone", HTTP_GET, [](AsyncWebServerRequest *request) {
        seconds_offset = request->getParam("offset")->value().toInt();
        String UTC_name = request->getParam("name")->value();
        EEPROM.put(seconds_offset_address, seconds_offset);
        EEPROM.commit();
        adjustRTCviaNTP();
        request->send(200, "text/plain", "200");
    });
    server.on("/sync_time", HTTP_GET, [](AsyncWebServerRequest *request) {
        int hours = request->getParam("hours")->value().toInt();
        int minutes = request->getParam("minutes")->value().toInt();
        int seconds = request->getParam("seconds")->value().toInt();
        int year = request->getParam("year")->value().toInt();
        int month = request->getParam("month")->value().toInt();
        int day = request->getParam("day")->value().toInt();
        adjustRTCviaPhone(hours, minutes, seconds, year, month, day);
        request->send(200, "text/plain", "200");
    });
    server.on(
        "/temperature_offset", HTTP_GET, [](AsyncWebServerRequest *request) {
            temperature_offset = request->getParam("offset")->value().toInt();
            EEPROM.put(temperature_offset_address, temperature_offset - 1);
            EEPROM.commit();
            request->send(200, "text/plain", "200");
        });
    server.on("/ntp_server", HTTP_GET, [](AsyncWebServerRequest *request) {
        NTP_URL = request->getParam("ntp_url")->value();
        writeStringToEEPROM(NTP_URL_address, NTP_URL);
        request->send(200, "text/plain", "200");
    });
    server.on("/get_data", HTTP_GET, [](AsyncWebServerRequest *request) {
        char buffer[100];
        sprintf(buffer,
                "{\"NTP\": \"%s\", \"temp_offset\": \"%d\", "
                "\"seconds_offset\": \"%d\", "
                "\"ssid\": \"%s\" }",
                NTP_URL.c_str(), temperature_offset, seconds_offset,
                WiFi.SSID().c_str());
        request->send(200, "text/plain", buffer);
    });
    server.on("/set_wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
        String ssid = request->getParam("ssid")->value();
        String password = request->getParam("password")->value();
        request->send(200, "text/plain", "200");
        delay(20);
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        WiFi.persistent(true);
        WiFi.begin(ssid, password);
    });

    server.on("/sendParameters", HTTP_GET, [](AsyncWebServerRequest *request) {
        String ssid = request->getParam("ssid")->value();
        String pass = request->getParam("pass")->value();
        String ssid_AP = request->getParam("ssid_AP")->value();
        String pass_AP = request->getParam("pass_AP")->value();
        String ntp_server = request->getParam("ntp_server")->value();
        String timezone = request->getParam("timezone")->value();
        bool inverse_input1 = request->getParam("inverse_input1")->value();
        bool scales = request->getParam("scales")->value();
        String p2 = request->getParam("p2")->value();
        String p3 = request->getParam("p3")->value();
        

//&ssid=${ssid}&pass=${pass}&ssid_AP=${ssid_AP}&pass_AP=${pass_AP}
//&ntp_server=${ntp_server}&timezone=${timezone}
//&inverse_input1=${inverse_input1}&scales=${scales}&p2=${p2}&p3=${p3}


        request->send(200, "text/plain", "200");
        delay(20);
       
    });
    server.begin();
}



void handleWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin();
    unsigned long startAttemptTime = millis();
    Serial.write("Loading");
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startAttemptTime < 5000) {
        setCursorVFD(0x38);
        if (div_step > 3) {
            div_step = 0;
        }
        Serial.write(date_div[div_step]);
        div_step++;
        delay(50);
    }

    if (WiFi.status() == WL_CONNECTED) {
        adjustRTCviaNTP();
        setCursorVFD(0x45);
        Serial.print(WiFi.localIP());
        delay(2000);
    } else {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(ap_ssid, ap_password);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        if (!rtc.isrunning()) {
            rtc.adjust(DateTime(2024, 2, 22, 22, 22, 22));
            clearVFD();
            setCursorVFD(0x31);
            Serial.print("Clock_AP vfdclock");
            setCursorVFD(0x45);
            Serial.print("192.168.4.2");
            delay(5000);
        }
    }
    startWebServer();
}
     