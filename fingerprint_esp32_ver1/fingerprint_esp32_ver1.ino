//

//sketch active
//

#include <Adafruit_Fingerprint.h>
#include <Keypad.h>
#include <SPIFFS.h>
#include <RTClib.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#define BUZZER 4
#define RELAY 13
#define mySerial Serial2

char command[15];
char key;
long timestart = 0;
long timeregisterfingerstart = 0;
String commandToString = "";
uint8_t id;
uint8_t checkExists = 0;
uint8_t statusRelay = 0;
String statusrelay = "";
String nameFingerprint = "unknown";
String namePasscode = "unknown";
uint8_t lenssid;
uint8_t lenpassword;


String ssid;
String password;
char arrssid[31];
char arrpassword[31];
const byte ROWS = 4; // four rows
const byte COLS = 3; // three columns
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {12, 27, 26, 15};
byte colPins[COLS] = {33, 32, 25};
char day_of_week[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
RTC_DS1307 rtc;
DateTime now;
AsyncWebServer server(80);

void setup()
{
  Serial.begin(115200);
  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY, OUTPUT);
  bip2();
  setupKeypad();
  delay(400);
  setupFingerprint();
  delay(400);
  setupSPIFFS();
  delay(400);
  setupTime();
  digitalWrite(RELAY, LOW);
  delay(400);
  setupWifi();
}

void loop()
{
  if (statusRelay == 0)
  {
    finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_BLUE, 400);
  }
  else if (statusRelay == 1)
  {
    finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_PURPLE, 400);
  }
  enterKeypad();
  getFingerprintID();

  if (commandToString.substring(0, 2) == "11")
  {
    Serial.println("register fingerprint"); //*11*xx#
    modeRegisterFinger(commandToString);
  }
  else if (commandToString.substring(0, 2) == "12") //*12*xxxx*aa#
  {
    Serial.println("delete fingerprint");
    modeDeleteFinger(commandToString);
  }
  else if (commandToString.substring(0, 2) == "22") //*22*999*xx*xxxx# //*xx* -> 7-9
  {
    Serial.println("register passcord");
    modeRegisterPasscode(commandToString);
  }
  else if (commandToString.substring(0, 2) == "33")
  {
    Serial.println("control relay with passcord");
    modeControlRelayPasscode(commandToString);
  }
  else if (commandToString.substring(0, 2) == "89")
  {
    Serial.println("reset factory");
    if (commandToString.substring(3, commandToString.length()) == "2021946")
    {
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 40, FINGERPRINT_LED_PURPLE, 15);
      resetFactory();
      if (statusRelay == 1)
      {
        closeRelay();
        statusRelay = 0;
        delay(140);
      }
      bip3();
    }
    else
    {
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 15);
      bipFail();
    }
  }
  else if (commandToString.substring(0, 2) == "55")
  { // 55*xx#
    modeDeletePasscode(commandToString);
  }
  else if (commandToString.substring(0, 2) == "19") { //reset password wifi 19*843*00#
    if (commandToString.substring(2, commandToString.length()) == "*843*00") {
      SPIFFS.remove("/SSIDWIFI.txt");
      delay(40);
      SPIFFS.remove("/PASSWORDWIFI.txt");
      delay(40);
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 40, FINGERPRINT_LED_PURPLE, 15);
      bip3();
      ESP.restart();
    }
    else {
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 15);
      bipFail();
    }
  }
  else if (commandToString.substring(0, 2) == "18") { //18*456*00#
    if (commandToString.substring(2, commandToString.length()) == "*456*00") {
      SPIFFS.remove("/EVENT.txt");
      delay(40);
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 40, FINGERPRINT_LED_PURPLE, 15);
      bip3();
    }
    else {
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 15);
      bipFail();
    }
  }
  else if (commandToString != "")
  {
    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 15);
    bipFail();

    Serial.print("SYS:");
    Serial.println(readfromFlash("/SYS.txt"));
    Serial.print("PASS:");
    Serial.println(readfromFlash("/PASS.txt"));
    Serial.print("DEL:");
    Serial.println(readfromFlash("/DEL.txt"));
    Serial.print("EVENT:");
    Serial.println(readfromFlash("/EVENT.txt"));
  }
}
void setupKeypad()
{
  Serial.println("###keypad###");
}
void setupFingerprint()
{
  finger.begin(57600);

  if (finger.verifyPassword())
  {
    Serial.println("Found fingerprint sensor!");
    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 40, FINGERPRINT_LED_BLUE, 15);
  }
  else
  {
    Serial.println("Did not find fingerprint sensor :(");
    while (1)
    {
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 10);
      delay(400);
    }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x"));
  Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x"));
  Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: "));
  Serial.println(finger.capacity);
  Serial.print(F("Security level: "));
  Serial.println(finger.security_level);
  Serial.print(F("Device address: "));
  Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: "));
  Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: "));
  Serial.println(finger.baud_rate);
}
void setupSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
  }
  else
  {
    Serial.println("SPIFFS successfully");
  }
}
void setupTime()
{
  if (rtc.begin())
  {
    Serial.println("DS1307 begin");
  }
  if (!rtc.isrunning())
  {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // rtc.adjust(DateTime(2022,11,11,9,18,35));
  }
  Serial.println(getTimeNow());
}
void bip() // buzzer for push keypad
{
  tone(BUZZER, 500, 300);
  delay(40);
  noTone(BUZZER);
}
void bip1()
{
  tone(BUZZER, 1000);
  delay(200);
  noTone(BUZZER);
  delay(200);
}
void bip2() // buzzer for mode register
{
  tone(BUZZER, 450);
  delay(200);
  noTone(BUZZER);
  delay(200);

  tone(BUZZER, 450);
  delay(200);
  noTone(BUZZER);
  delay(200);
}
void bip3() // buzzer succes
{
  tone(BUZZER, 450);
  delay(200);
  noTone(BUZZER);
  delay(200);

  tone(BUZZER, 450);
  delay(200);
  noTone(BUZZER);
  delay(200);

  tone(BUZZER, 450);
  delay(200);
  noTone(BUZZER);
  delay(200);
}
void bipFail() // buzzer fail
{
  tone(BUZZER, 250);
  delay(900);
  noTone(BUZZER);
  delay(500);
}

