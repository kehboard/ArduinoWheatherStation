#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "RTClib.h"
#include <Timezone.h>
#include "SparkFunBME280.h"
LiquidCrystal_I2C lcd(0x27,20,4);

#define EEPROM_SYSTEM_RESET 0x00

#define EEPROM_ADDRESS_REMOTE_SENSOR_UPDATE_PERIOD 0x01
#define EEPROM_ADDRESS_USE_DAYTIME 0x05
#define EEPROM_ADDRESS_TO_DAYTIME_WEEK 0x06
#define EEPROM_ADDRESS_TO_DAYTIME_DOW 0x07
#define EEPROM_ADDRESS_TO_DAYTIME_MONTH 0x08
#define EEPROM_ADDRESS_TO_DAYTIME_HOUR 0x09
#define EEPROM_ADDRESS_TO_DAYTIME_OFFSET_MINUTE 0x0A
#define EEPROM_ADDRESS_FROM_DAYTIME_WEEK 0x0C
#define EEPROM_ADDRESS_FROM_DAYTIME_DOW 0x0D
#define EEPROM_ADDRESS_FROM_DAYTIME_MONTH 0x10
#define EEPROM_ADDRESS_FROM_DAYTIME_HOUR 0x11
#define EEPROM_ADDRESS_FROM_DAYTIME_OFFSET_MINUTE 0x13


#define EEPROM_REMOTE_SENSOR_UPDATE_PERIOD_DEFAULT_VALUE 5000
#define EEPROM_USE_DAYTIME_DEFAULT_VALUE true
#define EEPROM_TO_DAYTIME_WEEK_DEFAULT_VALUE 0
#define EEPROM_TO_DAYTIME_DOW_DEFAULT_VALUE 1
#define EEPROM_TO_DAYTIME_MONTH_DEFAULT_VALUE 3
#define EEPROM_TO_DAYTIME_HOUR_DEFAULT_VALUE 3
#define EEPROM_TO_DAYTIME_OFFSET_MINUTE_DEFAULT_VALUE 180
#define EEPROM_FROM_DAYTIME_WEEK_DEFAULT_VALUE 0
#define EEPROM_FROM_DAYTIME_DOW_DEFAULT_VALUE 1
#define EEPROM_FROM_DAYTIME_MONTH_DEFAULT_VALUE 10
#define EEPROM_FROM_DAYTIME_HOUR_DEFAULT_VALUE 4
#define EEPROM_FROM_DAYTIME_OFFSET_MINUTE_DEFAULT_VALUE 120


void writeByteIntoEEPROM(int address, bool value){
  EEPROM.write(address, value);
}

byte readByteFromEEPROM(int address){
  return EEPROM.read(address);
}


void writeBooleanIntoEEPROM(int address, bool value){
  EEPROM.write(address, value);
}

bool readBooleanFromEEPROM(int address){
  return EEPROM.read(address);
}


void writeUnsignedIntIntoEEPROM(int address, unsigned int number)
{ 
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

unsigned int readUnsignedIntFromEEPROM(int address)
{ 
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

void writeUnsignedLongIntoEEPROM(int address, unsigned long number)
{ 
  EEPROM.write(address, (number >> 24) & 0xFF);
  EEPROM.write(address + 1, (number >> 16) & 0xFF);
  EEPROM.write(address + 2, (number >> 8) & 0xFF);
  EEPROM.write(address + 3, number & 0xFF);
}
unsigned long readUnsignedLongFromEEPROM(int address)
{
  return ((unsigned long)EEPROM.read(address) << 24) +
         ((unsigned long)EEPROM.read(address + 1) << 16) +
         ((unsigned long)EEPROM.read(address + 2) << 8) +
         (unsigned long)EEPROM.read(address + 3);
}


TimeChangeRule myDST = {"SUN", readByteFromEEPROM(EEPROM_ADDRESS_TO_DAYTIME_WEEK), readByteFromEEPROM(EEPROM_ADDRESS_TO_DAYTIME_DOW), readByteFromEEPROM(EEPROM_ADDRESS_TO_DAYTIME_MONTH), readUnsignedIntFromEEPROM(EEPROM_ADDRESS_TO_DAYTIME_HOUR), readUnsignedIntFromEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_OFFSET_MINUTE)};
TimeChangeRule mySTD = {"WIN", readByteFromEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_WEEK), readByteFromEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_DOW), readByteFromEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_MONTH), readUnsignedIntFromEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_HOUR), readUnsignedIntFromEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_OFFSET_MINUTE)};
Timezone tz(myDST, mySTD);
float remoteValue[2];
float localValue[3];
BME280 mySensorA;
RTC_DS3231 rtc;
SoftwareSerial mySerial(9, 8);
void setup(){
  Wire.begin();
  lcd.init();
  lcd.backlight();
  writeByteIntoEEPROM(EEPROM_SYSTEM_RESET, false);
  restoreDefaults();
  mySensorA.setI2CAddress(0x76);
  mySensorA.beginI2C();
  rtc.begin();
  Serial.begin(115200);
  mySerial.begin(9600);
  /*if(getRemoteValue()){
    Serial.println("remote ok");
    }
  else{
    Serial.println("remote fail");
    }*/
  getLocalValue();
}

