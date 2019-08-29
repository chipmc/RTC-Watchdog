/*
 * Project RTC-Watchdog
 * Description: A simple application for us to show the power of a varialbe external Real Time Clock to wake the Particle device
 * Use Case: The Particle device can set a wake up interval and go to sleep.  That is it!
 * Acknowledgement: This work builds on the DSRTCLib library by Sridhar Rajagopal https://github.com/sridharrajagopal
 * Author: Chip McClelland chip@seeinsights.com
 * Date: 8/28/19
 */

/*  EEPROM Mapping
    0 - Time Zone Setting
    1 - Sleep Test Enabled

    Demo Modes (to showcase RTC functionality) - set with Particle Functions
    0 - No sleeping for you!
    1 - Sleep for sleepSeconds and then the RTC wakes you
    2 - Repeating alarm example (every minute)
    3 - Houly repeating alarm example (long form)
*/



#include <DSRTCLib.h>                                                             // This is the library for the DS1339

int INT_PIN = D3;                                                                 // INTerrupt pin from the RTC. 

// Prototypes and System Mode calls
DS1339 RTC = DS1339(INT_PIN);                                                     // Instantiate the library - need to lose the int number input

const char releaseNumber[6] = "0.11";                                             // Displays the release on the console
int alarmMode = 0;                                                                // Sets the sleep mode (0-none, 1-defined seconds, 2-repeating interval)
int alarmSeconds = 10;                                                            // How long will we sleep in mode 1
volatile bool alarmFlag = false;

void setup()   {              
  Particle.variable("Release",releaseNumber);
  
  Particle.function("Set-Timezone",setTimeZone);                                  // Set the local timezone (-12,12)
  Particle.function("Alarm-Mode", setAlarmMode);                                  // Sets the sleep mode (0-none, 1-defined seconds, 2-repeating interval)
  Particle.function("Alarm-Seconds",setAlarmSeconds);                             // Allows us to sleep for a set nuber of seconds       

  Time.zone((float)EEPROM.read(0));                                               // Retain time zone setting through power cycle

  RTC.start();                                                                    // Ensure RTC oscillator is running, if not already  
  RTC.enable_interrupt();                                                         // This stops the 1 Hz squarewave on INT and allows us to set alarms

  if(!RTC.time_is_set() && Particle.connected()) {                                // set a time, if none set already...
    Particle.publish("Timer","Clock Not Set",PRIVATE);                            // With a coin cell battery, this should be a rare event
    syncRTCtoParticleTime();                                                      // We will set the DS1339 clock to the Particle time
  }
  
  if(!RTC.time_is_set() && Particle.connected()) {                                // If the oscillator is borked (or not really talking to the RTC), try to warn about it
    Particle.publish("Timer","Check the oscillator",PRIVATE);                     // With a coin cell battery, this should be a rare event
  }

  attachInterrupt(INT_PIN, pinInterruptHandler, FALLING);                         // Attach an interrupt so we can test the alarms
}

void loop()                     
{
  switch (alarmMode) {
    case 0: 
      timeCheckRTC();                                                                // Check the time each second and update the registers
      break;
    case 1:
      secondsTillAlarm();                                                         // Set an alarm to go off in alarmSeconds (which is set in a Particle function - default 10)
      alarmMode = 0;                                                              // Go back to default state                                                        
    break;
    case 2: 
      minuteRepeatingAlarm();                                                     // Sets an alarm to go off every minute
      alarmMode = 0;                                                              // Go back to default state
    break;
    case 3:
      hourlyRepeatingAlarm();                                                     // You guessed it...every hour
      alarmMode = 0;                                                              // back
    break;
  }  

  if (alarmFlag) {                                                                // Checks the Alarm flag set in the interrupt handler
    RTC.clear_interrupt();                                                        // tell RTC to clear its interrupt flag and drop the INT line
    if (Particle.connected()) {
      waitUntil(meterParticlePublish);
      Particle.publish("Alarm","Alarm detected", PRIVATE);
    }
    alarmFlag = false;                                                            // Reset the flag
  }
}

void timeCheckRTC() {
  static unsigned long lastTimeCheck = 0;
  static int currentMinute = 0;
  if (millis() >= lastTimeCheck + 1000) {                                         // Check the time every 10 seconds
    RTC.readTime();                                                               // Load the time into the registers
    if (RTC.getMinutes() != currentMinute) {
      publishRTCTime();                                                           // Publish the current time to the Particle console
      currentMinute = RTC.getMinutes();                                           // Update the current minute
    }
    lastTimeCheck = millis();                                                     // Update for next check
  }
}

void syncRTCtoParticleTime()                                                      // Calling this function sets the RTC clock to the Particle clock 
{    
    RTC.setSeconds(Time.second());
    RTC.setMinutes(Time.minute());
    RTC.setHours(Time.hour());
    RTC.setDays(Time.day());
    RTC.setMonths(Time.month());
    RTC.setYears(Time.year());
    RTC.writeTime();                                                              // Write time to the RTC registers
    RTC.readTime();                                                               // Load the current time registers on the RTC
    publishRTCTime();                                                             // Publish the time to the Particle console
}

