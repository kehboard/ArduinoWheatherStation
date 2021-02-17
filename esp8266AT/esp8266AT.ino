#include <ESP8266WiFi.h>
#include "ESPFlashString.h"
#include "ESPFlash.h" 
#include "ESP8266WebServer.h"
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include "FS.h"
//#include "staticHTML.h"

// spi path for init system
#define SYS_RESTORED "/sysRestored"
// spi paths for hotspot
#define SOFTAP_SSID_PATH "/softSSID"
#define SOFTAP_PASS_PATH "/softPASS"
// spi paths for wifi ap
#define AP_ENABLE_PATH "/hardAPEN"
#define AP_SSID_PATH "/hardSSID"
#define AP_PASS_PATH "/hardPASS"
// spi paths for local logging (not implemented)
//#define LOC_LOGGING_PERIOD "/loclogPeriod"
//#define LOC_LOG_IN_TMP "/loclogInTmp"
//#define LOC_LOG_IN_HUM "/loclogInHum"
//#define LOC_LOG_IN_PRS "/loclogInPrs"
//#define LOC_LOG_OUT_TMP "/loclogOutTmp"
//#define LOC_LOG_OUT_HUM "/loclogOutHum"
//#define LOC_LOG_UNIXTIME "/loclogUnixTime"
// spi paths for narodmon
#define NARODMON_LOG_PERIOD "/narmonPeriod"
#define NARODMON_LOG_SETTINGS "/narmonSet"
#define NARODMON_LOG_ENABLE "/narmonEn"
// spi paths for time
#define TIME_DAYTIME_AUTO "/timeDaytimeAuto"
#define TIME_TO_DAYTIME_RULE "/timeToDaytimeRule"
#define TIME_FROM_DAYTIME_RULE "/timeFromDaytimeRule"
#define TIME_NTP_SERVER "/NtpServer"
#define TIME_NTP_USE "/useNtpServer"

//default For Hotspot
#define SOFTAP_SSID "SmartWeather"
#define SOFTAP_PASS "SmartWeather"

//default for wifi ap
#define AP_ENABLE true
#define AP_SSID "ASUSHOME"
#define AP_PASS "5AfAe5CaB92Dc12663D76dcfbB"

//default for narodmon
#define NARODMON_ENABLE false
#define NARODMON_PERIOD 5
#define NARODMON_USE_SENSOR_IN_TMP false
#define NARODMON_USE_SENSOR_IN_HUM false
#define NARODMON_USE_SENSOR_IN_PRS false
#define NARODMON_USE_SENSOR_OUT_TMP false
#define NARODMON_USE_SENSOR_OUT_HUM false

//default for local logging (not implemented)
//#define LOCAL_LOG_PERIOD 1
//#define LOCAL_LOG_STORAGE 100

//default for time
#define TIME_TO_DAYTIME_AUTO true
//enum week {Last=0, First, Second, Third, Fourth}; 
#define TIME_TO_DAYTIME_WEEK 0
//enum dow {Sun=1, Mon, Tue, Wed, Thu, Fri, Sat};
#define TIME_TO_DAYTIME_DOW 1
//enum month {Jan=1, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec};
#define TIME_TO_DAYTIME_MONTH 3
#define TIME_TO_DAYTIME_HOUR 3
#define TIME_TO_DAYTIME_OFFSET 180
//enum week {Last=0, First, Second, Third, Fourth}; 
#define TIME_FROM_DAYTIME_WEEK 0
//enum dow {Sun=1, Mon, Tue, Wed, Thu, Fri, Sat};
#define TIME_FROM_DAYTIME_DOW 1
//enum month {Jan=1, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec};
#define TIME_FROM_DAYTIME_MONTH 10
#define TIME_FROM_DAYTIME_HOUR 4
#define TIME_FROM_DAYTIME_OFFSET 120
#define USE_NTP_SERVER false
#define NTP_SERVER_HOST "ua.pool.ntp.org"

int loggingPeriod = 0;
int narodmonLoggingPeriod=0;
unsigned long getDataToLocStatistic=0;
unsigned long getDataToNarodmon=0;

ESP8266WebServer server(80);
int narodMonUpdateTime = 0;

void setup()
{
  if(!SPIFFS.begin()){
    //Serial.println("An Error has occurred while mounting SPIFFS");
  }
  restoreDefaults();
  ESPFlash<int> narodmonPeriod(NARODMON_LOG_PERIOD);
  narodMonUpdateTime = narodmonPeriod.get();
  //loggingPeriod=locLoggingPeriod.get();
  //delay(10000);
  ESPFlashString softAPSSID(SOFTAP_SSID_PATH, SOFTAP_SSID);
  ESPFlashString softAPPASS(SOFTAP_PASS_PATH, SOFTAP_PASS);
  // Setup serial port
  Serial.begin(115200);
  Serial.setTimeout(300);
  Serial.println();
  
  
  // Begin Access Point
  WiFi.mode(WIFI_AP_STA);
  if(softAPPASS.get()==""){
    WiFi.softAP(softAPSSID.get());
  }
  else{
    WiFi.softAP(softAPSSID.get(), softAPPASS.get());
  }
  connectToWifi();
  //Serial.print("IP address for network ");
  //Serial.print(softAPSSID.get());
  //Serial.print(":");
  //Serial.print(WiFi.softAPIP()); 
  //server.on("/hello", easterEgg);
  server.on("/getWifiStatus",getWifiStatus);
  server.on("/setWifi",setWifiCredetians);
  server.on("/getWifi",getWifiCredetians);
  server.on("/setHotspot",setWifiHotSpotCredetians);
  server.on("/getHotspot",getWifiHotSpotCredetians);
  server.on("/exportSettings", exportSettings);
  server.on("/hardreset", hardReset);
  /*server.on("/setLogging",setLoggingPeriod);
  server.on("/getLogging",getLoggingPeriod);*/
  server.on("/setTime",setTimeToDS3231);
  server.on("/getTime",getTimeFromDS3231);
  server.on("/setTimeZoneRules",setTimeZoneRules);
  server.on("/getTimeZoneRules",getTimeZoneRules);
  server.on("/getNTP",getNtpSettings);
  server.on("/setNTP",setNtpSettings);
  server.on("/forceNTPSync",forceNtpSync);
  server.on("/getNarodmon",getNarodmonSettings);
  server.on("/setNarodmon",setNarodmonSettings);
  server.on("/forceNarodmon",forceNarodmon);
  /*server.on("/getAllData",getAllData);*/
  server.on("/getActual",getActualData);
  server.on("/",indexPage);
  /*server.on("/uPlot.min.css",uplotCss);
  server.on("/uPlot.iife.min.js",uplotJs);*/
  server.on("/settings.html",settingsPage);
  server.begin();
}

