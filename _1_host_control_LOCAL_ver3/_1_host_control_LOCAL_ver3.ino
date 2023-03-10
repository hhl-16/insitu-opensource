/*
  proximity send tcpserver
  host for stm32f4
  esp connect uart2
  DWM1000 <-> SP1

  HID=AB:DC:76:5F:FE:39,24/12/22,14:30:15,45%,1,CA:14:CF:5A:45:4A
  HID=AB:DC:76:5F:FE:39,T=24/12/22,14:30:15,B=45%,Z=1,TID=CA:14:CF:5A:45:4A|
*/

#include "WiFiEsp.h"
#include "SoftwareSerial.h"
#include <SPI.h>
#include <DW1000.h>
#include "WiFiEspUdp.h"
#include <NTPClient.h>
#include "STM32FreeRTOS.h"
#include "queue.h"
#include <FlashStorage_STM32.h>
#include <STM32RTC.h>

STM32RTC& rtc = STM32RTC::getInstance();
QueueHandle_t xQueue;


HardwareSerial Serial2(USART2);
#define mySerial Serial2

//----PIN DWM1000-----
const uint8_t PIN_RST = PC0; // reset pin
const uint8_t PIN_IRQ = PC1; // irq pin
const uint8_t PIN_SS = PA4; // spi select pin
//--------
//---PIN OUTPUT ----
const uint8_t PIN_LED = PA7 ;
const uint8_t PIN_SIREN = PA6;
//const uint8_t PIN_RELAY = PE8;
const uint8_t PIN_RELAY = PE14;
//--------

//board fix


String ssid = "HOST LOCAL";
String password = "123456789";

// DEBUG packet sent status and count
volatile boolean received = false;
volatile boolean error = false;
volatile int16_t numReceived = 0; // todo check int type
String message;

String ID_tag = "";
String zone = "";
String time = "";
String dataUWB = "";
String a = "";

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;
String header;
//typedef struct {
//  String data;
//  String ip;
//  String ssid;
//  String password;
//} inputconfig;

String receiRX = "";

String HOSTMAC = "HID=AB:DC:76:5F:FE:39";
int number[6];

unsigned long posROM;

String datarom = "";

bool statusAP = false;


void printWifiStatus();
void reconnect();
void sendtcpserver(String);

void setupDWM1000();
void handleReceived();
void handleError();
void receiver();

//inputconfig configwifi;

String datademorom;

//rssi zone
//int m1 = -81;
//int m1_2 = -87;
//int m2 = -91;
//int m2_2 = -94;
//int m3 = -97;
//int m3_2 = -100;
//int m4 = -106;
//                    0     1     2   3    4    5    6     7     8      9   10    11    12     13    14    15
//int16_t mzone[16] = { -35, -81, -87, -91, -94, -97, -100, -106, -109, -112, -115, -120, -130, -140, -145, -150};
//                       0    1    1.5   2  2.5   3   3.5     4    4.5    5    5.5    6    6.5   7     7.5   8
float mzone[16];

float rssi_zone1[2];
float rssi_zone2[2];
float rssi_zone3[2];

int numberpre1;
int numberpre2;
int z = 0;
uint8_t setnumber = 0;
bool stateRelay = false;
bool getvaluerssi = false;


