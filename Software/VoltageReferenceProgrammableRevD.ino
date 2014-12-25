
// Include libraries this sketch will use
#include <ProfileTimer.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Rotary.h>
#include <Bounce.h>

// Defines
// DAC Maximum value
#define MAXVAL 4095
// Maximum number of offset table entries
#define MAX_OFFSETS 10

// Pins to drive Segment Shift Register
#define SR_DATA A0
#define SR_CLK A1

// Display digit pins
#define DIGIT_1 4
#define DIGIT_2 5
#define DIGIT_3 6
#define DIGIT_4 7

// Charge pump pin
#define CHARGE_PUMP 9

// UI Rotary Encoder w Button
#define BUTTON 8

// DAC Slave Select
#define DACSS 10

// This sets the delay time for frame rate
// The value is repeated for each of the digits on the display so 
//   frame rate = 1 / (4 * DIGIT_TIME(uS)) -> 125 Hz
#define DIGIT_TIME 2000

// Mode values
// send refVal to DAC after adding correction factor
#define CORRECTED true
// send raw refVal numbers to the DAC w/o correcting
#define RAW false

// Character generator for 7-segment displays
static uint8_t	chargen[] = {
// PC={Dp,G,C,D,A,B,F,E} Segments scrambled to ease PCB layout
  0b00111111,  // 0
  0b00100100,  // 1
  0b01011101,  // 2
  0b01111100,  // 3
  0b01100110,  // 4
  0b01111010,  // 5
  0b01111011,  // 6
  0b00101100,  // 7
  0b01111111,  // 8
  0b01111110,  // 9
  0b01101111,  // 10  "A"
  0b01110011,  // 11  "B"
  0b01011011,  // 12  "C"
  0b01110101,  // 13  "D"
  0b01011011,  // 14  "E"
  0b01001011,  // 15  "F"
};

// Instantiate objects used in this project
Rotary r = Rotary(2, 3);
Bounce button = Bounce(BUTTON, 5);

int16_t  refVal = 1000;
uint8_t CmdArray[7];
uint8_t CmdArrayIdx = 0;
int16_t num = refVal;
int DispBCD = 0;
boolean newVal = true;
boolean mode = CORRECTED;  // when mode = CORRECTED then offset value is used
boolean useFlag = false;  // when true uses downloaded value of offset
uint8_t sdata;
uint16_t DACreg;
int8_t offset = 0;


//-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|
void setup() {
  // Initialize USB interface
  Serial.begin(115200);
  delay(2000);
  Serial.println("Voltage Reference");  // set up the segment control pins

  pinMode(CHARGE_PUMP, OUTPUT);
  analogWrite(CHARGE_PUMP, 128);

  pinMode(BUTTON, INPUT_PULLUP);
  button.update();
  if(button.read() == LOW) {mode = RAW;} else {mode = CORRECTED;}

  pinMode(SR_DATA, OUTPUT);
  pinMode(SR_CLK, OUTPUT);
  
  // set up the digit control pins
  pinMode(DIGIT_1, OUTPUT);
  pinMode(DIGIT_2, OUTPUT);
  pinMode(DIGIT_3, OUTPUT);
  pinMode(DIGIT_4, OUTPUT);
  
  // set up the select control pin for the DAC
  // DAC select is active low, data is shifted in MSB first
  // on the falling edge of the clock
  pinMode(DACSS, OUTPUT);
  digitalWrite(DACSS, HIGH);
  SPI.begin();
  SPI.setDataMode(SPI_MODE1);  // set to falling edge clock
  SPI.setBitOrder(MSBFIRST);  // set to MSB first
}