void forceNarodmon(){
  float data[5]={-273.15,-273.15,-273.15,-273.15,-273.15};
  float minVal = -273.15;
  bool data0=true;
  bool data1=true;
  bool data2=true;
  bool data3=true;
  bool data4=true;
  while (data[0] == minVal && data[1] == minVal && data[3] == minVal && data[4] == minVal){
  Serial.println("AT&GETALL?");
  String response = Serial.readStringUntil('\n');
  if(response.startsWith("A")){
    response = response.substring(response.indexOf("=")+1, response.length());
    data[0] = response.substring(0, response.indexOf(',')).toFloat();
    response = response.substring(response.indexOf(',')+1, response.length());
    data[1] = response.substring(0, response.indexOf(',')).toFloat();
    response = response.substring(response.indexOf(',')+1, response.length());
    data[2] = response.substring(0, response.indexOf(',')).toFloat();
    response = response.substring(response.indexOf(',')+1, response.length());
    data[3] = response.substring(0, response.indexOf(',')).toFloat();
    response = response.substring(response.indexOf(',')+1, response.length());
    data[4] = response.substring(0, response.indexOf(',')).toFloat();
    response = response.substring(response.indexOf(',')+1, response.length());
   Serial.flush();
 }
 }
 if (data[0] != minVal && data[1] != minVal && data[3] != minVal && data[4] != minVal){
   String buff = "#";
   buff += WiFi.macAddress();
   buff += "\n";
   if (data0 != false){
    buff += "#S1#";
    buff += data[0];
    buff += "\n";
   }
   if (data1 != false){
    buff += "#S2#";
    buff += data[1];
    buff += "\n";
   }
   if (data2 != false){
    buff += "#S3#";
    buff += data[2];
    buff += "\n";
   }
   if (data3 != false){
    buff += "#S4#";
    buff += data[3];
    buff += "\n";
   }
   if (data4 != false){
    buff += "#S5#";
    buff += data[4];
    buff += "\n";
   }
   buff += "##\n";
   IPAddress NarodmonHost;
   WiFi.hostByName("narodmon.com", NarodmonHost);
   WiFiClient client;
   client.connect(NarodmonHost, 8283);
   client.print(buff);
   String responce = client.readStringUntil('\n');
   client.stop();
}
server.send(200,"text/html","SEND_OK");
}

void forceNtpSync(){
  DynamicJsonDocument doc(2048);
  String jsonString = "";
  ESPFlashString ntpServer(TIME_NTP_SERVER, NTP_SERVER_HOST);
  time_t epochTime =0;
    byte count=0;
    while (epochTime == 0 && count <5){
      epochTime = getNtpTime(ntpServer.get());
    }
    if(epochTime != 0){
    setUnixTimeSerial(epochTime);
    doc["time"]=epochTime;
    }
    else{
      doc["time"]=-1;
    }
  serializeJson(doc,jsonString);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200,"application/json",jsonString);
}

void getActualData(){
  DynamicJsonDocument doc(2048);
  String jsonString = "";
  //doc["unixtime"] = getUnixTimeSerial();
  Serial.println("AT&GETALL?");
  //delay(300);
  String response = Serial.readStringUntil('\n');
  if(response.startsWith("A")){
    response = response.substring(response.indexOf("=")+1, response.length());
    doc["locTmp"] = response.substring(0, response.indexOf(',')).toFloat();
    response = response.substring(response.indexOf(',')+1, response.length());
    doc["locHum"] = response.substring(0, response.indexOf(',')).toFloat();
    response = response.substring(response.indexOf(',')+1, response.length());
    doc["locPrs"] = response.substring(0, response.indexOf(',')).toFloat();
    response = response.substring(response.indexOf(',')+1, response.length());
    doc["ouTmp"] = response.substring(0, response.indexOf(',')).toFloat();
    response = response.substring(response.indexOf(',')+1, response.length());
    doc["ouHum"] = response.substring(0, response.indexOf(',')).toFloat();
    response = response.substring(response.indexOf(',')+1, response.length());
    doc["unixtime"]=strtol(response.c_str(),NULL,0);
  }
  else{
    doc["locTmp"] = -273.15;
    doc["locHum"] = -1.00;
    doc["locPrs"] = -1.00;
    doc["ouTmp"] = -273.15;
    doc["ouHum"] = -1.00;
    doc["unixtime"]= -1.00;
  }
  Serial.flush();
  serializeJson(doc,jsonString);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json",jsonString);
}

void settingsPage(){
  File file = SPIFFS.open("/settings.html", "r");
  size_t sent = server.streamFile(file, "text/html");
  file.close();
}

void indexPage(){
  File file = SPIFFS.open("/index.html", "r");
  size_t sent = server.streamFile(file, "text/html");
  file.close();
}

void uplotCss(){
  File file = SPIFFS.open("/uPlot.min.css", "r");
  size_t sent = server.streamFile(file, "text/css");
  file.close();  
}

void uplotJs(){
  File file = SPIFFS.open("/uPlot.iife.min.js", "r");
  size_t sent = server.streamFile(file, "text/javascript");
  file.close();  
}