void restoreDefaults(){
  if(!readBooleanFromEEPROM(EEPROM_SYSTEM_RESET)){
    //writeBooleanIntoEEPROM(EEPROM_SYSTEM_RESET,true);
    writeUnsignedLongIntoEEPROM(EEPROM_ADDRESS_REMOTE_SENSOR_UPDATE_PERIOD ,EEPROM_REMOTE_SENSOR_UPDATE_PERIOD_DEFAULT_VALUE);
    writeBooleanIntoEEPROM(EEPROM_ADDRESS_USE_DAYTIME ,EEPROM_USE_DAYTIME_DEFAULT_VALUE);
    writeByteIntoEEPROM(EEPROM_ADDRESS_TO_DAYTIME_WEEK ,EEPROM_TO_DAYTIME_WEEK_DEFAULT_VALUE);
    writeByteIntoEEPROM(EEPROM_ADDRESS_TO_DAYTIME_DOW ,EEPROM_TO_DAYTIME_DOW_DEFAULT_VALUE);
    writeByteIntoEEPROM(EEPROM_ADDRESS_TO_DAYTIME_MONTH ,EEPROM_TO_DAYTIME_MONTH_DEFAULT_VALUE);
    writeByteIntoEEPROM(EEPROM_ADDRESS_TO_DAYTIME_HOUR ,EEPROM_TO_DAYTIME_HOUR_DEFAULT_VALUE);
    writeUnsignedIntIntoEEPROM(EEPROM_ADDRESS_TO_DAYTIME_OFFSET_MINUTE ,EEPROM_FROM_DAYTIME_WEEK_DEFAULT_VALUE);
    writeByteIntoEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_WEEK ,EEPROM_FROM_DAYTIME_WEEK_DEFAULT_VALUE);
    writeByteIntoEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_DOW ,EEPROM_FROM_DAYTIME_DOW_DEFAULT_VALUE);
    writeByteIntoEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_MONTH ,EEPROM_FROM_DAYTIME_MONTH_DEFAULT_VALUE);
    writeByteIntoEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_HOUR ,EEPROM_FROM_DAYTIME_HOUR_DEFAULT_VALUE);
    writeUnsignedIntIntoEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_OFFSET_MINUTE ,EEPROM_FROM_DAYTIME_OFFSET_MINUTE_DEFAULT_VALUE);

  }
}