void setup()
{
  Serial.begin(115200);
  //RTC--------------------
  rtc.setClockSource(STM32RTC::LSE_CLOCK);
  rtc.begin();
  Serial.println(printRTC());
  //declare pin output---------
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_SIREN, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  digitalWrite(PIN_SIREN, LOW);
  digitalWrite(PIN_RELAY, HIGH);
  mzone[0] = -35;
  //  readStringFromEEPROM(0, &datarom);
  //  Serial.print("datarom:");
  //  Serial.println(datarom);
  //  configzone(datarom);
  //  Serial.print("rssi_zone1:");
  //  Serial.print(rssi_zone1[0]);
  //  Serial.println(rssi_zone1[1]);
  //
  //  Serial.print("rssi_zone2:");
  //  Serial.print(rssi_zone2[0]);
  //  Serial.println(rssi_zone2[1]);
  //
  //  Serial.print("rssi_zone3:");
  //  Serial.print(rssi_zone3[0]);
  //  Serial.println(rssi_zone3[1]);

  // ZONEINFO,DZ1=1500,DZ2=2000,DZ3=2500,SZ1=1,SZ2=3,SZ3=2
  //decalare esp07
  mySerial.begin(115200);
  //decalare module dwm1000
  setupDWM1000();
  //  NVIC_SystemReset();
  //create task for RTOS STM32DUINO
  xQueue = xQueueCreate(5, sizeof(String));
  //      xTaskCreate(vReceiveUART, "uart", 128, NULL, 2, NULL);

  if (xQueue != NULL) {
    xTaskCreate(vReceiveUWB, "uwb", 1024, NULL, 3, NULL);
    //    xTaskCreate(vReceiveUART, "uart", 128, NULL, 3, NULL);
    xTaskCreate(vSendDATA, "esp", 1024, NULL, 2, NULL);
    //    xTaskCreate(vControl, "control", 128, NULL, 2, NULL);

    vTaskStartScheduler();
  }
  else {
    Serial.println("the queue could not be created");
  }




}
void loop()
{

}
//--- TASK NH???N DATA OF MODULE DWM1000-----
void vReceiveUWB(void * pvParameters) {
  String a = "";
  int i = 0;
  int numberpre1;
  String a1 = "12345";
//  HID=AB:DC:76:5F:FE:39,T=30/1/23,15:24:53,B=45%,Z=1,TID=11:DC:76:5F:FE:39 
  for(int k=0;k<=4;k++){
    a = "HID=AB:DC:76:5F:FE:39,T=00/1/23,15:24:53,B=00%,Z=0,TID=00:00:00:00:00:00" ;
    xQueueSend(xQueue, &a, portMAX_DELAY);
  }

  for (;;) {
    if (received) {
      // get data as string
      //HID=AB:DC:76:5F:FE:39,24/12/22,14:30:15,45%,1,CA:14:CF:5A:45:4A
      DW1000.getData(message);
      if (getvaluerssi == true) {
        mySerial.print("RSSI VALUE:" + String(DW1000.getReceivePower()));
        Serial.println("RSSI VALUE:" + String(DW1000.getReceivePower()));
        getvaluerssi = false;
      }
      // a = HOSTMAC + ",T=" + printRTC() + "," + "B=45%,Z=" + DW1000.getReceivePower() + ",TID=" + message;
      //--------zone 1---------
      if (DW1000.getReceivePower() < rssi_zone1[0] && DW1000.getReceivePower() > rssi_zone1[1]) {
        a = HOSTMAC + ",T=" + printRTC() + "," + "B=45%,Z=1" + ",TID=" + message;
        xQueueSend(xQueue, &a, portMAX_DELAY);
      }
      else if (DW1000.getReceivePower() < rssi_zone2[0] && DW1000.getReceivePower() > rssi_zone2[1]) {
        a = HOSTMAC + ",T=" + printRTC() + "," + "B=45%,Z=2" + ",TID=" + message;
        xQueueSend(xQueue, &a, portMAX_DELAY);
      }
      else if (DW1000.getReceivePower() < rssi_zone3[0] && DW1000.getReceivePower() > rssi_zone3[1]) {
        a = HOSTMAC + ",T=" + printRTC() + "," + "B=45%,Z=3" + ",TID=" + message;
        xQueueSend(xQueue, &a, portMAX_DELAY);
      }
      Serial.print("A:");
      Serial.println(a);
      // xQueueSend(xQueue, &a, portMAX_DELAY);

      i++;
      //a = "Ps,MID=1,HID=AB:DC:76:5F:FE:39,Z=" + ID_tag +  ",BS=45,"  + "TID=" + message;
      received = false;
      //      xQueueSend(xQueue, &a, portMAX_DELAY);
    }

    //---------
    if (mySerial.available()) {
      receiRX = mySerial.readStringUntil('\n');
    }
    if (receiRX != "") {
      Serial.print("receiRX:");
      Serial.println(receiRX);
      if (receiRX.indexOf("ZONEINFO") >= 0) { //ZONEINFO,DZ1=3000,DZ2=1500,DZ3=1500,SZ1=1,SZ2=2,SZ3=3
        updatezone(receiRX);
        NVIC_SystemReset();
      }
      else if (receiRX.indexOf("/") >= 0) {
        updatedatetime(receiRX);
      }
      //setStateRelay:ON
      //setStateRelay:OFF
      else if (receiRX.indexOf("setStateRelay:ON") >= 0) {
        Serial.println("12345");
        stateRelay = true;
      }
      else if (receiRX.indexOf("setStateRelay:OFF") >= 0) {
        Serial.println("12345---");
        digitalWrite(PIN_RELAY, HIGH);
        stateRelay = false;
      }
      //getRSSI
      else if (receiRX.indexOf("getRSSI") >= 0) {
        getvaluerssi = true;
      }
      //SETRSSI,1000=-5,1500=-123,2000=-250,2500=-4,3000=5,3500=120,4000=140,4500=1,5000=5,5500=5,6000=15,6500=19,7000=15,7500=0,8000=15
      else if (receiRX.indexOf("SET") >= 0) {
        Serial.println("-------------");
        updatedaterssi(receiRX);
      }
      else if (receiRX.indexOf("isState:IDLE") >= 0) { //isState:IDLE
        while (1) {
          if (mySerial.available()) {
            receiRX = mySerial.readStringUntil('\n');
          }
          if (receiRX.indexOf("isState:ACTIVATE") >= 0) {
            break;
          }
          receiRX = "";
        }
      }
      else if (receiRX.indexOf("resetRelay") >= 0) {
        NVIC_SystemReset();

      }
    }
    receiRX = "";

    //----------
  }
  //HID=AB:DC:76:5F:FE:39,24/12/22,14:30:15,45%,1,CA:14:CF:5A:45:4A";
  if (error) {
    Serial.println("Error receiving a message");
    error = false;
    DW1000.getData(message);
    Serial.print("Error data is ... "); Serial.println(message);
  }
  taskYIELD();
}