void enterKeypad() // input keypad
{
  char key;
  int j = 0;
  key = keypad.getKey();
  if (key == '*')
  {
    bip();
    timestart = millis();
    while (1)
    {
      char key1;
      key1 = keypad.getKey();
      if (key1)
      {
        bip();
        Serial.print(key1);
        if (key1 == '#')
        {
          break;
        }
        else
        {
          command[j++] = key1;
        }
      }
      if (millis() - timestart > 60000)
      {
        break;
      }
    }
  }
  delay(40);
  command[j] = '\0';

  commandToString = String(command);
}
uint8_t getFingerprintEnroll(String idtemp)
{ // register finger

  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #");
  Serial.println(id);
  while (p != FINGERPRINT_OK)
  {
    p = finger.getImage();
    switch (p)
    {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 40, FINGERPRINT_LED_BLUE, 15);
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p)
  {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER)
  {
    p = finger.getImage();
  }
  Serial.print("ID ");
  Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK)
  {
    p = finger.getImage();
    switch (p)
    {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p)
  {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");
  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK)
  {
    Serial.println("Prints matched!");
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Communication error");
    return p;
  }
  else if (p == FINGERPRINT_ENROLLMISMATCH)
  {
    Serial.println("Fingerprints did not match");
    return p;
  }
  else
  {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID ");
  Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK)
  {
    Serial.println("Stored!");
    bip3();
    writetoFlash("/SYS.txt", "REG FINGERPRINT,id=" + idtemp + ",name=" + nameFingerprint + "," + getTimeNow(), "a");
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Communication error");
    return p;
  }
  else if (p == FINGERPRINT_BADLOCATION)
  {
    Serial.println("Could not store in that location");
    return p;
  }
  else if (p == FINGERPRINT_FLASHERR)
  {
    Serial.println("Error writing to flash");
    return p;
  }
  else
  {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}
bool isNumber(String s) // check data is number or character
{
  int i;
  for (i = 0; i < s.length(); i++)
  {
    if (isdigit(s[i]) == false)
    {
      return false;
    }
  }
  return true;
}
uint8_t exisgetFingerprintID() // check finger is new or old
{
  uint8_t p = finger.getImage();
  switch (p)
  {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      // Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  p = finger.image2Tz();
  switch (p)
  {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK)
  {
    Serial.println("fingerprint exist");
    checkExists = 1;
    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 10);
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Communication error");
    return p;
  }
  else if (p == FINGERPRINT_NOTFOUND)
  {
    Serial.println("new finger");
    checkExists = 2;
    return p;
  }
  else
  {
    Serial.println("Unknown error");
    return p;
  }
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);
  return finger.fingerID;
}
void writetoFlash(String namefile, String data, const char *mode)
{ // write data in flash
  File file = SPIFFS.open(namefile, mode);
  delay(40);
  if (file)
  {
    Serial.println("write to flash");
    file.print(data);
  }
  else
  {
    Serial.println("write to flash failed");
  }
  delay(50);
  file.close();
}