void publishRTCTime() {                                                           // Publish the time in Particle's format - will print whatever time is in the register (Time or Alarm)
  if (Particle.connected()) {
    waitUntil(meterParticlePublish);
    Particle.publish("RTC Time",Time.timeStr(RTC.date_to_epoch_seconds()),PRIVATE);
  }
}

void secondsTillAlarm() {
  if (Particle.connected()) {
    char data[32];
    snprintf(data,sizeof(data),"Going to sleep for %i seconds",alarmSeconds);
    waitUntil(meterParticlePublish);
    Particle.publish("Alarm", data, PRIVATE);
  }
  RTC.alarmSeconds(alarmSeconds);
}

void minuteRepeatingAlarm()                                                       // Shows the repeating alarm frequency
{                                                       
  if (Particle.connected()) {
    waitUntil(meterParticlePublish);
    Particle.publish("Alarm","Repeating every minute",PRIVATE);                   // Message to let you know what is going on
  }             
  RTC.disable_interrupt();                                                        // Prevent an immediate interrupt since this alarm starts from now       
  RTC.setAlarmRepeat(EVERY_MINUTE);                                               // if alarming every minute, time registers larger than 1 second (hour, etc.) are don't-care
  RTC.writeAlarm();                                                               // Choices are: EVERY_SECOND, EVERY_MINUTE, EVERY_HOUR, EVERY_DAY, EVERY_WEEK, EVERY_MONTH
  RTC.clear_interrupt();                                                          // This prevents a alarm interrupt as soon as the alarm is set
  RTC.enable_interrupt();
}

void hourlyRepeatingAlarm()                                                       // Sets an alarm that goes off every hour (long form)
{
  if (Particle.connected()) {
    waitUntil(meterParticlePublish);
    Particle.publish("Alarm","Repeating every hour",PRIVATE);                     // Message to let you know what is going on
  }
  RTC.setAlarmRepeat(EVERY_HOUR);                                                 // There is no DS1339 setting for 'alarm once' - user must shut off the alarm after it goes off.

  RTC.setSeconds(0);                                                              // Here we can set an alarm frequency
  RTC.setMinutes(1);
  RTC.setHours(0);
  RTC.setDays(0);
  RTC.setMonths(0);
  RTC.setYears(0);

  RTC.writeAlarm();                                                               // Write the alarm to the registers
 }

// Helper Functions for Particle Functions and other utility functions

void pinInterruptHandler() {                                                      // Interrupt handler - note no i2c commands can be in here!
  alarmFlag = true;
}

int setTimeZone(String command)
{
  char * pEND;
  char data[256];
  time_t t = Time.now();
  int8_t tempTimeZoneOffset = strtol(command,&pEND,10);                           // Looks for the first integer and interprets it
  if ((tempTimeZoneOffset < -12) | (tempTimeZoneOffset > 12)) return 0;           // Make sure it falls in a valid range or send a "fail" result
  Time.zone((float)tempTimeZoneOffset);
  EEPROM.update(0,tempTimeZoneOffset);                                            // Store the new value in EEPROM Position 0
  snprintf(data, sizeof(data), "Time zone offset %i",tempTimeZoneOffset);
  if (Particle.connected()) {
      waitUntil(meterParticlePublish);
      Particle.publish("Time", data, PRIVATE);
      waitUntil(meterParticlePublish);
      Particle.publish("Particle Time",Time.timeStr(t), PRIVATE);
  }
  syncRTCtoParticleTime();
  return 1;
}

int setAlarmSeconds(String command)
{
  char * pEND;
  char data[256];
  unsigned long tempAlarmSeconds = strtol(command,&pEND,10);                      // Looks for the first integer and interprets it
  if (tempAlarmSeconds < 0) return 0;                                             // Make sure it falls in a valid range or send a "fail" result
  snprintf(data, sizeof(data), "Sleep Seconds set to %ld",tempAlarmSeconds);
  if (Particle.connected()) {
      waitUntil(meterParticlePublish);
      Particle.publish("Time", data, PRIVATE);
  }
  alarmSeconds = tempAlarmSeconds;
  return 1;
}

int setAlarmMode(String command)                                                  // This is where we can enable or disable the sleep interval for the device
{
  char * pEND;
  if (command != "1" && command != "0" && command != "2" && command != "3") return 0; // Before we begin, let's make sure we have a valid input

  alarmMode = strtol(command,&pEND,10);
  
  return 1;
}

bool meterParticlePublish(void)                                                   // Keep out publishes from getting rate limited
{
  static unsigned long lastPublish=0;                                             // Initialize and store value here
  if(millis() - lastPublish >= 1000) {                                            // The Particle platform rate limits to one publush per second
    lastPublish = millis();
    return 1;
  }
  else return 0;
}