//---TASK SEND DATA T???I ESP07 VIA USART
void vSendDATA(void * pvParameters) {
  String data;
  String dataknow;

  for (;;) {
    if (xQueueReceive(xQueue, &data, portMAX_DELAY) == pdPASS) {
      pingESP(data);
      if (data.indexOf("Z=1") >= 0) {
        setnumber = 1;
        Serial.print("sernumber:");
        Serial.println(setnumber);
      }
      if (data.indexOf("Z=2") >= 0) {
        setnumber = 2;
        Serial.print("sernumber:");
        Serial.println(setnumber);
      }
      if (data.indexOf("Z=3") >= 0) {
        setnumber = 3;
        Serial.print("sernumber:");
        Serial.println(setnumber);
      }
    }
    if (setnumber == 1) {
      if (stateRelay == true) {
        digitalWrite(PIN_RELAY, LOW);
      }
      chooose_alarm(number[3]);
    }
    else if (setnumber == 2) {
      digitalWrite(PIN_RELAY, HIGH);
      chooose_alarm(number[4]);
    }
    else if (setnumber == 3) {
      digitalWrite(PIN_RELAY, HIGH);
      chooose_alarm(number[5]);
    }
    setnumber = 0;
  }
}



//---SETUP MODULE DWM1000---
void setupDWM1000() {
  Serial.println("START MODULE DWM1000...");
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_SS);
  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(1);
  DW1000.setNetworkId(10);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();
  Serial.println(F("Committed configuration ..."));
  // DEBUG chip info and registers pretty printed
  char msg[128];
  DW1000.getPrintableDeviceIdentifier(msg);
  Serial.print("Device ID: "); Serial.println(msg);
  DW1000.getPrintableExtendedUniqueIdentifier(msg);
  Serial.print("Unique ID: "); Serial.println(msg);
  DW1000.getPrintableNetworkIdAndShortAddress(msg);
  Serial.print("Network ID & Device Address: "); Serial.println(msg);
  DW1000.getPrintableDeviceMode(msg);
  Serial.print("Device mode: "); Serial.println(msg);
  // attach callback for (successfully) received messages
  DW1000.attachReceivedHandler(handleReceived);
  DW1000.attachReceiveFailedHandler(handleError);
  DW1000.attachErrorHandler(handleError);
  // start reception
  receiver();
}