//-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|
void loop() {
  
    // Increment by 500 if the button was pressed
  // If the increment result is more than MAX just set result to zero
  if(button.update()) {
    if(button.read() == HIGH) {
      refVal+=500;
      if(refVal>MAXVAL)refVal=1;
      newVal = true;
    }
  }
  
  // Check the rotary and increment or decrement if it was turned
  // If the increment result is more than MAX roll over to zero
  // If the decrement goes below zero value will roll over to 65535 (unsigned word)
  //   In that case set to result to MAX value
  unsigned char result = r.process();
  if (result) {
    result == DIR_CW ? refVal-- : refVal++;
    if(refVal<1)refVal=MAXVAL;
    if(refVal>MAXVAL)refVal=1;
    newVal = true;
  }

  while (Serial.available())
  {
    sdata = Serial.read();
    switch (sdata){
    case 35: // '#'  set DAC with correction factor
    case 33: // '!'  set DAC with no correction factor
    case 'W': // 'W' write offset values to EEPROM
    case 'R': // 'R' dump the offset table (mostly for debug)
    case 'U': // 'U' use a temporary offset
      CmdArrayIdx = 0;
      CmdArray[CmdArrayIdx++] = sdata;
      break;
    case 10: // LF
    case 13: // CR
      CmdArray[CmdArrayIdx++] = sdata;
      if(ParseCommand())
        newVal = true;
      break;
    default:
      CmdArray[CmdArrayIdx++] = sdata;
      if(CmdArrayIdx>6) CmdArrayIdx = 6;
    }
  }  

  // If the value has changed, update the display and send 
  // the new value to the DAC and the serial port
  if(newVal) {
    Serial.println((float)refVal/1000, 3);
    
    // Convert the 12-bit DAC value into a 16-bit value plus
    // an optional signed 8-bit correction factor.
    
    // First look up the correction factor
    if(!useFlag){
      int i=0;
      for(i=0;i<MAX_OFFSETS;i++){
        Serial.print((int16_t)(EEPROM.read(i*3)<<8 | EEPROM.read(i*3+1)));
        Serial.print(":");
        Serial.println((int8_t)EEPROM.read(i*3+2));
        if(refVal>=(int16_t)(EEPROM.read(i*3)<<8 | EEPROM.read(i*3+1))) {offset = EEPROM.read(i*3+2);}
        else break;
      }
    }
    
    // Next shift the 12-bit value up 4 bits and add in the correction factor if desired.
    if(mode == CORRECTED) {
     Serial.println(offset);
     DACreg = (refVal<<4) + offset;
    }
    else {
      DACreg = (refVal<<4);
    }

    // Now write the 16-bit value out to the DAC
    digitalWrite(DACSS, LOW);
    SPI.transfer((uint8_t) 0);
    SPI.transfer((uint8_t)((DACreg>>8) & 0xFF));
    SPI.transfer((uint8_t)(DACreg & 0xFF));
    digitalWrite(DACSS, HIGH);
    
    newVal = false;  // set the flag to false again after updating the DAC
  }
  
  // Convert the binary reference value into decimal digits
  // then multiplex the display using the digits
  num = refVal;
  DispBCD = 0;
  while(num >= 1000){DispBCD++; num-=1000;}
  ShiftOut(chargen[DispBCD] | 0x80);
  digitalWrite(DIGIT_1, LOW);
  delayMicroseconds(DIGIT_TIME);
  digitalWrite(DIGIT_1, HIGH);

  result = r.process();
  if (result) {
    result == DIR_CW ? refVal-- : refVal++;
    if(refVal<1)refVal=MAXVAL;
    if(refVal>MAXVAL)refVal=1;
    newVal = true;
  }

  DispBCD = 0;
  while(num >= 100) {DispBCD++; num-=100;}
  ShiftOut(chargen[DispBCD]);
  digitalWrite(DIGIT_2, LOW);
  delayMicroseconds(DIGIT_TIME);
  digitalWrite(DIGIT_2, HIGH);

  result = r.process();
  if (result) {
    result == DIR_CW ? refVal-- : refVal++;
    if(refVal<1)refVal=MAXVAL;
    if(refVal>MAXVAL)refVal=1;
    newVal = true;
  }

  DispBCD = 0;
  while(num >= 10)  {DispBCD++; num-=10;}
  ShiftOut(chargen[DispBCD]);
  digitalWrite(DIGIT_3, LOW);
  delayMicroseconds(DIGIT_TIME);
  digitalWrite(DIGIT_3, HIGH);

  result = r.process();
  if (result) {
    result == DIR_CW ? refVal-- : refVal++;
    if(refVal<1)refVal=MAXVAL;
    if(refVal>MAXVAL)refVal=1;
    newVal = true;
  }

  ShiftOut(chargen[num]);
  digitalWrite(DIGIT_4, LOW);
  delayMicroseconds(DIGIT_TIME);
  digitalWrite(DIGIT_4, HIGH);
}


//-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|
void ShiftOut(uint8_t data) {
  digitalWrite(SR_DATA, (data & 0x80));
  digitalWrite(SR_CLK, LOW);
  digitalWrite(SR_CLK, HIGH);
  digitalWrite(SR_DATA, (data & 0x40));
  digitalWrite(SR_CLK, LOW);
  digitalWrite(SR_CLK, HIGH);
  digitalWrite(SR_DATA, (data & 0x20));
  digitalWrite(SR_CLK, LOW);
  digitalWrite(SR_CLK, HIGH);
  digitalWrite(SR_DATA, (data & 0x10));
  digitalWrite(SR_CLK, LOW);
  digitalWrite(SR_CLK, HIGH);
  digitalWrite(SR_DATA, (data & 0x08));
  digitalWrite(SR_CLK, LOW);
  digitalWrite(SR_CLK, HIGH);
  digitalWrite(SR_DATA, (data & 0x04));
  digitalWrite(SR_CLK, LOW);
  digitalWrite(SR_CLK, HIGH);
  digitalWrite(SR_DATA, (data & 0x02));
  digitalWrite(SR_CLK, LOW);
  digitalWrite(SR_CLK, HIGH);
  digitalWrite(SR_DATA, (data & 0x01));
  digitalWrite(SR_CLK, LOW);
  digitalWrite(SR_CLK, HIGH);
}

//-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|=-=|
// Takes the data from the array and generates a new value for the Reference
boolean ParseCommand() {

  uint8_t i = 0;
  uint16_t j = 0;
  boolean retVal = false;

  if(CmdArray[5] != 10 && CmdArray[5] != 13) {
    // Error in the command array.  Clear it and start over.
    for(i=0;i<7;i++)CmdArray[i] = 0;
    CmdArrayIdx = 0;
    return false;
  }
  
  for(i=1;i<5;i++){
    j = j*10 + (CmdArray[i]-48); // '0'==48
  }
  if(j>MAXVAL) j = MAXVAL;
  // Serial.println(j);

  switch (CmdArray[0]) {
    case 33: // '!'
      mode = RAW;
      refVal = j;
      retVal = true;
      break;
    case 35: // '#'
      mode = CORRECTED;
      refVal = j;
      retVal = true;
      break;
    case 'W':
      if(mode == RAW) {
        EEPROM.write(refVal, (byte)j);
        // Serial.print(refVal);
        // Serial.print(":");
        // Serial.println((byte)j);
      }
      retVal = false;
      break;
    case 'U':
      offset = (byte)j;
      useFlag = true;
      retVal = true;
      break;
    case 'N':
      mode = CORRECTED;
      useFlag = false;
      retVal = true;
      break;
    case 'R':
      for(i=0; i<MAX_OFFSETS*3; i++){
        Serial.println(EEPROM.read(i), HEX);
      }
    default:
      retVal = false;
  }

  for(i=0;i<7;i++)CmdArray[i] = 0;
  CmdArrayIdx = 0;
  return retVal;
}