void getTimeFromDS3231(){
  DynamicJsonDocument doc(2048);
  String jsonString = "";
  doc["unixtime"] = getUnixTimeSerial();
  serializeJson(doc,jsonString);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json",jsonString);
}

void setTimeToDS3231(){
  if(server.args()>0){
  if(server.arg(F("unixtime")) != ""){
     setUnixTimeSerial(strtol(server.arg(F("unixtime")).c_str(),NULL,0));
  }
  DynamicJsonDocument doc(2048);
  String jsonString = "";
  doc["unixtime"] = getUnixTimeSerial();
  serializeJson(doc,jsonString);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json",jsonString);
  }
}


void setUnixTimeSerial(time_t unixTime){
  Serial.print("AT&LOCUTIME=");
  Serial.println(unixTime);
}

time_t getUnixTimeSerial(){
  Serial.println(F("AT&LOCUTIME?"));
  String respond;
  respond = Serial.readStringUntil('\n');
  time_t curTime = 0L;
  if(respond.startsWith("A")){
    if(respond.substring(respond.indexOf('=')+1, respond.length()).length()!=0){
      curTime = strtol(respond.substring(respond.indexOf('=')+1, respond.length()).c_str(), NULL, 0);
    }
  }
  return curTime;
}

void setTimeZoneRules(){
  DynamicJsonDocument doc(2048);
  String jsonString = "";
  ESPFlash<bool> autoSetDaytime(TIME_DAYTIME_AUTO);
  ESPFlash<int> autoRuleToDaytime(TIME_TO_DAYTIME_RULE);
  ESPFlash<int> autoRuleFromDaytime(TIME_FROM_DAYTIME_RULE);
  int autoRuleToDaytimeArr[5];
  int autoRuleFromDaytimeArr[5];
  autoRuleToDaytime.getBackElements(autoRuleToDaytimeArr,sizeof(autoRuleToDaytimeArr));
  autoRuleFromDaytime.getBackElements(autoRuleFromDaytimeArr,sizeof(autoRuleFromDaytimeArr));
  if(server.args()>0){
  if(server.arg(F("auSetDayTime")) != ""){
     autoSetDaytime.set(server.arg(F("auSetDayTime"))=="1"? true : false);
  }
  if(server.arg(F("toDTWeek")) != ""){
    autoRuleToDaytimeArr[0] = server.arg(F("toDTWeek")).toInt();
  }
  if(server.arg(F("toDTDoW")) != ""){
    autoRuleToDaytimeArr[1] = server.arg(F("toDTDoW")).toInt();
  }
    if(server.arg(F("toDTMon")) != ""){
    autoRuleToDaytimeArr[2] = server.arg(F("toDTMon")).toInt();
  }
    if(server.arg(F("toDTHour")) != ""){
    autoRuleToDaytimeArr[3] = server.arg(F("toDTHour")).toInt();
  }
    if(server.arg(F("toDTOff")) != ""){
   autoRuleToDaytimeArr[4] = server.arg(F("toDTOff")).toInt();
  }
  if(server.arg(F("fromDTWeek")) != ""){
    autoRuleFromDaytimeArr[0] = server.arg(F("fromDTWeek")).toInt();
  }
  if(server.arg(F("fromDTDoW")) != ""){
    autoRuleFromDaytimeArr[1] = server.arg(F("fromDTDoW")).toInt();
  }
    if(server.arg(F("fromDTMon")) != ""){
    autoRuleFromDaytimeArr[2] = server.arg(F("fromDTMon")).toInt();
  }
    if(server.arg(F("fromDTHour")) != ""){
   autoRuleFromDaytimeArr[3] = server.arg(F("fromDTHour")).toInt();
  }
    if(server.arg(F("fromDTOff")) != ""){
    autoRuleFromDaytimeArr[4] = server.arg(F("fromDTOff")).toInt();
  }
  Serial.print("AT&DTIME=");
  Serial.print(autoSetDaytime.get());
  Serial.print(",");
  Serial.print(autoRuleToDaytimeArr[0]);
  Serial.print(",");
  Serial.print(autoRuleToDaytimeArr[1]);
  Serial.print(",");
  Serial.print(autoRuleToDaytimeArr[2]);
  Serial.print(",");
  Serial.print(autoRuleToDaytimeArr[3]);
  Serial.print(",");
  Serial.print(autoRuleToDaytimeArr[4]);
  Serial.print(",");
  Serial.print(autoRuleFromDaytimeArr[0]);
  Serial.print(",");
  Serial.print(autoRuleFromDaytimeArr[1]);
  Serial.print(",");
  Serial.print(autoRuleFromDaytimeArr[2]);
  Serial.print(",");
  Serial.print(autoRuleFromDaytimeArr[3]);
  Serial.print(",");
  Serial.println(autoRuleFromDaytimeArr[4]);
  }
  autoRuleToDaytime.setElements(autoRuleToDaytimeArr,sizeof(autoRuleToDaytimeArr));
  autoRuleFromDaytime.setElements(autoRuleFromDaytimeArr,sizeof(autoRuleFromDaytimeArr));
  doc["auSetDayTime"]=autoSetDaytime.get();
  JsonArray ruleto = doc.createNestedArray("ruleToDt");
  for(int i=0; i<5;i++){
    ruleto.add(autoRuleToDaytime.getElementAt(i));
  }
  JsonArray rulefrom = doc.createNestedArray("ruleFromDt");
  for(int i=0; i<5;i++){
    rulefrom.add(autoRuleFromDaytime.getElementAt(i));
  }
  serializeJson(doc,jsonString);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json",jsonString);
}

void getTimeZoneRules(){
  DynamicJsonDocument doc(2048);
  String jsonString = "";
  ESPFlash<bool> autoSetDaytime(TIME_DAYTIME_AUTO);
  ESPFlash<int> autoRuleToDaytime(TIME_TO_DAYTIME_RULE);
  ESPFlash<int> autoRuleFromDaytime(TIME_FROM_DAYTIME_RULE);
  doc["auSetDayTime"]=autoSetDaytime.get();
  JsonArray ruleto = doc.createNestedArray("ruleToDt");
  for(int i=0; i<5;i++){
    ruleto.add(autoRuleToDaytime.getElementAt(i));
  }
  JsonArray rulefrom = doc.createNestedArray("ruleFromDt");
  for(int i=0; i<5;i++){
    rulefrom.add(autoRuleFromDaytime.getElementAt(i));
  }
  serializeJson(doc,jsonString);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json",jsonString);
}

