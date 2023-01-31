#include <EEPROM.h>
#include <RTClib.h>

#include <DmxSimple.h>

#define BRIGHTNESS 255
#define TESTMODE_DELAY_DURATION 500

//{a,b,c,d,e,f,g}
uint8_t segtable[10][7] = {
  {1,1,1,1,1,1,0}, //0
  {0,1,1,0,0,0,0}, //1
  {1,1,0,1,1,0,1}, //2
  {1,1,1,1,0,0,1}, //3
  {0,1,1,0,0,1,1}, //4
  {1,0,1,1,0,1,1}, //5
  {1,0,1,1,1,1,1}, //6
  {1,1,1,0,0,0,0}, //7
  {1,1,1,1,1,1,1}, //8
  {1,1,1,0,0,1,1}  //9
};

//actual DMX address
uint8_t segmap[2][7] = {{2,3,4,5,6,7,8},{11,12,13,14,15,16,17}};

bool testmode = false;
uint8_t testmode_number = 00;

uint8_t days = 00;
uint8_t hours = 00;

DateTime now, uka;

RTC_DS3231 rtc;

//char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void setup() {
  
  //setup serial monitor
  Serial.begin(9600);
  Serial.setTimeout(20000);
  Serial.println("Boot!");

  //setup rtc
  //rtc.begin();
  
  while (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    delay(1000);
  }

  /*if (rtc.lostPower()) {
    Serial.println("RTC reset!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

    //while(1);
  }*/
    
  now = rtc.now();
  uka = readTarget(0);
  Serial.println("Read time:");
  printDateTime(&now);
  printDateTime(&uka);
   

  DmxSimple.usePin(3);
  DmxSimple.maxChannel(20);
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);

  Serial.println("Boot done");
  
}



void loop() {
  //Check time
  now = rtc.now();
  TimeSpan delta = TimeSpan(DateTime(uka)-DateTime(now));
  days = delta.days();
  hours = delta.hours();
  
  
  // Update segments with new data
  if(days > 0){
    setsegments((uint8_t)days/10,(uint8_t)days%10);
  }
  else{
    setsegments((uint8_t)hours/10,(uint8_t)hours%10);
  }

  
  //update terminal:
  if (Serial.available()){
    printMode();
  }

  delay(100);
}

void setsegments(uint8_t seg1, uint8_t seg2)
{
  if (seg1 >= 10){
    seg1 = 0;
  }
  for (uint8_t i = 0; i < 7; i++){
    if (segtable[seg1][i]){
      DmxSimple.write(segmap[0][i],BRIGHTNESS);
    } else {
      DmxSimple.write(segmap[0][i],0);
    }
  }  
   

  if (seg2 >= 10){
    seg2 = 0;
  }
  for (uint8_t i = 0; i < 7; i++){
    if (segtable[seg2][i]){
      DmxSimple.write(segmap[1][i],BRIGHTNESS);
    } else {
      DmxSimple.write(segmap[1][i],0);
    }
  }
}

void testMode() // Repeatedly asks user for number and sets segments to given number.
{
  testmode = true;
  while(testmode){
    Serial.println("Set testmode number (-1 to end testmode. -99 to count through all numbers): ");
    int16_t n = Serial.parseInt();

    if(n == -1){
      testmode = false;
    }
    else if(n == -99){  // Count from 0 through 99.
      for(int i = 0; i < 100; i++){
        setsegments((uint8_t)i/10, (uint8_t)i%10);
        delay(TESTMODE_DELAY_DURATION);
      }
    }
    else{
      setsegments((uint8_t)n/10,(uint8_t)n%10);
    }

  }
}

void printMode()
{
  Serial.println();
  switch (Serial.read()){ 
    case '?': 
      Serial.println("now: ");
      printDateTime(&now);
    
      Serial.println("uka: ");
      printDateTime(&uka);

      Serial.println("Days left:");
      Serial.print(days,DEC);
    break;
    case 'n':
      Serial.println("Set time:");
      now = DateTime(setDateTime());
      rtc.adjust(DateTime(now));
      Serial.println("time set!:");
      printDateTime(&now);
    break;
    case 'u':
      Serial.println("Set uka:");
      uka = setDateTime();
      writeTarget(0,&uka);
    break;
    case '\n':
    case '\r':
    break;
    case 'h':
      Serial.println("?: check data\nn: set RTC\nu: set uka\nt: testmode");
    break;
    case 't':
      testMode();
    break;
    default:
    Serial.println("'h' for help?");
    break;
  }
  
}

void printDateTime(DateTime *t)
{
  Serial.print(t->year(), DEC);
  Serial.print('-');
  Serial.print(t->month(), DEC);
  Serial.print('-');
  Serial.print(t->day(), DEC);
  //Serial.print(" (");
  //Serial.print(daysOfTheWeek[t.dayOfTheWeek()]);
  //Serial.print(") ");
  Serial.print(" ");
  Serial.print(t->hour(), DEC);
  Serial.print(':');
  Serial.print(t->minute(), DEC);
  Serial.print(':');
  Serial.print(t->second(), DEC);
  Serial.println();
}

DateTime setDateTime()
{
  Serial.println("YYYY:");
  uint16_t Y = Serial.parseInt();
  
  Serial.println("MM:");
  uint16_t M = Serial.parseInt();
  
  Serial.println("DD:");
  uint16_t D = Serial.parseInt();
  
  Serial.println("hh:");
  uint16_t h = Serial.parseInt();
  
  Serial.println("mm:");
  uint16_t m = Serial.parseInt();
  
  Serial.println("ss:");
  uint16_t s = Serial.parseInt();
  
  return DateTime(Y,M,D,h,m,s);
}

DateTime readTarget(uint8_t address)
{
  uint16_t Y = EEPROM.read(address + 0) + 2000;
  uint16_t M = EEPROM.read(address + 1);
  uint16_t D = EEPROM.read(address + 2);
  uint16_t h = EEPROM.read(address + 3);
  uint16_t m = EEPROM.read(address + 4);
  uint16_t s = EEPROM.read(address + 5);
  
  return DateTime(Y,M,D,h,m,s);
  
}


void writeTarget(uint8_t address, DateTime *uka)
{
  EEPROM.write(address + 0, (uint8_t)(uka->year() - 2000));
  EEPROM.write(address + 1, uka->month());
  EEPROM.write(address + 2, uka->day());
  EEPROM.write(address + 3, uka->hour());
  EEPROM.write(address + 4, uka->minute());
  EEPROM.write(address + 5, uka->second());
}

