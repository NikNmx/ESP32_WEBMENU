#include <Arduino.h>
#include <EEPROM.h>
//#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <NTPClient.h>
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
GyverDBFile db(&LittleFS, "/data4.db");

//#include <SettingsGyver.h>
//SettingsGyver sett("My Settings", &db);

//Create DB_structure
enum dbkeys : size_t {
    db_ssid = SH("db_ssid"),
    db_pass = SH("db_pass"),
    db_ssid_AP = SH("db_ssid_AP"),
    db_pass_AP = SH("db_pass_AP"), 
    db_ntp_server = SH("db_NTP_server"),
    db_timezone = SH("db_timezone"),
    db_inverse_input1 = SH("db_inverse_input1"),
    db_scales = SH("db_scales"),
    db_p2 = SH("db_p2"),
    db_p3 = SH("db_p3"),
    };



#define MAX_EEPROM_STRING_SIZE 256 
//global_variables for db parameters . Values are only for set variables types.
static String ssid = "IoTT";
static String password = "1111qQ1111qQ,@,@";
static String ssid_AP = "AP1";
static String password_AP = "00000000";
static String ntp_server = "pool.ntp.org";
static String timezone = "7200";
static String inverse_input1 = "true";
static String scales = "false";
static String p2 = "1234";
static String p3 = "pam_param";
        
IPAddress apIP(192, 168, 4, 2);

// counters. 
byte div_step = 0;
unsigned long last_millis = 0;
unsigned long current_millis;
unsigned long temp_millis = 0;