void setNtpSettings(){
  DynamicJsonDocument doc(2048);
  String jsonString = "";
  ESPFlashString ntpHost(TIME_NTP_SERVER, NTP_SERVER_HOST);
  ESPFlash<bool> useNtp(TIME_NTP_USE);
  if(server.args()>0){
  if(server.arg(F("server")) != ""){
     ntpHost.set(server.arg(F("server")));
  }
  if(server.arg(F("use")) != ""){
   useNtp.set(server.arg(F("use"))=="1"? true : false);
    }
  }
  doc["server"]=ntpHost.get();
  doc["use"]=useNtp.get();
  serializeJson(doc,jsonString);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json",jsonString);
}

void getNtpSettings(){
  DynamicJsonDocument doc(2048);
  String jsonString = "";
  ESPFlashString ntpHost(TIME_NTP_SERVER, NTP_SERVER_HOST);
  ESPFlash<bool> useNtp(TIME_NTP_USE);
  doc["server"]=ntpHost.get();
  doc["use"]=useNtp.get();
  serializeJson(doc,jsonString);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json",jsonString);
}

void getNarodmonSettings(){
  DynamicJsonDocument doc(2048);
  String jsonString = "";
  ESPFlash<bool> narodmonEnable(NARODMON_LOG_ENABLE);
  ESPFlash<int> narodmonPeriod(NARODMON_LOG_PERIOD);
  ESPFlash<bool> narodmonSettings(NARODMON_LOG_SETTINGS);
  doc["nMonEn"]=narodmonEnable.get();
  doc["nMonLogPer"]=narodmonPeriod.get();
  bool nmonset[5];
  narodmonSettings.getBackElements(nmonset, sizeof(nmonset));
  JsonArray ports = doc.createNestedArray("nMonSet");
  for(int i=0; i<5;i++){
    ports.add(nmonset[i]);
  }

  serializeJson(doc,jsonString);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json",jsonString);
}

void setNarodmonSettings(){
  DynamicJsonDocument doc(2048);
  String jsonString = "";
  ESPFlash<bool> narodmonEnable(NARODMON_LOG_ENABLE);
  ESPFlash<int> narodmonPeriod(NARODMON_LOG_PERIOD);
  ESPFlash<bool> narodmonSettings(NARODMON_LOG_SETTINGS);
  bool nmonset[5];
  narodmonSettings.getBackElements(nmonset, sizeof(nmonset));
    if(server.args()>0){
      if(server.arg(F("nMonEn")) != ""){
         narodmonEnable.set(server.arg(F("nMonEn")) == "1" ? true : false);
      }
      if(server.arg(F("nMonSet")) != ""){
       for(int i =0; i<5;i++){
          nmonset[i]=bitRead(server.arg(F("nMonSet")).toInt(),i);
       }
        narodmonSettings.setElements(nmonset, sizeof(nmonset));
      }
      if(server.arg(F("nMonLogPer")) != ""){
        narodmonPeriod.set(server.arg(F("nMonLogPer")).toInt());
      }      
    }
  doc["nMonEn"]=narodmonEnable.get();
  doc["nMonLogPer"]=narodmonPeriod.get();
  JsonArray ports = doc.createNestedArray("nMonSet");
  for(int i=0; i<5;i++){
    ports.add(nmonset[i]);
  }

  serializeJson(doc,jsonString);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json",jsonString);
}

void hardReset(){
  ESPFlash<bool> restore(SYS_RESTORED);
  restore.set(false);
  restoreDefaults();
  server.send(204,"text/html");
  
  ESP.restart();
}


void exportSettings(){
  DynamicJsonDocument doc(3000);
  String jsonString = "";
  ESPFlashString softAPSSID(SOFTAP_SSID_PATH, SOFTAP_SSID);
  ESPFlashString softAPPASS(SOFTAP_PASS_PATH, SOFTAP_PASS);
  ESPFlashString hardAPSSID(AP_SSID_PATH, AP_SSID);
  ESPFlashString hardAPPASS(AP_PASS_PATH, AP_PASS);
  ESPFlash<bool> hardAPEN(AP_ENABLE_PATH);
  //ESPFlash<int> locLoggingPeriod(LOC_LOGGING_PERIOD);
  ESPFlash<bool> narodmonEnable(NARODMON_LOG_ENABLE);
  ESPFlash<int> narodmonPeriod(NARODMON_LOG_PERIOD);
  ESPFlash<bool> narodmonSettings(NARODMON_LOG_SETTINGS);
  ESPFlash<bool> autoSetDaytime(TIME_DAYTIME_AUTO);
  ESPFlash<int> autoRuleToDaytime(TIME_TO_DAYTIME_RULE);
  ESPFlash<int> autoRuleFromDaytime(TIME_FROM_DAYTIME_RULE);
  doc["SoSSID"]=softAPSSID.get();
  doc["SoPass"]=softAPPASS.get();
  doc["HdEn"]=hardAPEN.get();
  doc["HdSSID"]=hardAPSSID.get();
  doc["HdPass"]=hardAPPASS.get();
  //doc["locLogPer"]=locLoggingPeriod.get();
  doc["nMonEn"]=narodmonEnable.get();
  JsonArray ports = doc.createNestedArray("nMonSet");
  for(int i=0; i<5;i++){
    ports.add(narodmonSettings.getElementAt(i));
  }
  doc["nMonLogPer"]=narodmonPeriod.get();
  doc["auSetDayTime"]=autoSetDaytime.get();
  JsonArray ruleto = doc.createNestedArray("ruleToDt");
  for(int i=0; i<5;i++){
    ruleto.add(autoRuleToDaytime.getElementAt(i));
  }
  JsonArray rulefrom = doc.createNestedArray("ruleFromDt");
  for(int i=0; i<5;i++){
    rulefrom.add(autoRuleFromDaytime.getElementAt(i));
  }
  serializeJson(doc,jsonString);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json",jsonString);
}