void handleReceived() {
  // status change on reception success
  received = true;
}

void handleError() {
  error = true;
}
void receiver() {
  DW1000.newReceive();
  DW1000.setDefaults();
  DW1000.receivePermanently(true);
  DW1000.startReceive();
}

//---GET TIME SYSTEM
String printRTC() {
  return String(rtc.getDay()) + "/" + String(rtc.getMonth()) + "/" + String(rtc.getYear()) + "," + \
         String(rtc.getHours()) + ":" + String(rtc.getMinutes()) + ":" + String(rtc.getSeconds()) ;
}
//---SEND DATA VIA USART
void pingESP(String data) {
  //  data += "|";
  mySerial.print("|" + data);
  Serial.print("DATA SEND ESP07: ");
  Serial.println(data);
}
//gettime from esp
void updatedatetime(String datatime) {//30/12/2022 11:59:55
  int8_t sp1 = datatime.indexOf("/");
  int8_t sp2 = datatime.indexOf("/", sp1 + 1);
  int8_t sp3 = datatime.indexOf(":");
  int8_t sp4 = datatime.indexOf(":", sp3 + 1);
  int8_t sp5 = datatime.indexOf(" ");

  int8_t day = datatime.substring(0, sp1).toInt();
  int8_t month = datatime.substring(sp1 + 1, sp2).toInt();
  int16_t year = datatime.substring(sp2 + 3, sp5).toInt();

  int8_t hour = datatime.substring(sp5 + 1, sp3).toInt();
  int8_t minute = datatime.substring(sp3 + 1, sp4).toInt();
  int8_t second = datatime.substring(sp4 + 1, datatime.length()).toInt();

  rtc.setTime(hour, minute, second);
  rtc.setDate(2, day, month, year);

  //  Serial.println(day); Serial.println(month); Serial.println(year);
  //  Serial.println(hour); Serial.println(minute); Serial.println(second);
}

void updatezone(String datazone) {
  writeStringToEEPROM(0, datazone);
}
void updatedaterssi(String datarssi) {
  mzone[1] = datarssi.substring(datarssi.indexOf("1000=") + 5, datarssi.indexOf(",1500")).toFloat();
  mzone[2] = datarssi.substring(datarssi.indexOf("1500=") + 5, datarssi.indexOf(",2000")).toFloat();
  mzone[3] = datarssi.substring(datarssi.indexOf("2000=") + 5, datarssi.indexOf(",2500")).toFloat();
  mzone[4] = datarssi.substring(datarssi.indexOf("2500=") + 5, datarssi.indexOf(",3000")).toFloat();
  mzone[5] = datarssi.substring(datarssi.indexOf("3000=") + 5, datarssi.indexOf(",3500")).toFloat();

  mzone[6] = datarssi.substring(datarssi.indexOf("3500=") + 5, datarssi.indexOf(",4000")).toFloat();
  mzone[7] = datarssi.substring(datarssi.indexOf("4000=") + 5, datarssi.indexOf(",4500")).toFloat();
  mzone[8] = datarssi.substring(datarssi.indexOf("4500=") + 5, datarssi.indexOf(",5000")).toFloat();
  mzone[9] = datarssi.substring(datarssi.indexOf("5000=") + 5, datarssi.indexOf(",5500")).toFloat();
  mzone[10] = datarssi.substring(datarssi.indexOf("5500=") + 5, datarssi.indexOf(",6000")).toFloat();

  mzone[11] = datarssi.substring(datarssi.indexOf("6000=") + 5, datarssi.indexOf(",6500")).toFloat();
  mzone[12] = datarssi.substring(datarssi.indexOf("6500=") + 5, datarssi.indexOf(",7000")).toFloat();
  mzone[13] = datarssi.substring(datarssi.indexOf("7000=") + 5, datarssi.indexOf(",7500")).toFloat();
  mzone[14] = datarssi.substring(datarssi.indexOf("7500=") + 5, datarssi.indexOf(",8000")).toFloat();
  mzone[15] = datarssi.substring(datarssi.indexOf("8000=") + 5, datarssi.length()).toFloat();
  Serial.print("mzone: ");
  for (int i = 0; i <= 15; i++) {
    Serial.print(mzone[i]);
  }
  Serial.println();
  //        1
  //-35, -81, -87, -91, -94, -97, -100, -106, -109, -112, -115, -120, -130, -140
  //  SETRSSI,1000=-5,1500=-123,2000=-250,2500=-4,3000=5,3500=120,4000=140,4500=1,5000=5,5500=5,6000=15,6500=19,7000=15,7500=0,8000=15
  //mzone[16]

  readStringFromEEPROM(0, &datarom);
  Serial.print("datarom:");
  Serial.println(datarom);
  configzone(datarom);
  Serial.print("rssi_zone1:");
  Serial.print(rssi_zone1[0]);
  Serial.println(rssi_zone1[1]);

  Serial.print("rssi_zone2:");
  Serial.print(rssi_zone2[0]);
  Serial.println(rssi_zone2[1]);

  Serial.print("rssi_zone3:");
  Serial.print(rssi_zone3[0]);
  Serial.println(rssi_zone3[1]);


}