// classes
//DateTime now;
WiFiUDP ntpUDP;
AsyncWebServer server(80);
void handleWiFi();
void setup() {
    LittleFS.begin(true);
    db.begin();
    Serial.begin(115200);
    Serial.println("setup_begin\n");
    
    
    db.init(dbkeys::db_ssid, "IoTT");
    db.init(dbkeys::db_pass, "12345678qq12345678qq12345678");
    db.init(dbkeys::db_ssid_AP, "AP_IoTT");
    db.init(dbkeys::db_pass_AP, "12345678qq");
    db.init(dbkeys::db_ntp_server, "pool.ntp.org");
    db.init(dbkeys::db_timezone, "7200");
    db.init(dbkeys::db_inverse_input1, "true");
    db.init(dbkeys::db_scales, "false");
    db.init(dbkeys::db_p2, "1234");
    db.init(dbkeys::db_p3, "pam_param");
    
    ssid = db[dbkeys::db_ssid].toString();
    password = db[dbkeys::db_pass].toString();
    
    ssid_AP = db[dbkeys::db_ssid_AP].toString();
    password_AP = db[dbkeys::db_pass_AP].toString();
    
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
        char buffer[300];
        //Variant  
        //String aaa;
        //aaa = db[SH("db_ssid")].toString();

        //String bbb;
        //bbb = db[SH("db_pass")].toString();
        sprintf(buffer,
                "{\"ssid\": \"%s\", \"pass\": \"%s\",\"ssid_AP\": \"%s\", \"pass_AP\": \"%s\", \"ntp_server\": \"%s\", \"timezone\": \"%s\", \"inverse_input1\": \"%s\", \"scales\": \"%s\", \"p2\": \"%s\", \"p3\": \"%s\" }",
                
                //NTP_URL.c_str(), temperature_offset, seconds_offset,
                //WiFi.SSID().c_str(),
                //password);
                db[SH("db_ssid")].toString(),   //One of 2 Variants for access DB records
                db[SH("db_pass")].toString(),
                db[dbkeys::db_ssid_AP].toString(), // The other variant
                db[dbkeys::db_pass_AP].toString(),
                db[dbkeys::db_ntp_server].toString(),
                db[dbkeys::db_timezone].toString(),
                db[dbkeys::db_inverse_input1].toString(),
                db[dbkeys::db_scales].toString(),
                db[dbkeys::db_p2].toString(),
                db[dbkeys::db_p3].toString()
                    
                //bbb.c_str()
                );
        request->send(200, "text/plain", buffer);
        Serial.println("Buffer:\n");
        Serial.println(buffer);
    });
    server.on("/set_wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
        ssid = request->getParam("ssid")->value();
        password = request->getParam("pass")->value();
        ssid_AP = request->getParam("ssid_AP")->value();
        password_AP = request->getParam("pass_AP")->value();
        String rrr = request->methodToString(); //may be string
        request->send(200, "text/plain", "200");
        delay(20);
        //WiFi.softAPdisconnect(true);
        //WiFi.mode(WIFI_STA);
        //WiFi.persistent(true);
        Serial.println("Set/wifi_pressed\n");
        Serial.println(rrr);
        Serial.println(ssid);
        Serial.println(password);
        //Serial.println(ssidhtmlAP);
        //Serial.println(passwordhtmlAP);
        //WiFi.begin(ssidhtml, passwordhtml);
        db[dbkeys::db_ssid] = ssid;
        db[dbkeys::db_pass] = password;
        db[dbkeys::db_ssid_AP] = ssid_AP;
        db[dbkeys::db_pass_AP] = password_AP; 
        Serial.println("dbstore settings:");
        Serial.println(db["db_ssid"].c_str());
        Serial.println(db[dbkeys::db_pass].c_str());
        Serial.println(db[dbkeys::db_ssid_AP].c_str());
        Serial.println(db[dbkeys::db_pass_AP].c_str());
        
    });

    server.on("/sendParameters", HTTP_GET, [](AsyncWebServerRequest *request) {
        char buffer[200];
        //const url = `/sendParameters?
        //&ssid=${ssid}&pass=${pass}&ssid_AP=${ssid_AP}&pass_AP=${pass_AP}
        //&ntp_server=${ntp_server}&timezone=${timezone}
        //&inverse_input1=${inverse_input1}&scales=${scales}
        //&p2=${p2}&p3=${p3}`;
    
        Serial.println("/sendParameters called\n");
        ssid = request->getParam("ssid")->value();
        password = request->getParam("pass")->value();
        ssid_AP = request->getParam("ssid_AP")->value();
        password_AP = request->getParam("pass_AP")->value();
        ntp_server = request->getParam("ntp_server")->value();
        timezone = request->getParam("timezone")->value();
        inverse_input1 = request->getParam("inverse_input1")->value();
        scales = request->getParam("scales")->value();
        p2 = request->getParam("p2")->value();
        p3 = request->getParam("p3")->value();
        request->send(200, "text/plain", "200");
        delay(20);
        //db["db_ssid"] = "labelssid1"; //template
        db[dbkeys::db_ssid] = ssid;
        db[dbkeys::db_pass] = password;
        db[dbkeys::db_ssid_AP] = ssid_AP;
        db[dbkeys::db_pass_AP] = password_AP;
        db[dbkeys::db_ntp_server] = ntp_server;
        db[dbkeys::db_timezone] = timezone;
        db[dbkeys::db_inverse_input1] = inverse_input1;
        db[dbkeys::db_scales] = scales; 
        db[dbkeys::db_p2] = p2;
        db[dbkeys::db_p3] = p3;
        db.update(); 
        Serial.println("Saved parameters: ");
        Serial.println(ssid + " " + password + " " + ssid_AP + " " + password_AP + " " + ntp_server + " " + timezone + " " + inverse_input1 + " " + scales + " " + p2 + " " + p3);
        Serial.print("\n");
        Serial.print(db["db_ssid"]);
        if (scales == "true") {
        db.clear();    
        }
        

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
        Serial.println(ssid_AP);
        Serial.println(password_AP);
        WiFi.softAP(ssid_AP, password_AP);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        
            
            Serial.print("192.168.4.2\n");
            delay(5000);
        
    }
    startWebServer();
}