void restoreDefaults(){
  ESPFlash<bool> restored(SYS_RESTORED);
  if(!restored.get()){
    //ESPFlash<float> loggedInTmp(LOC_LOG_IN_TMP);
    //loggedInTmp.size();
    //ESPFlash<float> loggedInHum(LOC_LOG_IN_HUM);
    //ESPFlash<float> loggedInPrs(LOC_LOG_IN_PRS);
    //ESPFlash<float> loggedOutTmp(LOC_LOG_OUT_TMP);
    //ESPFlash<float> loggedOutHum(LOC_LOG_OUT_HUM);
    //ESPFlash<time_t> loggedTime(LOC_LOG_UNIXTIME);
    
    ESPFlashString softAPSSID(SOFTAP_SSID_PATH, SOFTAP_SSID);
    ESPFlashString softAPPASS(SOFTAP_PASS_PATH, SOFTAP_PASS);
    ESPFlashString hardAPSSID(AP_SSID_PATH, AP_SSID);
    ESPFlashString hardAPPASS(AP_PASS_PATH, AP_PASS);
    ESPFlash<bool> hardAPEN(AP_ENABLE_PATH);
    hardAPEN.set(AP_ENABLE);
    //ESPFlash<int> locLoggingPeriod(LOC_LOGGING_PERIOD);
    //locLoggingPeriod.set(LOCAL_LOG_PERIOD);
    ESPFlash<bool> narodmonEnable(NARODMON_LOG_ENABLE);
    narodmonEnable.set(NARODMON_ENABLE);
    ESPFlash<int> narodmonPeriod(NARODMON_LOG_PERIOD);
    narodmonPeriod.set(NARODMON_PERIOD);
    ESPFlash<bool> narodmonSettings(NARODMON_LOG_SETTINGS);
    narodmonSettings.append(NARODMON_USE_SENSOR_IN_TMP); 
    narodmonSettings.append(NARODMON_USE_SENSOR_IN_HUM); 
    narodmonSettings.append(NARODMON_USE_SENSOR_IN_PRS); 
    narodmonSettings.append(NARODMON_USE_SENSOR_OUT_TMP);
    narodmonSettings.append(NARODMON_USE_SENSOR_OUT_HUM);
    ESPFlash<bool> autoSetDaytime(TIME_DAYTIME_AUTO);
    autoSetDaytime.set(TIME_TO_DAYTIME_AUTO);
    ESPFlash<int> autoRuleToDaytime(TIME_TO_DAYTIME_RULE);
    autoRuleToDaytime.append(TIME_TO_DAYTIME_WEEK);
    autoRuleToDaytime.append(TIME_TO_DAYTIME_DOW);
    autoRuleToDaytime.append(TIME_TO_DAYTIME_MONTH);
    autoRuleToDaytime.append(TIME_TO_DAYTIME_HOUR);
    autoRuleToDaytime.append(TIME_TO_DAYTIME_OFFSET);
    ESPFlash<int> autoRuleFromDaytime(TIME_FROM_DAYTIME_RULE);
    autoRuleFromDaytime.append(TIME_FROM_DAYTIME_WEEK);
    autoRuleFromDaytime.append(TIME_FROM_DAYTIME_DOW);
    autoRuleFromDaytime.append(TIME_FROM_DAYTIME_MONTH);
    autoRuleFromDaytime.append(TIME_FROM_DAYTIME_HOUR);
    autoRuleFromDaytime.append(TIME_FROM_DAYTIME_OFFSET);
    ESPFlashString ntpHost(TIME_NTP_SERVER, NTP_SERVER_HOST);
    ESPFlash<bool> useNtp(TIME_NTP_USE);
    Serial.print("AT&DTIME=");
    Serial.print(TIME_DAYTIME_AUTO);
    Serial.print(",");
    Serial.print(TIME_TO_DAYTIME_WEEK);
    Serial.print(",");
    Serial.print(TIME_TO_DAYTIME_DOW);
    Serial.print(",");
    Serial.print(TIME_TO_DAYTIME_MONTH);
    Serial.print(","); 
    Serial.print(TIME_TO_DAYTIME_HOUR);
    Serial.print(",");
    Serial.print(TIME_FROM_DAYTIME_WEEK);
    Serial.print(",");
    Serial.print(TIME_FROM_DAYTIME_DOW);
    Serial.print(",");
    Serial.print(TIME_FROM_DAYTIME_MONTH);
    Serial.print(",");
    Serial.print(TIME_FROM_DAYTIME_HOUR);
    Serial.print(",");
    Serial.println(TIME_FROM_DAYTIME_OFFSET);
    useNtp.set(USE_NTP_SERVER);
    restored.set(true);
    }
}

void setWifiHotSpotCredetians(){
  ESPFlashString softAPSSID(SOFTAP_SSID_PATH, SOFTAP_SSID);
  ESPFlashString softAPPASS(SOFTAP_PASS_PATH, SOFTAP_PASS);
  if(server.args()>0){
    if(server.arg(F("ssid")) != ""){
      if(server.arg(F("pass")) != F("")){
        softAPSSID.set(server.arg(F("ssid")));
        softAPPASS.set(server.arg(F("pass")));
        WiFi.softAPdisconnect();
        WiFi.softAP(softAPSSID.get(),softAPPASS.get());
      }
      else
      {
        softAPSSID.set(server.arg(F("ssid")));
        softAPPASS.set(server.arg(F("pass")));
        WiFi.softAPdisconnect();
        WiFi.softAP(softAPSSID.get());
      }
    }
  }
  DynamicJsonDocument doc(2048);
  String jsonString;
  doc["ssid"]=softAPSSID.get();
  doc["pass"]=softAPPASS.get();
  serializeJson(doc,jsonString);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json",jsonString);
}

