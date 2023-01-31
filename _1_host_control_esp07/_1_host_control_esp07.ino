/*
   HID=AB:DC:76:5F:FE:39,T=24/12/22,14:30:15,B=45%,Z=1,TID=CA:14:CF:5A:45:4A|
   ZONEINFO,DZ1=1500,DZ2=2000,DZ3=2500,SZ1=1,SZ2=3,SZ3=2
*/



#include <AsyncElegantOTA.h>
#include "LittleFS.h"
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

String ssid;
String password;
String datasend = "";
String a = "";
AsyncWebServer server(80);
void setup() {
  Serial.begin(115200);
  if (!LittleFS.begin()) {
    return;
  }
  delay(2000);
  setupwifi();


}

void loop() {
  if (Serial.available()) {
    datasend = Serial.readStringUntil('\n');
    if (datasend.indexOf("RSSI VALUE:") >=0) {
//      Serial.println(a);
      a = datasend.substring(11,datasend.indexOf("|"));
      File file1 = LittleFS.open("/CALIBRATION.txt", "w+");
      file1.println(a);
      file1.close();
    }
    File file = LittleFS.open("/EVENT.txt", "a");
    file.println(datasend);
    file.close();
    //      Serial.println(data);
  }
}

void setupwifi() {
  if (readfromFlash("/ZONEINFO.txt") == "") {
    writetoFlash("/ZONEINFO.txt", "ZONEINFO,DZ1=3000,DZ2=1500,DZ3=1500,SZ1=1,SZ2=2,SZ3=3", "w+");
  }

  Serial.println(readfromFlash("/ZONEINFO.txt"));
  delay(1000);
  //write rssi
  if (readfromFlash("/VALUERSSI.txt") == "") {
    writetoFlash("/VALUERSSI.txt", "SETRSSI,1000=-81,1500=-87,2000=-91,2500=-94,3000=-97,3500=-100,4000=-106,4500=-109,5000=-112,5500=-115,6000=-120,6500=-130,7000=-140,7500=-145,8000=-150", "w+");
  }
  Serial.println(readfromFlash("/VALUERSSI.txt"));
  delay(140);
  //setStateRelay
  if (readfromFlash("/RELAYSTATE.txt") == "") {
    writetoFlash("/RELAYSTATE.txt", "OFF", "w+");
  }
  Serial.println("setStateRelay:" + readfromFlash("/RELAYSTATE.txt")); //gửi tới stm32 "setStateRelay:ON",setStateRelay:OFF
  delay(140);
  if (readfromFlash("/SYSTEMSTATE.txt") == "") {
    writetoFlash("/SYSTEMSTATE.txt", "ACTIVATE", "w+");
  }
  Serial.println("isState:" + readfromFlash("/SYSTEMSTATE.txt")); //gửi tới stm32 "isState:ACTIVATE",isState:IDLE
  delay(140);
  

  if (readfromFlash("/SSIDWIFI.txt") == "") {
    writetoFlash("/SSIDWIFI.txt", "HOST - AB:DC:76:5F:FE:39", "w+");
    //    writetoFlash("/SSIDWIFI.txt", "HOST - 15:EF:01:AB:FE:00", "w+");
    //    15:EF:01:AB:FE:00
    //    ssid = "HOST - AB:DC:76:5F:FE:39";
  }
  if (readfromFlash("/PASSWORDWIFI.txt") == "") {
    writetoFlash("/PASSWORDWIFI.txt", "InsB202C", "w+");
    //    password = "InsB202C";
  }
  ssid = readfromFlash("/SSIDWIFI.txt");
  password = readfromFlash("/PASSWORDWIFI.txt");
  delay(40);
  //  Serial.print("ssid:");
  //  Serial.println(ssid);

  //  Serial.print("password:");
  //  Serial.println(password);

  WiFi.softAP(ssid.c_str(), password.c_str());
  IPAddress IP = WiFi.softAPIP();
  //  Serial.print("AP IP address: ");
  //  Serial.println(IP);

  // Print ESP8266 Local IP Address
  //  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/index.html", String(), false, processor);
  });

  server.on("/History-1.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/History-1.html", String(), false, processor);
  });
  server.on("/History-2.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/History-2.html", String(), false, processor);
  });
  server.on("/History-3.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/History-3.html", String(), false, processor);
  });
  server.on("/configuration.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/configuration.html", String(), false, processor);
  });




  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/style.css", "text/css");
  });

  server.on("/src/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/src/bootstrap.min.js", "text/javascrip");
  });
  server.on("/src/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/src/bootstrap.min.css", "text/css");
  });
  server.on("/src/jquery-3.3.1.min.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/src/jquery-3.3.1.min.js", "text/javascrip");
  });

  server.on("/src/moment.min.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/src/moment.min.js", "text/javascrip");
  });
  server.on("/src/datetimepicker.min.css", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/src/datetimepicker.min.css", "text/css");
  });
  server.on("/src/datetimepicker.min.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/src/datetimepicker.min.js", "text/javascrip");
  });


  server.on("/fonts/glyphicons.ttf", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/fonts/glyphicons.ttf", "text/plain");
  });
  server.on("/fonts/glyphicons.woff", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/fonts/glyphicons.woff", "text/plain");
  });
  server.on("/fonts/glyphicons.woff2", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/fonts/glyphicons.woff2", "text/plain");
  });

  server.on("/src/table2excels.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/src/table2excels.js", "text/javascrip");
  });
  server.on("/src/xlsxmin.min.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/src/xlsxmin.min.js", "text/javascrip");
  });

  server.on("/ZoneImage.jpg", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/ZoneImage.jpg", "image/jpeg");
  });



  //public file data txt
  server.on("/ZONEINFO.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/ZONEINFO.txt", "text/plain");
  });
  server.on("/EVENT.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/EVENT.txt", "text/plain");
  });
  server.on("/SSIDWIFI.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/SSIDWIFI.txt", "text/plain");
  });
  server.on("/PASSWORDWIFI.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/PASSWORDWIFI.txt", "text/plain");
  });
  server.on("/SYSTEMSTATE.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/SYSTEMSTATE.txt", "text/plain");
  });

  server.on("/RELAYSTATE.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/RELAYSTATE.txt", "text/plain");
  });
  server.on("/CALIBRATION.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/CALIBRATION.txt", "text/plain");
  });
  //VALUERSSI.txt
  server.on("/VALUERSSI.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/VALUERSSI.txt", "text/plain");
  });


  //--- get time ----------------
  server.on("/getSystemTime", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    String inputupdatetime;
    if (request->hasParam("mySystemTime")) {
      inputupdatetime = request->getParam("mySystemTime")->value();
    }
    //    Serial.println("inputupdatetime: ");
    //    Serial.println(inputupdatetime);
    updatedatetime(inputupdatetime);
    request->send(200, "text/plain", "Cập nhật thời gian thành công");
  });

  //--- get wifi ------------
  server.on("/getWiFiInfo", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    String inputupdatewifi;
    if (request->hasParam("mySsid")) {
      inputupdatewifi = request->getParam("mySsid")->value();
    }
    //    Serial.println("inputupdatewifi: ");
    //    Serial.println(inputupdatewifi);
    updatewifi(inputupdatewifi);
    request->send(200, "text/plain", "Cập nhật wifi thành công");
  });
  //--- get zone---------------
  server.on("/getZoneInfo", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    String inputupdatezone;
    if (request->hasParam("Zone1")) {
      inputupdatezone = request->getParam("Zone1")->value();
    }
    //    Serial.println("inputupdatezone: ");
    //    Serial.println(inputupdatezone);
    updatezone(inputupdatezone);
    request->send(200, "text/plain", "Cập nhật zone thành công");
    ESP.restart();
  });

  //-------------RESET RELAY-----------------
  server.on("/resetRelay", HTTP_GET, [](AsyncWebServerRequest * request)
  { //http://192.168.4.1/resetRelay
    Serial.println("resetRelay");
    request->send(200, "text/plain", "reset thành công");
  });

  //------------------------------------
  //----------------STATUS SYSTEM---------------
  server.on("/ToggleSystem", HTTP_GET, [](AsyncWebServerRequest * request)
  { //http://192.168.4.1/ToggleSystem?isState={value}
    String inputupdatesystem;
    if (request->hasParam("isState")) {
      inputupdatesystem = request->getParam("isState")->value();
    }
    updatesystem(inputupdatesystem);
    request->send(200, "text/plain", "Cập nhật system thành công");
  });
  //----------------------------

  //---------ENABLE RELAY-------------
  server.on("/setStateRelay", HTTP_GET, [](AsyncWebServerRequest * request)
  { //http://192.168.4.1/setStateRelay?state={value}
    String inputrelay;
    if (request->hasParam("state")) {
      inputrelay = request->getParam("state")->value();
    }
    updaterelay(inputrelay);
    request->send(200, "text/plain", "Cập nhật relay thành công");
  });
  //----------------------------------

  //------------RSSI-----------------
  //http://192.168.4.1/calibrateRSSI
  server.on("/calibrateRSSI", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    Serial.println("getRSSI");
    request->send(200, "text/plain", "Cập nhật relay thành công");
  });
  //------------------------------------

  //------------UPDATE RSSI-----------------
  server.on("/setRSSI", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    String inputrssi;
    if (request->hasParam("1000")) {
      inputrssi = request->getParam("1000")->value();
    }
    updaterssi(inputrssi);
    request->send(200, "text/plain", "Cập nhật rssi thành công");
  });
  //----------------------------------------


  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
}
String processor(const String &var)
{
  //  Serial.println(var);
  if (var == "STATE")
  {
    return "AB:DC:76:5F:FE:39";
  }
  else if (var == "currentValueWifiSSID") {
    return readfromFlash("/SSIDWIFI.txt");
  }
  else if (var == "currentValueWifiPassword") {
    return readfromFlash("/PASSWORDWIFI.txt");
  }
  return String();
}
// ---- read data in file namefile-------------------
String readfromFlash(String namefile) {
  String dataFlash = "";
  File file = LittleFS.open(namefile, "r");
  if (!file) {
    //    Serial.println("Failed to open file for reading");
    //        return;
  }
  while (file.available()) {
    dataFlash += (char)file.read();
  }
  file.close();
  return dataFlash;
}
void writetoFlash(String namefile, String data, const char *mode) {
  File file = LittleFS.open(namefile, mode);
  if (file)
  {
    //    Serial.println("write to flash");
    file.print(data);
  }
  else
  {
    //    Serial.println("write to flash failed");
  }
  file.close();
}
//---------write data update wifi in flash-----------
void updatewifi(String data) { //Abc,myPassword=1234567890
  int8_t index;
  String processdata[2];
  index = data.indexOf(",myPassword=");
  processdata[0] = data.substring(0, index);
  processdata[1] = data.substring(index + 12, data.length());

  //  Serial.println("updatewifi");
  //  Serial.println(processdata[0]);
  //  Serial.println(processdata[1]);

  writetoFlash("/SSIDWIFI.txt", processdata[0], "w");
  writetoFlash("/PASSWORDWIFI.txt", processdata[1], "w");
}
//--------write data update zone in flash----------
void updatezone(String data) { //ZONEINFO,DZ1=1500,DZ2=2000,DZ3=2500,SZ1=1,SZ2=3,SZ3=2
  data = "ZONEINFO,DZ1=" + data;
  //  Serial.println("updatezone");
  writetoFlash("/ZONEINFO.txt", data, "w");
  Serial.println(data);
  
  
  
  
  
}
//--------write data time update rtc----------
void updatedatetime(String data) {
  Serial.println(data);
}
void updatesystem(String data) {
  writetoFlash("/SYSTEMSTATE.txt", data , "w+");
  Serial.println("isState:" + data);
}
void updaterelay(String data) {
  writetoFlash("/RELAYSTATE.txt", data , "w+");
  Serial.println("setStateRelay:" + data);
}
void updaterssi(String data) {
  writetoFlash("/VALUERSSI.txt","SETRSSI,1000=" + data , "w+");
  Serial.println("SETRSSI,1000=" + data);
}
