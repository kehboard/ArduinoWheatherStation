#include <SHT21.h>  // include SHT21 library
#include <EEPROM.h>
//#include <avr/sleep.h>
#include <SimpleSleep.h>
SimpleSleep Sleep;
#define EEPROMADDERSS 0

SHT21 sht; 

int wakePin = 2;                 // pin used for waking up
int sleepStatus = 0;             // variable to store a request for sleep
int count = 0;       

float temp;   // variable to store temperature
float humidity; // variable to store hemidity

void setup() {
  Wire.begin();   // begin Wire(I2C)
  Serial.begin(9600); // begin Serial
  //pinMode(wakePin, INPUT);
  //pinMode(13, OUTPUT);
  //attachInterrupt(0, wakeUpNow, LOW);
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

//void wakeUpNow()        // here the interrupt is handled after wakeup
//{
//  digitalWrite(13,1);
  // execute code here after wake-up before returning to the loop() function
  // timers and code using timers (serial.print and more...) will not work here.
  // we don't really need to execute any special functions here, since we
  // just want the thing to wake up
//}

//void sleepNow()         // here we put the arduino to sleep
//{
    /* Now is the time to set the sleep mode. In the Atmega8 datasheet
     * https://www.atmel.com/dyn/resources/prod_documents/doc2486.pdf on page 35
     * there is a list of sleep modes which explains which clocks and
     * wake up sources are available in which sleep mode.
     *
     * In the avr/sleep.h file, the call names of these sleep modes are to be found:
     *
     * The 5 different modes are:
     *     SLEEP_MODE_IDLE         -the least power savings
     *     SLEEP_MODE_ADC
     *     SLEEP_MODE_PWR_SAVE
     *     SLEEP_MODE_STANDBY
     *     SLEEP_MODE_PWR_DOWN     -the most power savings
     *
     * For now, we want as much power savings as possible, so we
     * choose the according
     * sleep mode: SLEEP_MODE_PWR_DOWN
     *
     */  
//    digitalWrite(13,0);
//    set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here
    
//    sleep_enable();          // enables the sleep bit in the mcucr register
                             // so sleep is possible. just a safety pin
 
    /* Now it is time to enable an interrupt. We do it here so an
     * accidentally pushed interrupt button doesn't interrupt
     * our running program. if you want to be able to run
     * interrupt code besides the sleep function, place it in
     * setup() for example.
     *
     * In the function call attachInterrupt(A, B, C)
     * A   can be either 0 or 1 for interrupts on pin 2 or 3.  
     *
     * B   Name of a function you want to execute at interrupt for A.
     *
     * C   Trigger mode of the interrupt pin. can be:
     *             LOW        a low level triggers
     *             CHANGE     a change in level triggers
     *             RISING     a rising edge of a level triggers
     *             FALLING    a falling edge of a level triggers
     *
     * In all but the IDLE sleep modes only LOW can be used.
     */
 
//    attachInterrupt(0,wakeUpNow, LOW); // use interrupt 0 (pin 2) and run function
                                       // wakeUpNow when pin 2 gets LOW
 
 //   sleep_mode();            // here the device is actually put to sleep!!
                             // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
 
  //  sleep_disable();         // first thing after waking from sleep:
                             // disable sleep...
  //  detachInterrupt(0);      // disables interrupt 0 on pin 2 so the
                             // wakeUpNow code will not be executed
                             // during normal running time.
 
//}

void loop() {
  while(!Serial.available())
  {
    Sleep.idle();
  }
  if(Serial.available()){
    //int val = Serial.read();
    //Serial.println(val);
    String cmd = Serial.readStringUntil('\n');
    if(cmd.startsWith("A")){
      if(cmd.substring(2, cmd.length()).length() > 0)
      {
        if (cmd.substring(3, cmd.indexOf("?")) == "DATA"){
            float tmp = sht.getTemperature();
            float hum = sht.getHumidity();
            Serial.print("&DATA=");
            Serial.print(tmp);
            Serial.print(",");
            Serial.println(hum);
          }
      }
      else if (cmd.substring(2, cmd.length()).length() == 0)
      {
        Serial.println("OK");
      }  
    }
    delay(100);
}
}