String readfromFlash(String namefile)
{ // read data from flash
  String dataFlash = "";
  File file = SPIFFS.open(namefile, "r");
  if (!file)
  {
    Serial.println("Failed to open file for reading");
  }
  // Serial.println("File Content:");
  while (file.available())
  {
    // Serial.write(file.read());
    dataFlash += (char)file.read();
  }
  file.close();
  // Serial.println(dataFlash);
  return dataFlash;
}

String getTimeNow()
{ // get time now to write log
  String kq;
  now = rtc.now();
  kq = "time=" + String(now.day(), DEC) + "/" + String(now.month(), DEC) + "/" + String(now.year(), DEC) + " " + String(now.hour(), DEC) + ":" + String(now.minute(), DEC) + ":" + String(now.second(), DEC) + "|";
  return kq;
}

uint8_t getFingerprintID()
{ // scan fingerprint , finger true relay open, finger false led red
  uint8_t p = finger.getImage();
  switch (p)
  {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      // Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p)
  {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK)
  {
    Serial.println("Found a print match!");
    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_PURPLE, 10);
    delay(500);
    bip1();
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Communication error");
    return p;
  }
  else if (p == FINGERPRINT_NOTFOUND)
  {
    Serial.println("Did not find a match");
    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 10);
    delay(1000);
    bipFail();
    return p;
  }
  else
  {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);
  if (statusRelay == 0)
  {
    Serial.println(getTimeNow());
    Serial.println("OPEN RELAY");
    openRelay();
    writetoFlash("/EVENT.txt", "FINGERPRINT,id=" + String(finger.fingerID) + ",state=On," + getTimeNow(), "a");
    delay(50);
    statusRelay = 1;
  }
  else if (statusRelay == 1)
  {
    Serial.println(getTimeNow());
    Serial.println("CLOSE RELAY");
    closeRelay();
    writetoFlash("/EVENT.txt", "FINGERPRINT,id=" + String(finger.fingerID) + ",state=Off," + getTimeNow(), "a");
    delay(50);
    statusRelay = 0;
  }
  return finger.fingerID;
}
void openRelay()
{ // open relay
  digitalWrite(RELAY, HIGH);
}
void closeRelay()
{ // close relay
  digitalWrite(RELAY, LOW);
}
int checkPassCode(String subs, String s)
{ // check sub string in string
  uint8_t len_subs = subs.length();
  long len_s = s.length();
  for (int i = 0; i < len_s - len_subs; i++)
  {
    int j;
    for (j = 0; j < len_subs; j++)
    {
      if (s[i + j] != subs[j])
        break;
    }
    if (j == len_subs)
      return i;
  }
  return -1;
}
void resetFactory()
{
  finger.emptyDatabase();
  delay(400);
  SPIFFS.remove("/EVENT.txt");
  delay(40);
  SPIFFS.remove("/SYS.txt");
  delay(40);
  SPIFFS.remove("/PASS.txt");
  delay(40);
  SPIFFS.remove("/DEL.txt");
  delay(40);
  ESP.restart();
}