void getWifiHotSpotCredetians(){
  ESPFlashString softAPSSID(SOFTAP_SSID_PATH, SOFTAP_SSID);
  ESPFlashString softAPPASS(SOFTAP_PASS_PATH, SOFTAP_PASS);
  DynamicJsonDocument doc(2048);
  String jsonString;
  doc["ssid"]=softAPSSID.get();
  doc["pass"]=softAPPASS.get();
  serializeJson(doc,jsonString);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json",jsonString);
}

void getWifiStatus(){
  DynamicJsonDocument doc(2048);
  doc["mac"] = WiFi.macAddress();
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    http.begin("http://clients3.google.com/generate_204");
    int httpCode = http.GET();
    if(httpCode == 204){
      doc["status"] = 2;
    }
    else{
      doc["status"] = 1;
    }
    doc["ip"] = WiFi.localIP().toString();
  }
  else{
    doc["ip"] = WiFi.localIP().toString();
    doc["status"] = 0;
  }
  String jsonString;
  serializeJson(doc,jsonString);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json",jsonString);
}

void setWifiCredetians(){
  if(server.args()>0){
    if(server.arg(F("apen")) != ""){
      ESPFlashString hardAPSSID(AP_SSID_PATH, AP_SSID);
      ESPFlashString hardAPPASS(AP_PASS_PATH, AP_PASS);
      ESPFlash<bool> hardAPEN(AP_ENABLE_PATH);
      if(server.arg(F("apen")) == F("1")){
        hardAPEN.set(true);
        hardAPSSID.set(server.arg(F("ssid")));
        hardAPPASS.set(server.arg(F("pass")));
      }
      else
      {
        hardAPEN.set(false);
      }
    }
  }
  DynamicJsonDocument doc(2048);
  String jsonString;
  doc["status"]=1;
  doc["mac"]=WiFi.macAddress();
  doc["ip"]=WiFi.localIP().toString();
  doc["isHot"]=  server.client().remoteIP().toString().startsWith("192.168.4") ? true : false;
  serializeJson(doc,jsonString);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json",jsonString);
  connectToWifi();
}

void getWifiCredetians(){
  DynamicJsonDocument doc(2048);
  String jsonString;
  ESPFlash<bool> hardAPEN(AP_ENABLE_PATH);
  ESPFlashString hardAPSSID(AP_SSID_PATH, AP_SSID);
  ESPFlashString hardAPPASS(AP_PASS_PATH, AP_PASS);
  doc["apen"]=hardAPEN.get();
  doc["ssid"]=hardAPSSID.get();
  doc["pass"]=hardAPPASS.get();
  serializeJson(doc,jsonString);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json",jsonString);
}

void connectToWifi(){
  ESPFlash<bool> hardAPEN(AP_ENABLE_PATH);
  ESPFlashString hardAPSSID(AP_SSID_PATH, AP_SSID);
  ESPFlashString hardAPPASS(AP_PASS_PATH, AP_PASS);
  if(hardAPEN.get() == false){
    WiFi.disconnect();
    WiFi.setAutoConnect(false);
  }
  else if(hardAPPASS.get().length()>=8){
  WiFi.begin(hardAPSSID.get(), hardAPPASS.get());
  WiFi.setAutoConnect(true);
  }
  else{
    WiFi.begin(hardAPSSID.get());
    WiFi.setAutoConnect(true);
    }
}