void configzone(String dataset) {
  // ZONEINFO,DZ1=1500,DZ2=2000,DZ3=2500,SZ1=1,SZ2=3,SZ3=2

  int8_t index_dz1 =  dataset.indexOf("DZ1");
  int8_t index_dz2 =  dataset.indexOf("DZ2");
  int8_t index_dz3 =  dataset.indexOf("DZ3");

  int8_t index_sz1 =  dataset.indexOf("SZ1");
  int8_t index_sz2 =  dataset.indexOf("SZ2");
  int8_t index_sz3 =  dataset.indexOf("SZ3");

  number[0] = dataset.substring(index_dz1 + 4, index_dz2 - 1).toInt();
  number[1] = dataset.substring(index_dz2 + 4, index_dz3 - 1).toInt();
  number[2] = dataset.substring(index_dz3 + 4, index_sz1 - 1).toInt();
  number[3] = dataset.substring(index_sz1 + 4, index_sz2 - 1).toInt();
  number[4] = dataset.substring(index_sz2 + 4, index_sz3 - 1).toInt();
  number[5] = dataset.substring(index_sz3 + 4, index_sz3 + 6).toInt();

  Serial.println(number[0]); Serial.println(number[1]); Serial.println(number[2]);
  Serial.println(number[3]); Serial.println(number[4]); Serial.println(number[5]);

  numberpre1 = number[0] + number[1]; // 0 -> zone 2
  numberpre2 = number[0] + number[1] + number[2]; // 0 -> zone 3
  Serial.print("numberpre:");
  Serial.print(numberpre1);
  Serial.println(numberpre2);
  //zone 1
  switch (number[0]) {
    case 1000:
      rssi_zone1[0] = mzone[0];
      rssi_zone1[1] = mzone[1];
      break;
    case 1500:
      rssi_zone1[0] = mzone[0];
      rssi_zone1[1] = mzone[2];
      break;
    case 2000:
      rssi_zone1[0] = mzone[0];
      rssi_zone1[1] = mzone[3];
      break;
    case 2500:
      rssi_zone1[0] = mzone[0];
      rssi_zone1[1] = mzone[4];
      break;
    case 3000:
      rssi_zone1[0] = mzone[0];
      rssi_zone1[1] = mzone[5];
      break;
    case 3500:
      rssi_zone1[0] = mzone[0];
      rssi_zone1[1] = mzone[6];
      break;
    case 4000:
      rssi_zone1[0] = mzone[0];
      rssi_zone1[1] = mzone[7];
      break;
    case 4500:
      rssi_zone1[0] = mzone[0];
      rssi_zone1[1] = mzone[8];
      break;
    case 5000:
      rssi_zone1[0] = mzone[0];
      rssi_zone1[1] = mzone[9];
      break;
  }
  //----------------------ZONE 2-----------------
  switch (numberpre1) {
    case 2000:
      rssi_zone2[0] = rssi_zone1[1];
      rssi_zone2[1] = mzone[3];
      break;
    case 2500:
      rssi_zone2[0] = rssi_zone1[1];
      rssi_zone2[1] = mzone[4];
      break;
    case 3000:
      rssi_zone2[0] = rssi_zone1[1];
      rssi_zone2[1] = mzone[5];
      break;
    case 3500:
      rssi_zone2[0] = rssi_zone1[1];
      rssi_zone2[1] = mzone[6];
      break;
    case 4000:
      rssi_zone2[0] = rssi_zone1[1];
      rssi_zone2[1] = mzone[7];
      break;
    case 4500:
      rssi_zone2[0] = rssi_zone1[1];
      rssi_zone2[1] = mzone[8];
      break;
    case 5000:
      rssi_zone2[0] = rssi_zone1[1];
      rssi_zone2[1] = mzone[9];
      break;
    case 5500:
      rssi_zone2[0] = rssi_zone1[1];
      rssi_zone2[1] = mzone[10];
      break;
    case 6000:
      rssi_zone2[0] = rssi_zone1[1];
      rssi_zone2[1] = mzone[11];
      break;
    case 6500:
      rssi_zone2[0] = rssi_zone1[1];
      rssi_zone2[1] = mzone[12];
      break;
    case 7000:
      rssi_zone2[0] = rssi_zone1[1];
      rssi_zone2[1] = mzone[13];
      break;
    case 7500:
      rssi_zone2[0] = rssi_zone1[1];
      rssi_zone2[1] = mzone[14];
      break;
    case 8000:
      rssi_zone2[0] = rssi_zone1[1];
      rssi_zone2[1] = mzone[15];
      break;
    default:
      rssi_zone2[0] = rssi_zone1[1];
      rssi_zone2[1] = -155;
      break;

  }
  //---------------------------
  //-----------------------ZONE 3--------------------
  switch (numberpre2) {
    case 3000:
      rssi_zone3[0] =  rssi_zone2[1];
      rssi_zone3[1] = mzone[5];
      break;
    case 3500:
      rssi_zone3[0] = rssi_zone2[1];
      rssi_zone3[1] = mzone[6];
      break;
    case 4000:
      rssi_zone3[0] = rssi_zone2[1];
      rssi_zone3[1] = mzone[7];
      break;
    case 4500:
      rssi_zone3[0] = rssi_zone2[1];
      rssi_zone3[1] = mzone[8];
      break;
    case 5000:
      rssi_zone3[0] = rssi_zone2[1];
      rssi_zone3[1] = mzone[9];
      break;
    case 5500:
      rssi_zone3[0] = rssi_zone2[1];
      rssi_zone3[1] = mzone[10];
      break;
    case 6000:
      rssi_zone3[0] = rssi_zone2[1];
      rssi_zone3[1] = mzone[11];
      break;
    case 6500:
      rssi_zone3[0] = rssi_zone2[1];
      rssi_zone3[1] = mzone[12];
      break;
    case 7000:
      rssi_zone3[0] = rssi_zone2[1];
      rssi_zone3[1] = mzone[13];
      break;
    case 7500:
      rssi_zone3[0] = rssi_zone2[1];
      rssi_zone3[1] = mzone[14];
      break;
    case 8000:
      rssi_zone3[0] = rssi_zone2[1];
      rssi_zone3[1] = mzone[15];
      break;
    default:
      rssi_zone3[0] = rssi_zone2[1];
      rssi_zone3[1] = -170;
      break;
  }
  //------------------------------------


}