uint8_t deleteFingerprint(uint8_t id, String preid, String name)
{
  uint8_t p = -1;
  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK)
  {
    Serial.println("Deleted!");
    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_PURPLE, 10);
    bip3();
    delay(140);
    writetoFlash("/DEL.txt", "DEL FINGERPRINT,id=" + preid + ",name=" + name + "," + getTimeNow(), "a");
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Communication error");
    bipFail();
  }
  else if (p == FINGERPRINT_BADLOCATION)
  {
    Serial.println("Could not delete in that location");
    bipFail();
  }
  else if (p == FINGERPRINT_FLASHERR)
  {
    Serial.println("Error writing to flash");
    bipFail();
  }
  else
  {
    Serial.print("Unknown error: 0x");
    Serial.println(p, HEX);
    bipFail();
  }

  return p;
}

//---------wifi--------------------------------
void setupWifi()
{

  WiFi.mode(WIFI_MODE_AP);
  delay(400);
  if (readfromFlash("/SSIDWIFI.txt") == "") {
    writetoFlash("/SSIDWIFI.txt", "INS-" + WiFi.softAPmacAddress(), "w+");
  }
  if (readfromFlash("/PASSWORDWIFI.txt") == "") {
    writetoFlash("/PASSWORDWIFI.txt", "InsB202C", "w+");
  }

  ssid = readfromFlash("/SSIDWIFI.txt");
  password = readfromFlash("/PASSWORDWIFI.txt");
  delay(40);

  WiFi.softAP(ssid.c_str(), password.c_str());

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(IP);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/style.css", "text/css");
  });

  server.on("/src/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/src/bootstrap.min.js", "text/javascrip");
  });
  server.on("/src/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/src/bootstrap.min.css", "text/css");
  });
  server.on("/src/jquery-3.3.1.min.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/src/jquery-3.3.1.min.js", "text/javascrip");
  });

  server.on("/src/moment.min.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/src/moment.min.js", "text/javascrip");
  });
  server.on("/src/datetimepicker.min.css", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/src/datetimepicker.min.css", "text/css");
  });
  server.on("/src/datetimepicker.min.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/src/datetimepicker.min.js", "text/javascrip");
  });


  server.on("/fonts/glyphicons.ttf", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/fonts/glyphicons.ttf", "text/plain");
  });
  server.on("/fonts/glyphicons.woff", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/fonts/glyphicons.woff", "text/plain");
  });
  server.on("/fonts/glyphicons.woff2", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/fonts/glyphicons.woff2", "text/plain");
  });

  server.on("/src/table2excels.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/src/table2excels.js", "text/javascrip");
  });
  server.on("/src/xlsxmin.min.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/src/xlsxmin.min.js", "text/javascrip");
  });




  server.on("/EVENT.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/EVENT.txt", "text/plain");
  });
  server.on("/SYS.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/SYS.txt", "text/plain");
  });
  server.on("/DEL.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/DEL.txt", "text/plain");
  });
  server.on("/PASS.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/PASS.txt", "text/plain");
  });


  // Route to set GPIO to HIGH
  server.on("/view-activities.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/view-activities.html", String(), false, processor);
  });

  // Route to set GPIO to LOW
  server.on("/view-fingerprint.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/view-fingerprint.html", String(), false, processor);
  });
  server.on("/view-passcode.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/view-passcode.html", String(), false, processor);
  });
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    int index_get = request->params();
    for (int i = 0; i < index_get; i++)
    {
      AsyncWebParameter *p = request->getParam(i);
      Serial.print("Param name: ");
      Serial.println(p->name());
      Serial.print("Param value: ");
      Serial.println(p->value());
      Serial.println("------");
      inputwebserver(p->name(), p->value());
    }
    request->send(200, "text/plain", "Cập nhật thành công");
  });

  server.on("/getSystemTime", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    String inputupdatetime;
    if (request->hasParam("mySystemTime")) {
      inputupdatetime = request->getParam("mySystemTime")->value();
    }
    Serial.println("inputupdatetime: ");
    Serial.println(inputupdatetime);
    updatedatetime(inputupdatetime);
    request->send(200, "text/plain", "Cập nhật thời gian thành công");
  });

  server.on("/getWiFiInfo", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    String inputupdatewifi;
    if (request->hasParam("mySsid")) {
      inputupdatewifi = request->getParam("mySsid")->value();
    }
    Serial.println("inputupdatewifi: ");
    Serial.println(inputupdatewifi);
    request->send(200, "text/plain", "Cập nhật thành công");
    updatewifi(inputupdatewifi);
    ESP.restart();
  });


  AsyncElegantOTA.begin(&server);
  server.begin();
}
String processor(const String &var)
{
  Serial.println(var);
  if (var == "STATE")
  {
    return WiFi.softAPmacAddress();
  }
  else if (var == "currentValueWifiSSID") {
    return readfromFlash("/SSIDWIFI.txt");
  }
  else if (var == "currentValueWifiPassword") {
    return readfromFlash("/PASSWORDWIFI.txt");
  }
  return String();
}
// replace data in string
void modeDeletePasscode(String data) // 55*xx  pass.txt -> 1111,01|1234,05|
{
  //  delete passcode when input keypad id
  String deleteid = "";
  String passdata = "", sysdata = "";
  String getnamedelete = "";
  int32_t splitsys[4];
  int16_t indexdeleteid;
  deleteid = data.substring(data.indexOf("*") + 1, data.length()); // get id in input keypad

  Serial.println("*************");
  Serial.print("deleteid:");
  Serial.println(deleteid); //-------

  if (isNumber(deleteid) && (deleteid.length() == 2))
  {
    passdata = readfromFlash("/PASS.txt");
    indexdeleteid = passdata.indexOf("," + deleteid + "|");
    delay(140);
    if (indexdeleteid >= 0)
    { // check ,xx| in pass.txt
      sysdata = readfromFlash("/SYS.txt");
      delay(140);
      splitsys[0] = sysdata.indexOf("REG PASSCODE,id=" + deleteid); // REG PASSCODE,id=14,name=no_name,time=25/11/2022 09:10:10|
      splitsys[1] = sysdata.indexOf("name", splitsys[0]) + 5;       // =                       =      =                         =
      splitsys[2] = sysdata.indexOf(",time", splitsys[1]);
      splitsys[3] = sysdata.indexOf("|", splitsys[2] + 1) + 1;
      getnamedelete = sysdata.substring(splitsys[1], splitsys[2]);

      Serial.print("getnamedelete:");
      Serial.println(getnamedelete);                               //------
      Serial.println(sysdata.substring(splitsys[0], splitsys[3])); //-----

      sysdata.replace(sysdata.substring(splitsys[0], splitsys[3]), "");
      writetoFlash("/SYS.txt", sysdata, "w+"); // remove id passcode

      writetoFlash("/DEL.txt", "DEL PASSCODE,id=" + deleteid + ",name=" + getnamedelete + "," + getTimeNow(), "a");
      Serial.println(passdata.substring(indexdeleteid - 4, indexdeleteid + 4));
      passdata.replace(passdata.substring(indexdeleteid - 4, indexdeleteid + 4), "");
      writetoFlash("/PASS.txt", passdata, "w+"); // remove id passcode
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_PURPLE, 10);
      bip3();
    }
    else
    {
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 15);
      bipFail();
    }
  }
  else
  {
    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 15);
    bipFail();
  }
}
void inputwebserver(String oldword, String newword)
{ // FINGERPRINT01
  //  01234567890123456789
  String datasys = readfromFlash("/SYS.txt");
  String checkheader = "";
  checkheader = oldword.substring(0, oldword.length() - 2) + ",id=" + oldword.substring(oldword.length() - 2, oldword.length());

  int32_t indexoldeword = datasys.indexOf(checkheader);
  int32_t indexnamestart = datasys.indexOf(",", indexoldeword) + 7;
  int32_t indexnameend = datasys.indexOf(",", indexnamestart);
  Serial.println(checkheader);

  if (oldword.indexOf("FINGERPRINT") >= 0)
  {
    Serial.println(checkheader);
    Serial.println(datasys.substring(indexoldeword, indexnameend));
    datasys.replace(datasys.substring(indexoldeword, indexnameend), checkheader + ",name=" + newword);
    writetoFlash("/SYS.txt", datasys, "w+");
  }
  else if (oldword.indexOf("PASSCODE") >= 0)
  {
    Serial.println(checkheader);
    datasys.replace(datasys.substring(indexoldeword, indexnameend), checkheader + ",name=" + newword);
    writetoFlash("/SYS.txt", datasys, "w+");
  }
  // replace id=1,name=unknown
}