//void easterEgg(){
//  server.send(200, "text/html", "<body></body><script>document.body.InnerHTML = window.atob(\"WFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYClhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWApYWCAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgWFgKWFggICBNTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTSAgIFhYClhYICAgTU1NTU1NTU1NTU1NTU1NTU1NTU1Nc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3NNTU1NTU1NTU1NTU1NTU1NTU1NTU0gICBYWApYWCAgIE1NTU1NTU1NTU1NTU1NTU1zcycnJyAgICAgICAgICAgICAgICAgICAgICAgICAgJycnc3NNTU1NTU1NTU1NTU1NTU1NICAgWFgKWFggICBNTU1NTU1NTU1NTU15eScnICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgJyd5eU1NTU1NTU1NTU1NTSAgIFhYClhYICAgTU1NTU1NTU15eScnICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAnJ3l5TU1NTU1NTU0gICBYWApYWCAgIE1NTU1NeScnICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICcneU1NTU1NICAgWFgKWFggICBNTU15JyAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAneU1NTSAgIFhYClhYICAgTWgnICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAnaE0gICBYWApYWCAgIC0gICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAtICAgWFgKWFggICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIFhYClhYICAgOjogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgOjogICBYWApYWCAgIE1NaGguICAgICAgICAuLmhoaGhoaC4uICAgICAgICAgICAgICAgICAgICAgIC4uaGhoaGhoLi4gICAgICAgIC5oaE1NICAgWFgKWFggICBNTU1NTWggICAuLmhoTU1NTU1NTU1NTWhoLiAgICAgICAgICAgICAgICAuaGhNTU1NTU1NTU1NaGguLiAgIGhNTU1NTSAgIFhYClhYICAgLS0tTU1NIC5oTU1NTWRkOjo6ZE1NTU1NTU1oaC4uICAgICAgICAuLmhoTU1NTU1NTWQ6OjpkZE1NTU1oLiBNTU0tLS0gICBYWApYWCAgIE1NTU1NTSBNTW1tJycgICAgICAnbW1NTU1NTU1NTXl5LiAgLnl5TU1NTU1NTU1tbScgICAgICAnJ21tTU0gTU1NTU1NICAgWFgKWFggICAtLS1tTU0gJycgICAgICAgICAgICAgJ21tTU1NTU1NTU0gIE1NTU1NTU1NbW0nICAgICAgICAgICAgICcnIE1NbS0tLSAgIFhYClhYICAgeXl5eW0nICAgIC4gICAgICAgICAgICAgICdtTU1NTW0nICAnbU1NTU1tJyAgICAgICAgICAgICAgLiAgICAnbXl5eXkgICBYWApYWCAgIG1tJycgICAgLnknICAgICAuLnl5eXl5Li4gICcnJycgICAgICAnJycnICAuLnl5eXl5Li4gICAgICd5LiAgICAnJ21tICAgWFgKWFggICAgICAgICAgIE1OICAgIC5zTU1NTU1NTU1Nc3MuICAgLiAgICAuICAgLnNzTU1NTU1NTU1Ncy4gICAgTk0gICAgICAgICAgIFhYClhYICAgICAgICAgICBOYCAgICBNTU1NTU1NTU1NTU1NTiAgIE0gICAgTSAgIE5NTU1NTU1NTU1NTU1NICAgIGBOICAgICAgICAgICBYWApYWCAgICAgICAgICAgICsgIC5zTU5OTk5OTU1NTU1OKyAgIGBOICAgIE5gICAgK05NTU1NTU5OTk5OTXMuICArICAgICAgICAgICAgWFgKWFggICAgICAgICAgICAgIG8rKysgICAgICsrKytNbyAgICBNICAgICAgTSAgICBvTSsrKysgICAgICsrK28gICAgICAgICAgICAgIFhYClhYICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICBvbyAgICAgIG9vICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICBYWApYWCAgICAgICAgICAgb00gICAgICAgICAgICAgICAgIG9vICAgICAgICAgIG9vICAgICAgICAgICAgICAgICBNbyAgICAgICAgICAgWFgKWFggICAgICAgICBvTU1vICAgICAgICAgICAgICAgIE0gICAgICAgICAgICAgIE0gICAgICAgICAgICAgICAgb01NbyAgICAgICAgIFhYClhYICAgICAgICtNTU1NICAgICAgICAgICAgICAgICBzICAgICAgICAgICAgICBzICAgICAgICAgICAgICAgICBNTU1NKyAgICAgICBYWApYWCAgICAgICtNTU1NTSsgICAgICAgICAgICArKytOTk5OKyAgICAgICAgK05OTk4rKysgICAgICAgICAgICArTU1NTU0rICAgICAgWFgKWFggICAgICtNTU1NTU1NKyAgICAgICArK05OTU1NTU1NTU1OKyAgICArTk1NTU1NTU1NTk4rKyAgICAgICArTU1NTU1NTSsgICAgIFhYClhYICAgICBNTU1NTU1NTU1OTisrK05OTU1NTU1NTU1NTU1NTU1OTk5OTU1NTU1NTU1NTU1NTU1OTisrK05OTU1NTU1NTU1NICAgICBYWApYWCAgICAgeU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NeSAgICAgWFgKWFggICBtICB5TU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NeSAgbSAgIFhYClhYICAgTU1tIHlNTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NTU1NeSBtTU0gICBYWApYWCAgIE1NTW0gLnl5TU1NTU1NTU1NTU1NTU1NTSAgICAgTU1NTU1NTU1NTSAgICAgTU1NTU1NTU1NTU1NTU1NTXl5LiBtTU1NICAgWFgKWFggICBNTU1NZCAgICcnJydoaGhoaCAgICAgICBvZGRkbyAgICAgICAgICBvYmJibyAgICAgICAgaGhoaCcnJycgICBkTU1NTSAgIFhYClhYICAgTU1NTU1kICAgICAgICAgICAgICdoTU1NTU1NTU1NTWRkZGRkZE1NTU1NTU1NTU1oJyAgICAgICAgICAgICBkTU1NTU0gICBYWApYWCAgIE1NTU1NTWQgICAgICAgICAgICAgICdoTU1NTU1NTU1NTU1NTU1NTU1NTU1NTWgnICAgICAgICAgICAgICBkTU1NTU1NICAgWFgKWFggICBNTU1NTU1NLSAgICAgICAgICAgICAgICcnZGRNTU1NTU1NTU1NTU1NTWRkJycgICAgICAgICAgICAgICAtTU1NTU1NTSAgIFhYClhYICAgTU1NTU1NTU0gICAgICAgICAgICAgICAgICAgJzo6ZGRkZGRkZGQ6OicgICAgICAgICAgICAgICAgICAgTU1NTU1NTU0gICBYWApYWCAgIE1NTU1NTU1NLSAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgLU1NTU1NTU1NICAgWFgKWFggICBNTU1NTU1NTU0gICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIE1NTU1NTU1NTSAgIFhYClhYICAgTU1NTU1NTU1NeSAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIHlNTU1NTU1NTU0gICBYWApYWCAgIE1NTU1NTU1NTU15LiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgLnlNTU1NTU1NTU1NICAgWFgKWFggICBNTU1NTU1NTU1NTU15LiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAueU1NTU1NTU1NTU1NTSAgIFhYClhYICAgTU1NTU1NTU1NTU1NTU15LiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIC55TU1NTU1NTU1NTU1NTU0gICBYWApYWCAgIE1NTU1NTU1NTU1NTU1NTU1zLiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgLnNNTU1NTU1NTU1NTU1NTU1NICAgWFgKWFggICBNTU1NTU1NTU1NTU1NTU1NTU1zcy4gICAgICAgICAgIC4uLi4gICAgICAgICAgIC5zc01NTU1NTU1NTU1NTU1NTU1NTSAgIFhYClhYICAgTU1NTU1NTU1NTU1NTU1NTU1NTU1ObyAgICAgICAgIG9OTk5ObyAgICAgICAgIG9OTU1NTU1NTU1NTU1NTU1NTU1NTU0gICBYWApYWCAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgWFgKWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYClhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWAoKICAgIC5vODhvLiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICBvOG8gICAgICAgICAgICAgICAgLgogICAgODg4IGAiICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIGAiJyAgICAgICAgICAgICAgLm84CiAgIG84ODhvbyAgIC5vb29vLm8gIC5vb29vby4gICAub29vb28uICBvb29vICAgLm9vb29vLiAgLm84ODhvbyBvb29vICAgIG9vbwogICAgODg4ICAgIGQ4OCggICI4IGQ4OCcgYDg4YiBkODgnIGAiWTggYDg4OCAgZDg4JyBgODhiICAgODg4ICAgIGA4OC4gIC44JwogICAgODg4ICAgIGAiWTg4Yi4gIDg4OCAgIDg4OCA4ODggICAgICAgIDg4OCAgODg4b29vODg4ICAgODg4ICAgICBgODguLjgnCiAgICA4ODggICAgby4gICk4OGIgODg4ICAgODg4IDg4OCAgIC5vOCAgODg4ICA4ODggICAgLm8gICA4ODggLiAgICBgODg4JwogICBvODg4byAgIDgiIjg4OFAnIGBZOGJvZDhQJyBgWThib2Q4UCcgbzg4OG8gYFk4Ym9kOFAnICAgIjg4OCIgICAgICBkOCcKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIC5vLi4uUCcKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIGBYRVIwJw==\")</script>");
//}