bool getRemoteValue(){
  mySerial.println("AT&DATA?");
  delay(1000);
  if(mySerial.available()){
    String response = mySerial.readStringUntil('\n');
    if (response.substring(0, response.indexOf("=")) == "&DATA"){
      remoteValue[0] = response.substring(response.indexOf('=')+1, response.indexOf(',')).toFloat();
      remoteValue[1] = response.substring(response.indexOf(',')+1, response.length()).toFloat();
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
unsigned long sensorUpdateMillis = 0;
unsigned long displayUpdateMillis = 0;

void loop(){
  handleSerial();
  handleDisplay();
  handleSensors();
}

void handleSensors(){
  if(millis() - sensorUpdateMillis > readUnsignedLongFromEEPROM(EEPROM_ADDRESS_REMOTE_SENSOR_UPDATE_PERIOD)){
    sensorUpdateMillis = millis();
    getRemoteValue();
    getLocalValue();
  }
}

void getLocalValue(){
  localValue[0] = mySensorA.readTempC();
  localValue[1] = mySensorA.readFloatHumidity();
  localValue[2] = mySensorA.readFloatPressure();
}

void handleDisplay(){
  if (millis()- displayUpdateMillis > 1000){
    time_t sec = tz.toLocal(rtc.now().unixtime());
    char buf[20];
    sprintf(buf, "%.2d:%.2d:%.2d  %.2d/%.2d/%.4d", hour(sec),minute(sec),second(sec),day(sec),month(sec),year(sec));
    lcd.setCursor(0,0);
    lcd.print(buf);
    lcd.setCursor(0,1);
    lcd.print("In:  T:");
    lcd.print(localValue[0], 0);
    lcd.print("C H:");
    lcd.print(localValue[1], 0);
    lcd.print("%");
    lcd.setCursor(0,2);
    lcd.print("Out: T:");
    lcd.print(remoteValue[0], 0);
    lcd.print("C H:");
    lcd.print(remoteValue[1], 0);
    lcd.print("%");
    lcd.setCursor(0,3);
    lcd.print("P:");
    lcd.print(localValue[2]/133.32,0);
    lcd.print("mmHg");
  }
}


void handleSerial(){
if(Serial.available()){
    String cmd = Serial.readStringUntil('\n');
    if(cmd.startsWith("A")){
      if(cmd.substring(2, cmd.length()).length() > 0)
      {
        if (cmd.substring(3, cmd.indexOf("?")) == "GETALL"){
          atGetAll();
          }
        if (cmd.substring(3, cmd.indexOf("?")) == "LOCUTIME"){
          atGetTimeLocalUnix();
          }
        if (cmd.substring(3, cmd.indexOf("=")) == "LOCUTIME"){
          atSetTimeLocalUnix(strtol(cmd.substring(cmd.indexOf("=")+1, cmd.length()).c_str(),NULL,0));
          }
        if (cmd.substring(3, cmd.indexOf("?")) == "DTIME"){
          cmd = cmd.substring(cmd.indexOf("=")+1, cmd.length());
          bool useDaytime = cmd.substring(0, cmd.indexOf(',')) == "1" ? true : false ;
          cmd = cmd.substring(cmd.indexOf(',')+1, cmd.length());
          byte toDaytimeWeek = (byte) cmd.substring(0, cmd.indexOf(',')).toInt();
          cmd = cmd.substring(cmd.indexOf(',')+1, cmd.length());
          byte toDaytimeDoW = (byte) cmd.substring(0, cmd.indexOf(',')).toInt();
          cmd = cmd.substring(cmd.indexOf(',')+1, cmd.length());
          byte toDaytimeMonth = (byte) cmd.substring(0, cmd.indexOf(',')).toInt();
          cmd = cmd.substring(cmd.indexOf(',')+1, cmd.length());
          byte toDaytimeHour = (byte) cmd.substring(0, cmd.indexOf(',')).toInt();
          cmd = cmd.substring(cmd.indexOf(',')+1, cmd.length());
          unsigned int toDaytimeOffsetMinute = cmd.substring(0, cmd.indexOf(',')).toInt();
          cmd = cmd.substring(cmd.indexOf(',')+1, cmd.length());
          byte fromDaytimeWeek = (byte) cmd.substring(0, cmd.indexOf(',')).toInt();
          cmd = cmd.substring(cmd.indexOf(',')+1, cmd.length());
          byte fromDaytimeDoW = (byte) cmd.substring(0, cmd.indexOf(',')).toInt();
          cmd = cmd.substring(cmd.indexOf(',')+1, cmd.length());
          byte fromDaytimeMonth = (byte) cmd.substring(0, cmd.indexOf(',')).toInt();
          cmd = cmd.substring(cmd.indexOf(',')+1, cmd.length());
          byte fromDaytimeHour = (byte) cmd.substring(0, cmd.indexOf(',')).toInt();
          cmd = cmd.substring(cmd.indexOf(',')+1, cmd.length());
          unsigned int fromDaytimeOffsetMinute = cmd.substring(0, cmd.indexOf(',')).toInt();
          cmd = cmd.substring(cmd.indexOf(',')+1, cmd.length());
          atSetDayTimeSettings(useDaytime, toDaytimeWeek, toDaytimeDoW, toDaytimeMonth, toDaytimeHour, 
          toDaytimeOffsetMinute, fromDaytimeWeek, fromDaytimeDoW, fromDaytimeMonth, fromDaytimeHour, fromDaytimeOffsetMinute);
          }
        if (cmd.substring(3, cmd.indexOf("?")) == "DTIME"){
          atGetDayTimeSettings();
          }
        if (cmd.substring(3, cmd.indexOf("?")) == "DEFAULT"){
          restoreDefaults();
          resetMe();
          }
        if (cmd.substring(3, cmd.indexOf("?")) == "RESETME"){
          resetMe();
          }
      }  
    }
  }  
}

void resetMe(){
  pinMode(12, OUTPUT);
  digitalWrite(12,1);
} 

void atGetTimeLocalUnix(){
  DateTime now = rtc.now();
  Serial.print(F("AT&LOCUTIME="));
  Serial.println(now.unixtime());
}

void atSetTimeLocalUnix(uint32_t unixtime){
  rtc.adjust(DateTime(year(unixtime), month(unixtime), day(unixtime), hour(unixtime), minute(unixtime), second(unixtime)));
}

void atSetDayTimeSettings(bool useDaytime, byte toDaytimeWeek, byte toDaytimeDoW, byte toDaytimeMonth, byte toDaytimeHour, unsigned int toDaytimeOffsetMinute, byte fromDaytimeWeek, byte fromDaytimeDoW, byte fromDaytimeMonth, byte fromDaytimeHour, unsigned int fromDaytimeOffsetMinute){
  writeBooleanIntoEEPROM(EEPROM_ADDRESS_USE_DAYTIME ,useDaytime);
  writeByteIntoEEPROM(EEPROM_ADDRESS_TO_DAYTIME_WEEK ,toDaytimeWeek);
  writeByteIntoEEPROM(EEPROM_ADDRESS_TO_DAYTIME_DOW ,toDaytimeDoW);
  writeByteIntoEEPROM(EEPROM_ADDRESS_TO_DAYTIME_MONTH ,toDaytimeMonth);
  writeByteIntoEEPROM(EEPROM_ADDRESS_TO_DAYTIME_HOUR ,toDaytimeHour);
  writeUnsignedIntIntoEEPROM(EEPROM_ADDRESS_TO_DAYTIME_OFFSET_MINUTE ,toDaytimeOffsetMinute);
  writeByteIntoEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_WEEK ,fromDaytimeWeek);
  writeByteIntoEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_DOW ,fromDaytimeDoW);
  writeByteIntoEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_MONTH ,fromDaytimeMonth);
  writeByteIntoEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_HOUR ,fromDaytimeOffsetMinute);
  writeUnsignedIntIntoEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_OFFSET_MINUTE ,fromDaytimeOffsetMinute);
  TimeChangeRule myLDST = {"SUN", readByteFromEEPROM(EEPROM_ADDRESS_TO_DAYTIME_WEEK), readByteFromEEPROM(EEPROM_ADDRESS_TO_DAYTIME_DOW), readByteFromEEPROM(EEPROM_ADDRESS_TO_DAYTIME_MONTH), readUnsignedIntFromEEPROM(EEPROM_ADDRESS_TO_DAYTIME_HOUR), readUnsignedIntFromEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_OFFSET_MINUTE)};
  TimeChangeRule myLSTD = {"WIN", readByteFromEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_WEEK), readByteFromEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_DOW), readByteFromEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_MONTH), readUnsignedIntFromEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_HOUR), readUnsignedIntFromEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_OFFSET_MINUTE)};
  myDST = myLDST;
  mySTD = myLSTD;
  Timezone tzl(myDST, mySTD);
  tz = tzl;
  Serial.println("OK");
}

