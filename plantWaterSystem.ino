#include <OneWire.h>

#include <DallasTemperature.h>
/*-----( Import needed libraries )-----*/
#include <Wire.h>  // Comes with Arduino IDE
// Get the LCD I2C Library here: 
// https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads
// Move any other LCD libraries to another folder or delete them
// See Library "Docs" folder for possible commands etc.
#include <LiquidCrystal_I2C.h>

/*-----( Declare Constants )-----*/
/*-----( Declare objects )-----*/
// set the LCD address to 0x27 for a 20 chars 4 line display
// Set the pins on the I2C chip used for LCD connections:
//                    addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

/*-----( Declare Variables )-----*/

#define PUMP_FLOW_L_H 300
#define PUMP_FLOW_ML_SEC ( PUMP_FLOW_L_H *(5/18))

#define PULSE_AMOUNT_ML 100


#define SECONDS_IN_DAY (60*60*24)
#define SECONDS_IN_HOUR (60*60)
#define SECONDS_IN_MINUTE (60)
#define SECONDS_TO_MILLI(SEC) ((SEC)*1000)
#define MILLI_TO_SEC(MILLI) ((MILLI)/1000) 
#define MILLI_TO_MINUTES(MILLI) ((MILLI)/(1000*SECONDS_IN_MINUTE))
#define MILLI_TO_HOURS(MILLI) ((MILLI)/(1000*SECONDS_IN_HOUR))
#define MILLI_TO_DAYS(MILLI) ((MILLI)/(1000*SECONDS_IN_DAY))

#define PAUSE_BETWEEN_PULSES (1000*2)
 
int relay = 4;

const unsigned long WaterPeriodSec = (6*86400); //in seconds (6 days)

unsigned long TimeRemainingMilliSec = 0;  // in milli seconds

unsigned long Timestamp = 0;

bool NeedWater = false;

const int WaterAmountInML = 600;

/**
 * This function converts amount of water in mL to seconds of pump work.
 */
float milliliterToSec(float amountInML) {
  float pumpFlowMlSec = (((float)(PUMP_FLOW_L_H)*5)/18);
  return (amountInML/(pumpFlowMlSec));
}

unsigned long milliToSec(unsigned long milli) {
  return (milli/1000);
}

unsigned long milliToMinutes(unsigned long milli) {
  return (milli/(1000*60));
}

unsigned long milliToHours(unsigned long milli) {
  return (milli/(1000*60*60));
}
unsigned long milliToDays(unsigned long milli) {
  return (milli/(1000*60*60*24));
}

unsigned long secondsToMilli(unsigned long seconds) {
  return (seconds*1000);
}

unsigned long secondsToMilli(float seconds) {
  return (seconds*1000);
}

/**
 * This function gets amount of water in mL, converts it to seconds 
 * of pump work, and activate the pump accordingly.
 */
void PumpFlow(float amountInML) {
  float flowTimeInSec = milliliterToSec(amountInML);
  unsigned long flowTimeMilli = secondsToMilli(flowTimeInSec);
  lcd.setCursor(0, 1);
  lcd.print("PUMP ON         ");
  digitalWrite(relay, LOW);
  delay(flowTimeMilli);
  digitalWrite(relay, HIGH);
  lcd.setCursor(0, 1);
  lcd.print("PUMP OFF        ");
}

/**
 * This function gets amount of water in mL, and splits it to small pulses
 * of water for the plant. It acctually operates the pump here.
 */
void giveWaterToPlant(float amountInML) {
  lcd.setCursor(0,0);  //Start at character 0 on line 0
  lcd.print("Watering Plant:");
  lcd.setCursor(0, 1);

  //  Water plant in a series of small pulses of water
  while(amountInML > PULSE_AMOUNT_ML) {
    //  do a pulse:
    PumpFlow(PULSE_AMOUNT_ML);
    amountInML -= PULSE_AMOUNT_ML;
    delay(PAUSE_BETWEEN_PULSES);
  }

  //check if last pulse is needed:
  if(amountInML > 0) {
    PumpFlow(amountInML);
  }

  //reset timestamp:
  Timestamp = millis();
}

/**
 * This function counts the time between water cycles, and sets flag when its time to water
 * the plant.
 */
void countdown() {
  unsigned long currentTime = millis();
  unsigned long offset = 0;
  if (currentTime < Timestamp) {
    //  an overflow occurred
    offset = 0xffffffff - Timestamp;
    offset += currentTime;
  } else {
    offset = currentTime - Timestamp;
  }
  if (offset >= TimeRemainingMilliSec) {
    //  time to water the plant, and reset the remaining time to the next cycle
    NeedWater = true;
    TimeRemainingMilliSec = secondsToMilli(WaterPeriodSec);
  } else {
    //  decrease offset time
    TimeRemainingMilliSec -= offset;
  }
  //  update the timestamp to be the last check point
  Timestamp = currentTime;
}

/**
 * This function updates the screed to show the remaining time to water the plant.
 */
void updateScreen() {
  lcd.setCursor(0,0);  //Start at character 0 on line 0
  lcd.print("Water Plant In: ");
  lcd.setCursor(0, 1);
  unsigned long days = (((milliToSec(TimeRemainingMilliSec)/60)/60)/24);
  unsigned int hours = ((milliToSec(TimeRemainingMilliSec) / SECONDS_IN_HOUR) % 24);
  unsigned int minutes = ((milliToSec(TimeRemainingMilliSec) / SECONDS_IN_MINUTE) % 60);
  unsigned int secs = (milliToSec(TimeRemainingMilliSec) % 60);
  String daysStr = String(days);
  String hoursStr = String(hours);
  String minutesStr = String(minutes);
  String label = String(daysStr + "d " + hoursStr + "h " + minutesStr + "min " + String(secs) + "s ");
  lcd.print(label);
}

void setup(void)
{
  // start serial port
  Serial.begin(9600);

  lcd.begin(16,2);   // initialize the lcd for 16 chars 2 lines, turn on backlight
  
  
  lcd.backlight(); // finish with backlight on  


// Wait and then tell user they can start the Serial Monitor and type in characters to
// Display. (Set Serial Monitor option to "No Line Ending")
  lcd.clear();
  lcd.setCursor(0,0); //Start at character 0 on line 0
  lcd.print("Plant Water Sys."); 
  
  pinMode(relay, OUTPUT);
  digitalWrite(relay,HIGH);

  // initialize variables:
  TimeRemainingMilliSec = secondsToMilli(WaterPeriodSec);
  Timestamp = millis();
}
 
 
void loop(void) {
  //  wait the time between waters
  countdown();
  if(NeedWater) {
    giveWaterToPlant(WaterAmountInML);
    NeedWater = false;
  }
  updateScreen();
}