unsigned long updateTimeFromNTP = 0;

bool hasInternetConn(){
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    http.begin("http://clients3.google.com/generate_204");
    int httpCode = http.GET();
    if(httpCode == 204){
      return true;
    }
    else{
      return false;
    }
  }
  else{
    return false;
  }
}

time_t getNtpTime(String ntpServerName) {
  const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
  byte packetBuffer[NTP_PACKET_SIZE];
  WiFiUDP udp;
  udp.begin(2390);
  IPAddress timeServerIP; 
  //get a random server from the pool
  WiFi.hostByName(ntpServerName.c_str(), timeServerIP);
  //sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(timeServerIP, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
  delay(1000);
  int cb = udp.parsePacket();
  if (!cb) {
    return 0;
  } else {
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    // now convert NTP time into everyday time:
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    return epoch;
}
}

void syncTimeFromNTP(){
  ESPFlashString ntpServer(TIME_NTP_SERVER, NTP_SERVER_HOST);
  if(hasInternetConn()){
    time_t epochTime =0;
    byte count=0;
    while (epochTime == 0 && count <5){
      epochTime = getNtpTime(ntpServer.get());
    }
    if(epochTime != 0){
    setUnixTimeSerial(epochTime);
    }
  }
}

void handleNtpSettings(){
    if (millis() - updateTimeFromNTP > 60000){
      ESPFlash<bool> ntpEnable(TIME_NTP_USE);
      if(ntpEnable.get()){
        syncTimeFromNTP();
      }
      updateTimeFromNTP = millis();
    }
}

void sendDataToNarodmon(bool data0,bool data1,bool data2,bool data3,bool data4){
 if(hasInternetConn()){
float data[5]={-273.15,-273.15,-273.15,-273.15,-273.15};
  float minVal = -273.15;
  bool data0=true;
  bool data1=true;
  bool data2=true;
  bool data3=true;
  bool data4=true;
  while (data[0] == minVal && data[1] == minVal && data[3] == minVal && data[4] == minVal){
  Serial.println("AT&GETALL?");
  String response = Serial.readStringUntil('\n');
  if(response.startsWith("A")){
    response = response.substring(response.indexOf("=")+1, response.length());
    data[0] = response.substring(0, response.indexOf(',')).toFloat();
    response = response.substring(response.indexOf(',')+1, response.length());
    data[1] = response.substring(0, response.indexOf(',')).toFloat();
    response = response.substring(response.indexOf(',')+1, response.length());
    data[2] = response.substring(0, response.indexOf(',')).toFloat();
    response = response.substring(response.indexOf(',')+1, response.length());
    data[3] = response.substring(0, response.indexOf(',')).toFloat();
    response = response.substring(response.indexOf(',')+1, response.length());
    data[4] = response.substring(0, response.indexOf(',')).toFloat();
    response = response.substring(response.indexOf(',')+1, response.length());
   Serial.flush();
 }
 }
 if (data[0] != minVal && data[1] != minVal && data[3] != minVal && data[4] != minVal){
   String buff = "#";
   buff += WiFi.macAddress();
   buff += "\n";
   if (data0 != false){
    buff += "#S1#";
    buff += data[0];
    buff += "\n";
   }
   if (data1 != false){
    buff += "#S2#";
    buff += data[1];
    buff += "\n";
   }
   if (data2 != false){
    buff += "#S3#";
    buff += data[2];
    buff += "\n";
   }
   if (data3 != false){
    buff += "#S4#";
    buff += data[3];
    buff += "\n";
   }
   if (data4 != false){
    buff += "#S5#";
    buff += data[4];
    buff += "\n";
   }
   buff += "##\n";
   IPAddress NarodmonHost;
   WiFi.hostByName("narodmon.com", NarodmonHost);
   WiFiClient client;
   client.connect(NarodmonHost, 8283);
   client.print(buff);
   String responce = client.readStringUntil('\n');
   client.stop();
}   
}}

unsigned long sendDataToNarodMonPeriod=0;



void handleNarodMonSettings(){
  if(millis() - sendDataToNarodMonPeriod > narodMonUpdateTime*60000 && narodMonUpdateTime !=0){
    ESPFlash<bool> narodmonEnable(NARODMON_LOG_ENABLE);
    if(narodmonEnable.get()){
      ESPFlash<bool> narodmonSettings(NARODMON_LOG_SETTINGS);
      bool narodmonSettingsArr[5];
      narodmonSettings.getBackElements(narodmonSettingsArr,sizeof(narodmonSettingsArr));
      sendDataToNarodmon(narodmonSettingsArr[0],narodmonSettingsArr[1],narodmonSettingsArr[2],narodmonSettingsArr[3],narodmonSettingsArr[4]);
    }
    sendDataToNarodMonPeriod = millis();
  }  
}
void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  //handleData();
  handleNtpSettings();
   handleNarodMonSettings();
}