void atGetDayTimeSettings(){
  Serial.print("AT&DTIME=");
  Serial.print(readByteFromEEPROM(EEPROM_ADDRESS_USE_DAYTIME));
  Serial.print(",");
  Serial.print(readByteFromEEPROM(EEPROM_ADDRESS_TO_DAYTIME_WEEK)); 
  Serial.print(",");
  Serial.print(readByteFromEEPROM(EEPROM_ADDRESS_TO_DAYTIME_DOW));
  Serial.print(",");
  Serial.print(readByteFromEEPROM(EEPROM_ADDRESS_TO_DAYTIME_MONTH));
  Serial.print(",");
  Serial.print(readUnsignedIntFromEEPROM(EEPROM_ADDRESS_TO_DAYTIME_HOUR));
  Serial.print(",");
  Serial.print(readUnsignedIntFromEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_OFFSET_MINUTE));
  Serial.print(",");
  Serial.print(readByteFromEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_WEEK));
  Serial.print(",");
  Serial.print(readByteFromEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_DOW));
  Serial.print(",");
  Serial.print(readByteFromEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_MONTH));
  Serial.print(",");
  Serial.print(readUnsignedIntFromEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_HOUR));
  Serial.print(",");
  Serial.println(readUnsignedIntFromEEPROM(EEPROM_ADDRESS_FROM_DAYTIME_OFFSET_MINUTE));

}



void atGetAll(){
  Serial.print("AT&GETALL=");
  Serial.print(localValue[0]);
  Serial.print(",");
  Serial.print(localValue[1]);
  Serial.print(",");
  Serial.print(localValue[2]/133.32, 2);
  Serial.print(",");
  Serial.print(remoteValue[0]);
  Serial.print(",");
  Serial.print(remoteValue[1]);
  
  
  Serial.print(",");
  DateTime now = rtc.now();
  Serial.println(now.unixtime());
  
}