void modeRegisterFinger(String data) // 11*xx#  -> 0 1 2 3 4 5
{ /* 1 1 * x x #  */
  String idtemp = data.substring(data.indexOf("*") + 1, data.length());
  if (isNumber(idtemp) && (idtemp.length() == 2))
  { // check is number or charater
    Serial.println(idtemp);
    id = idtemp.toInt(); // convert input keypad to int
    if (finger.loadModel(id) == 0)
    { // check id exist
      Serial.println("id exists");
      bipFail(); // id exist
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 10);
    }
    else
    {
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_BLUE, 1000);
      bip2();
      timeregisterfingerstart = millis();
      while (1) // id new
      {
        // finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_BLUE, 10);
        exisgetFingerprintID(); // assign variable when detect finger exist
        if (checkExists == 1)
        { // finger exist
          Serial.println("FINGER EXIST!!!");
          checkExists = 0;
          finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 15);
          bipFail();
          break;
        }
        else if (checkExists == 2)
        { // finger new
          Serial.println("FINGER NEW");
          finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 40, FINGERPRINT_LED_BLUE, 15);
          getFingerprintEnroll(idtemp);
          checkExists = 0;
          break;
        }
        if ((millis() - timeregisterfingerstart) > 60000)
          break;
      }
    }
  }
  else
  {
    Serial.println("after *11* no number");
    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 15);
    bipFail(); // id exist
  }
}
void modeDeleteFinger(String data) // 12*xxxx*aa#
{
  int8_t splitdata[2];
  int32_t splitsys[4];
  String prepasscode = "";
  String preidfinger = "";
  String sysdata = "";
  String passdata = "";
  String getnamedelete = "";

  splitdata[0] = data.indexOf("*");
  splitdata[1] = data.indexOf("*", splitdata[0] + 1);

  prepasscode = data.substring(splitdata[0] + 1, splitdata[1]);  // xxxx
  preidfinger = data.substring(splitdata[1] + 1, data.length()); // aa

  passdata = readfromFlash("/PASS.txt");
  if ((passdata.indexOf(prepasscode) >= 0) &&
      (splitdata[0] == 2) &&
      (splitdata[1] == 7) &&
      (preidfinger.length() == 2) &&
      isNumber(preidfinger))
  {
    sysdata = readfromFlash("/SYS.txt");                                // REG PASSCODE,id=14,name=no_name,time=25/11/2022 09:10:10|
    splitsys[0] = sysdata.indexOf("REG FINGERPRINT,id=" + preidfinger); // REG
    if (splitsys[0] >= 0)
    {
      splitsys[1] = sysdata.indexOf("name", splitsys[0]);
      splitsys[2] = sysdata.indexOf(",time", splitsys[1]);
      splitsys[3] = sysdata.indexOf("|", splitsys[1]) + 1;

      getnamedelete = sysdata.substring(splitsys[1] + 5, splitsys[2]);
      Serial.println("start delete finger");
      id = preidfinger.toInt();
      deleteFingerprint(id, preidfinger, getnamedelete);
      sysdata.replace(sysdata.substring(splitsys[0], splitsys[3]), "");
      writetoFlash("/SYS.txt", sysdata, "w+");
    }
    else
    {
      Serial.println("exis mode delete fingerprint");
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 15);
      bipFail();
    }
  }
  else
  {
    Serial.println("exis mode delete fingerprint");
    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 15);
    bipFail();
  }

  // int32_t indexstart = 0;
  // int32_t indexend = 0;
  // String prenamefingerprint = "";
  // int8_t preindex1 = data.indexOf("*") + 1;                          // 3
  // int8_t preindex2 = data.indexOf("*", preindex1);                   // 7
  // String prepasscode = data.substring(preindex1, preindex2);         // xxxx passcode
  // String preidfinger = data.substring(preindex2 + 1, data.length()); // aa id finger

  // String datasys = readfromFlash("/SYS.txt");
  // indexstart = datasys.indexOf("REG FINGERPRINT,id=" + preidfinger);

  // indexend = datasys.indexOf("|", indexstart);
  // prenamefingerprint = datasys.substring(indexstart + 27, datasys.indexOf(",", indexstart + 27));
  // String datareplace = datasys.substring(indexstart, indexend + 1);

  // if ((readfromFlash("/PASS.txt").indexOf(prepasscode) >= 0) && (preindex1 == 3) && (preindex2 == 7) && (indexstart != -1))
  // {
  //   Serial.println("start delete finger");
  //   id = preidfinger.toInt();
  //   deleteFingerprint(id, preidfinger, prenamefingerprint);
  //   Serial.println(datareplace);
  //   datasys.replace(datareplace, "");
  //   writetoFlash("/SYS.txt", datasys, "w+");
  // }
  // else
  // {
  //   Serial.println("exis mode delete fingerprint");
  //   bipFail();
  //   finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 10);
  // }
}
void modeRegisterPasscode(String data) // 22*999*aa*bbbb mode22
{ // 012345678901234567890
  String splitdata[3];
  int8_t splitspecial[3];
  String passdata = "";
  String headercheck = "";

  splitspecial[0] = data.indexOf("*");
  splitspecial[1] = data.indexOf("*", splitspecial[0] + 1);
  splitspecial[2] = data.indexOf("*", splitspecial[1] + 1);

  splitdata[0] = data.substring(splitspecial[0] + 1, splitspecial[1]); // 999
  splitdata[1] = data.substring(splitspecial[1] + 1, splitspecial[2]); // aa
  splitdata[2] = data.substring(splitspecial[2] + 1, data.length());   // bbbb
  //  Serial.println("-----------------");
  //  Serial.print("data:");  Serial.println(data);
  //  Serial.print("splitdata[0]:");  Serial.println(splitdata[0]);
  //  Serial.print("splitdata[1]:");  Serial.println(splitdata[1]);
  //  Serial.print("splitdata[2]:");  Serial.println(splitdata[2]);
  //  Serial.println("---------------");
  if ((splitdata[0] == "999") &&
      (splitspecial[0] == 2) &&
      (splitspecial[1] == 6) &&
      (splitspecial[2] == 9) &&
      (splitdata[1].length() == 2) &&
      (splitdata[2].length() == 4))
  {
    passdata = readfromFlash("/PASS.txt"); // bbbb,aa|
    headercheck = splitdata[2] + "," + splitdata[1] + "|";
    Serial.print("headercheck:");
    Serial.println(headercheck);
    Serial.println(passdata.indexOf(headercheck));
    if ((passdata.indexOf(splitdata[2]) == -1) && (passdata.indexOf("," + splitdata[1] + "|") == -1))
    {
      writetoFlash("/SYS.txt", "REG PASSCODE,id=" + splitdata[1] + ",name=" + namePasscode + "," + getTimeNow(), "a");
      delay(50);
      writetoFlash("/PASS.txt", headercheck, "a");
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_PURPLE, 10);
      bip3();
    }
    else
    {
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 15);
      Serial.println("*22* no number");
      bipFail();
    }
  }
  else
  {
    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 15);
    bipFail();
  }
}
void modeControlRelayPasscode(String data) // 33*888*bbbb#
{ // 01234567890123456789
  int32_t indexpasscode;
  String predataflash = "";
  String idPasscode = "";
  String precoverpasscode = "";
  predataflash = readfromFlash("/PASS.txt");
  precoverpasscode = data.substring(7, data.length());
  indexpasscode = predataflash.indexOf(data.substring(7, data.length()));
  if ((data.substring(3, 6) == "888") && (indexpasscode >= 0) && (precoverpasscode.length() == 4))
  {

    idPasscode = predataflash.substring(indexpasscode + 5, indexpasscode + 7);
    Serial.println("============");
    Serial.println(indexpasscode);
    Serial.println(idPasscode);

    if (statusRelay == 0)
    {
      openRelay();
      statusRelay = 1;
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_PURPLE, 10);
      bip1();
      writetoFlash("/EVENT.txt", "PASSCODE,id=" + idPasscode + ",state=On," + getTimeNow(), "a");
    }
    else if (statusRelay == 1)
    {
      closeRelay();
      statusRelay = 0;
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_PURPLE, 10);
      bip1();
      writetoFlash("/EVENT.txt", "PASSCODE,id=" + idPasscode + ",state=Off," + getTimeNow(), "a");
    }
  }
  else
  {
    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 15);
    bipFail();
  }
}
void updatewifi(String data) {
  //abc,myPassword=abc1
  int8_t indexheader;
  String pressid = "";
  String prepassword = "";
  indexheader =  data.indexOf(",myPassword=");
  pressid = data.substring(0, indexheader);
  prepassword = data.substring(indexheader + 12, data.length());

  writetoFlash("/SSIDWIFI.txt", pressid, "w");
  writetoFlash("/PASSWORDWIFI.txt", prepassword, "w");
  Serial.print("pressid: ");
  Serial.print(pressid);
  Serial.print("prepassword: ");
  Serial.print(prepassword);
}
void updatedatetime(String data) {//http://192.168.4.1/getSystemTime?mySystemTime=13/01/2023 08:58:26
  int8_t indexspecial1 = data.indexOf("/");
  int8_t indexspecial2 = data.indexOf("/", indexspecial1 + 1);
  int8_t indexspecial3 = data.indexOf(" ");
  int8_t indexspecial4 = data.indexOf(":");
  int8_t indexspecial5 = data.indexOf(":", indexspecial4 + 1);

  int8_t preday = data.substring(0, indexspecial1).toInt();
  int8_t premonth = data.substring(indexspecial1+1, indexspecial2).toInt();
  int16_t preyear = data.substring(indexspecial2+1, indexspecial3).toInt();

  int8_t prehour = data.substring(indexspecial3 + 1, indexspecial4).toInt();
  int8_t preminute = data.substring(indexspecial4 + 1, indexspecial5).toInt();
  int8_t presecond = data.substring(indexspecial5 + 1, indexspecial5 + 3).toInt() + 2;

  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  rtc.adjust(DateTime(preyear, premonth, preday, prehour, preminute, presecond));
  Serial.println(preyear); Serial.println(premonth); Serial.println(preday);
  Serial.println(prehour); Serial.println(preminute); Serial.println(presecond);

}
