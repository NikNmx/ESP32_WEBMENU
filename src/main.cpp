#include <Arduino.h>
#include <EEPROM.h>
//#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
//#include <NTPClient.h>
//#include <OneWire.h>
//#include <RTClib.h>
//#include <SPI.h>
#include <WiFiUdp.h>
#include <pages.h>
#include <GyverDB.h>
#include <GyverDBFile.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif


#include <LittleFS.h>
GyverDBFile db(&LittleFS, "/data2.db");

//#include <SettingsGyver.h>
//SettingsGyver sett("My Settings", &db);

//Create DB_structure
enum dbkeys : size_t {
    dbkeys,
    db_ssid = SH("db_ssid"),
    db_pass = SH("db_pass"),
    db_ssid_AP = SH("db_ssid_AP"),
    db_pass_AP = SH("db_pass_AP"), 
    db_NTP_server = SH("db_NTP_server"),
    db_timezone = SH("db_timezone"),
    db_inverse_input1 = SH("db_inverse_input1"),
    db_scales = SH("db_scales"),
    db_p2 = SH("db_p2"),
    db_p3 = SH("db_p3"),
    };



#define MAX_EEPROM_STRING_SIZE 256


const char *ssid = "IoTT";
const char *password = "1111qQ1111qQ,@,@";
void handleWiFi();

IPAddress apIP(192, 168, 4, 2);

// counters
byte div_step = 0;
unsigned long last_millis = 0;
unsigned long current_millis;
unsigned long temp_millis = 0;

// classes
//DateTime now;
WiFiUDP ntpUDP;
AsyncWebServer server(80);

void setup() {
    LittleFS.begin();
    db.begin();
    Serial.begin(115200);
    Serial.println("setup_begin\n");
    
    handleWiFi();

}

void loop() {
    //Serial.print(".");
current_millis = millis();
db.tick();

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






void startWebServer() {
    // Main page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", main_page);
    });

    // Handler
    server.on("/set_timezone", HTTP_GET, [](AsyncWebServerRequest *request) {
        int seconds_offset = request->getParam("offset")->value().toInt();
        String UTC_name = request->getParam("name")->value();
        //EEPROM.put(seconds_offset_address, seconds_offset);
        //EEPROM.commit();
        
        request->send(200, "text/plain", "200");
    });
    server.on("/sync_time", HTTP_GET, [](AsyncWebServerRequest *request) {
        int hours = request->getParam("hours")->value().toInt();
        int minutes = request->getParam("minutes")->value().toInt();
        int seconds = request->getParam("seconds")->value().toInt();
        int year = request->getParam("year")->value().toInt();
        int month = request->getParam("month")->value().toInt();
        int day = request->getParam("day")->value().toInt();
        //adjustRTCviaPhone(hours, minutes, seconds, year, month, day);
        request->send(200, "text/plain", "200");
    });
    server.on(
        "/temperature_offset", HTTP_GET, [](AsyncWebServerRequest *request) {
            int temperature_offset = request->getParam("offset")->value().toInt();
            //EEPROM.put(temperature_offset_address, temperature_offset - 1);
            //EEPROM.commit();
            request->send(200, "text/plain", "200");
        });
    server.on("/ntp_server", HTTP_GET, [](AsyncWebServerRequest *request) {
        String NTP_URL = request->getParam("ntp_url")->value();
        //writeStringToEEPROM(NTP_URL_address, NTP_URL);
        request->send(200, "text/plain", "200");
    });
    server.on("/get_data", HTTP_GET, [](AsyncWebServerRequest *request) {
        char buffer[100];
        sprintf(buffer,
                "{\"ssid\": \"ssid_id\", \"pass\": \"procent_s\", \"scales\": \"true\", \"timezone\": \"10800\"  }",
                //NTP_URL.c_str(), temperature_offset, seconds_offset,
                WiFi.SSID().c_str(),
                password);
        request->send(200, "text/plain", buffer);
        Serial.println("Buffer:\n");
        Serial.println(buffer);
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
        char buffer[200];
        Serial.println("/sendParameters called\n");
        String ssid1 = request->getParam("ssid")->value();
        String pass1 = request->getParam("pass")->value();
        //String ssid_AP1 = request->getParam("ssid_AP")->value();
        //String pass_AP1 = request->getParam("pass_AP")->value();
        //String ntp_server1 = request->getParam("ntp_server")->value();
        //String timezone1 = request->getParam("timezone")->value();
        //String inverse_input11 = request->getParam("inverse_input1")->value();
        //String scales1 = request->getParam("scales")->value();
        //String p21 = request->getParam("p2")->value();
        //String p31 = request->getParam("p3")->value();
        request->send(200, "text/plain", "200");
        delay(20);
        Serial.print(ssid1);
        Serial.print(pass1);
        Serial.print("\n");

        

//&ssid=${ssid}&pass=${pass}&ssid_AP=${ssid_AP}&pass_AP=${pass_AP}
//&ntp_server=${ntp_server}&timezone=${timezone}
//&inverse_input1=${inverse_input1}&scales=${scales}&p2=${p2}&p3=${p3}


        request->send(200, "text/plain", "200");
        delay(20);
       
    });
    server.begin();
}



void handleWiFi() {
    Serial.print("StartWifi\n");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    unsigned long startAttemptTime = millis();
    Serial.print("Loading\n");
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startAttemptTime < 5000) {
    Serial.print("Wifi_not_connected!\n");       
        if (div_step > 3) {
            div_step = 0;
        }
        Serial.write("div_step+1");
        div_step++;
        delay(50);
    }

    if (WiFi.status() == WL_CONNECTED) {
        
        
        Serial.print(WiFi.localIP());
        delay(2000);
    } else {
        WiFi.mode(WIFI_AP);
        Serial.print("Soft_APstart\n");
        WiFi.softAP(ssid, password);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        
            Serial.print("Clock_AP vfdclock\n");
            //setCursorVFD(0x45);
            Serial.print("192.168.4.2\n");
            delay(5000);
        
    }
    startWebServer();
}