int writeStringToEEPROM(int addrOffset, const String & strToWrite)
{
  byte len = strToWrite.length();
  EEPROM.write(addrOffset, len);
  for (int i = 0; i < len; i++)
  {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
  }
  EEPROM.commit();
  return addrOffset + 1 + len;
}

int readStringFromEEPROM(int addrOffset, String * strToRead)
{
  int newStrLen = EEPROM.read(addrOffset);
  char data[newStrLen + 1];
  for (int i = 0; i < newStrLen; i++)
  {
    data[i] = EEPROM.read(addrOffset + 1 + i);
  }
  data[newStrLen] = '\ 0';
  *strToRead = String(data);
  return addrOffset + 1 + newStrLen;
}

void music1() {
  Serial.println("music1");

  digitalWrite(PIN_SIREN, HIGH);
  digitalWrite(PIN_LED, HIGH);
  vTaskDelay( 1000 / portTICK_PERIOD_MS );
  digitalWrite(PIN_SIREN, LOW);
  digitalWrite(PIN_LED, LOW);


//  digitalWrite(PIN_LED, HIGH);
//  vTaskDelay( 1000 / portTICK_PERIOD_MS );
//  digitalWrite(PIN_LED, LOW);
//  vTaskDelay( 1000 / portTICK_PERIOD_MS );

}
void music2() {
  Serial.println("music2");
  tone(PIN_SIREN, 20, 1000);
  digitalWrite(PIN_LED, HIGH);
  vTaskDelay( 1000 / portTICK_PERIOD_MS );
  noTone(PIN_SIREN);
  digitalWrite(PIN_SIREN, LOW);
  digitalWrite(PIN_LED, LOW);

//  digitalWrite(PIN_LED, HIGH);
//  vTaskDelay( 1000 / portTICK_PERIOD_MS );
//  digitalWrite(PIN_LED, LOW);
//  vTaskDelay( 1000 / portTICK_PERIOD_MS );
}
void music3() {
  Serial.println("music3");
  tone(PIN_SIREN, 15, 1000);
  digitalWrite(PIN_LED, HIGH);
  vTaskDelay( 1000 / portTICK_PERIOD_MS );
  noTone(PIN_SIREN);
  digitalWrite(PIN_SIREN, LOW);
  digitalWrite(PIN_LED, LOW);

//  digitalWrite(PIN_LED, HIGH);
//  vTaskDelay( 1000 / portTICK_PERIOD_MS );
//  digitalWrite(PIN_LED, LOW);
//  vTaskDelay( 1000 / portTICK_PERIOD_MS );
}
void music4() {
  Serial.println("music4");
  tone(PIN_SIREN, 400, 1000);
  digitalWrite(PIN_LED, HIGH);
  vTaskDelay( 1000 / portTICK_PERIOD_MS );
  noTone(PIN_SIREN);
  digitalWrite(PIN_SIREN, LOW);
  digitalWrite(PIN_LED, LOW);

//  digitalWrite(PIN_LED, HIGH);
//  vTaskDelay( 1000 / portTICK_PERIOD_MS );
//  digitalWrite(PIN_LED, LOW);
//  vTaskDelay( 1000 / portTICK_PERIOD_MS );
}
void music5() {
  Serial.println("music5");
  tone(PIN_SIREN, 4000, 400);
  digitalWrite(PIN_LED, HIGH);
  vTaskDelay( 1000 / portTICK_PERIOD_MS );
  noTone(PIN_SIREN);
  digitalWrite(PIN_SIREN, LOW);
  digitalWrite(PIN_LED, LOW);

//  digitalWrite(PIN_LED, HIGH);
//  vTaskDelay( 1000 / portTICK_PERIOD_MS );
//  digitalWrite(PIN_LED, LOW);
//  vTaskDelay( 1000 / portTICK_PERIOD_MS );
}
void music0() {
  Serial.println("music0");
  digitalWrite(PIN_LED, HIGH);
  vTaskDelay( 1000 / portTICK_PERIOD_MS );
  digitalWrite(PIN_LED, LOW);
}
void chooose_alarm(uint8_t chooose) {
  switch (chooose) {
    case 0:
      music0();
      break;
    case 1:
      music1();
      break;
    case 2:
      music2();
      break;
    case 3:
      music3();
      break;
    case 4:
      music4();
      break;
    case 5:
      music5();
      break;
  }
}
