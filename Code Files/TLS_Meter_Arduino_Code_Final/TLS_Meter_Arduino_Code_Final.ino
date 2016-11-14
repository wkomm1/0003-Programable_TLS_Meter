// ========================================================================================================================
// ++                                    Programable Temperature Light and Sound Meter                                   ++
// ++                                      designed & programed by: William Kommritz                                     ++
// ++                                       Contact Email Address: wkomm1@gmail.com                                      ++
// ++                                         Naugatuck Valley  Community College                                        ++
// ++                                          PROJECTS - EET H294 - 01   (2014)                                         ++
// ========================================================================================================================

// Include the required libraries
#include <EEPROM.h>  // EEPROM library
#include <LiquidCrystal.h>  // LCD library

// Determine LCD I/O configuration
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);  // create "lcd" object

// I/O pin assignment constants
const int tempSensorPin = A0;  // input pin for TMP36
const int lightSensorPin = A1;  // input pin for LDR
const int soundSensorPin = A2;  // input pin for the microphone circuit
const int rightButton = A5;  // pin attached to the right button
const int menuButton = A4;  // pin attached to the menu button
const int leftButton = A3;  // pin attached to the left button
const int tempOutputPin = 8;  // temperature indcator LED pin
const int lightOutputPin = 9;  // light indcator LED pin
const int soundOutputPin = 10;  // sound indcator LED pin

// Micilanious configuration constants
const int lcdRefreshRate = 750;  // refresh rate for the LCD screen
const int debounceDelay = 200;  // debounce delay for the pushbuttons
const int tempCalibration = 0;  // used to adjust the temperature reading in degrees
const int tempRange[] = {-13, 212};  // TMP36 ADC range (-13, 212 for F -25, 100 for C)
const int soundSampleTime = 20;  // sample rate for microphone in ms 20 = 50Hz

// 16-bit EEPROM words used for configuration settings
word tempOutputType;  // temperature output type, eeprom type 0
word lightOutputType;  // light output type, eeprom type 1
word soundOutputType;  // sound output type, eeprom type 2
word tempOutputValue;  // temperature output value, eeprom type 3
word lightOutputValue;  // light output value, eeprom type 4
word soundOutputValue;  // sound output value, eeprom type 5
word extraFunction;  // extra function type, 0 for disabled, eeprom type 6
word vuSensitivity;  // vu meter sensitivity value lower is more, eeprom type 7

// Working values, these values are changed a lot by the program
int tempData;  // storage for temperature data from the sensor, used by getSensorData()
int lightData;  // storage for light data from the sensor, used by getSensorData()
int soundData;  // storage for sound data from the sensor, used by getSensorData()
int currentSelection;  // for determining the menu entry !!VERY IMPORTANT!!
int extraFunctionPin;  // temp/light LED pin used by extraFunction()
int extraFunctionData;  // temp/light ADC data used by extraFunction()
int extraFunctionType;  // temp/light output type used by extraFunction()
int extraFunctionValue;  // temp/light output value used by extraFunction()
unsigned long lcdMillis;  // for determining the LCD refresh rate
unsigned long buttonMillis;  // for determining last button press
unsigned long soundMillis;  // for determining the microphone sample rate
unsigned long extraFunctionCount;  // calculate count per min/sec used by extraFunction()
unsigned long extraFunctionCurrent;  // current cycle count used by extraFunction()
unsigned long extraFunctionTotal;  // total counts used by extraFunction()
boolean changed = false;  // used in menus to determine if LCD should be refreshed

// This array contains the strings used for the major menu entries
const char* menuData[3][4] = {  // pointer array has 3, 4 dimention elements
  {"TEMP MENU", "LIGHT MENU", "SOUND MENU", "EXTRA FUNCTIONS"},  // menu selections
  {"NEVER", "ALWAYS", "ABOVE VALUE", "BELOW VALUE"},  // LED output types
  {"DISABLED", "LIGHT COUNTER", "SOUND COUNTER", "VU METER"}};  // special functions


// ========================================================================================================================
// =======================================================================================> Setup Loop Function <==========
// ========================================================================================================================
void setup() {  // standard arduino setup loop
  Serial.begin(115200);  // start the searial port at 115200 baud rate
  lcd.begin(16,2);  // initilize the LCD screen as a 16 character by 2 line display
  lcd.clear();  // clear the LCD screen

  pinMode(tempOutputPin, OUTPUT);  // set the temp output LED pin as an output
  pinMode(lightOutputPin, OUTPUT);  // set the light output LED pin as an output
  pinMode(soundOutputPin, OUTPUT);  // set the sound output LED pin as an output
  digitalWrite(tempOutputPin, LOW);  // turn off the temp LED
  digitalWrite(lightOutputPin, LOW);  // turn off the light LED
  digitalWrite(soundOutputPin, LOW);  // turn off the sound LED

  // Resets special EEPROM words on startup if left and right buttons are pressed
  buttonMillis = 0;  // bypases the debounce delay for button presses
  if(checkButtons(3)) {  // do this if left and right buttons are pressed
    eepromWrite(0, 0);  // overwrite current EEPROM value for tempOutputType
    eepromWrite(1, 0);  // overwrite current EEPROM value for lightOutputType
    eepromWrite(2, 0);  // overwrite current EEPROM value for soundOutputType
    eepromWrite(3, 70);  // overwrite current EEPROM value for tempOutputValue
    eepromWrite(4, 500);  // overwrite current EEPROM value for lightOutputValue
    eepromWrite(5, 50);  // overwrite current EEPROM value for soundOutputValue
    eepromWrite(6, 0);  // overwrite current EEPROM value for extraFunction
    eepromWrite(7, 250);  // overwrite current EEPROM value for vuSensitivity
  }  // end of EEPROM overwrite

  // Restore special EEPROM words from values in EEPROM
  tempOutputType = eepromRead(0);  // set temperature output type to EEPROM type 0
  lightOutputType = eepromRead(1);  // set light output type to EEPROM type 1
  soundOutputType = eepromRead(2);  // set sound output type to EEPROM type 2
  tempOutputValue = eepromRead(3);  // set temperature output value to EEPROM type 3
  lightOutputValue = eepromRead(4);  // set light output value to EEPROM type 4
  soundOutputValue = eepromRead(5);  // set sound output value to EEPROM type 5
  extraFunction = eepromRead(6);  // set the extra function type to EEPROM type 6
  vuSensitivity = eepromRead(7);  // set the vu meter sensitivity to EEPROM type 6

  // Print out the characters that do not change on the LCD screen
  // LCD was cleared at the begining when it was initilized
  if(extraFunction==0) {  // only do this if extraFunction is disabled, 0
    lcd.print("TEMP  LDR#  MIC#");  // print the first line (for sensor value designation)
  }
  else if(extraFunction<3) {  // do if extraFunction is not disabled and not vu meter
    lcd.print("COUNT:");  // print "COUNT:" from current position
    lcd.setCursor(0,1);  // go to the first position on second line
    lcd.print("PM:");  // print "PS:" from current position
    lcd.setCursor(9,1);  // go to 10th position on second line
    lcd.print("PS:");  // print "PS:" from current position
  }  // end of LCD update section

  // Clear the button and LCD times
  buttonMillis = 0;  // reset the button millis to bypass debounce delay
  lcdMillis = 0;  // LCD will refresh the next time it needs to
}  // end of setup loop


// ========================================================================================================================
// =========================================================================================> Main Loop Function <=========
// ========================================================================================================================
void loop() {  // standard arduino main loop
  // Check for main menu entry and run the extra function if enabled
  if(checkButtons(1)) {  // check to see if the menu button is pressed
    displayMenu(0);  // go to the main menu, 0
  }
  while(extraFunction != 0) {  // latch into the extra function if it is not disabled
    extraFunctions();  // run the extra function object
  }

  // Turn on the sensors LED if needed see sensorLED object for parameters
  sensorLED(tempOutputPin, getSensorData(0), tempOutputType, tempOutputValue);  // sensorLED object
  sensorLED(lightOutputPin, getSensorData(1), lightOutputType, lightOutputValue);  // sensorLED object
  sensorLED(soundOutputPin, getSensorData(2), soundOutputType, soundOutputValue);  // sensorLED object

  // Refresh the LCD screen with the sensor values if enough time has elapsed or if bypased
  if(lcdMillis == 0 || (millis() - lcdMillis) > lcdRefreshRate) {  // see above line
    lcd.setCursor(0,1);  // go to second line first position
    lcd.print("                ");  // print 16 spaces to clear the line
    lcd.setCursor(0,1);  // go to second line first position
    lcd.print(tempData);  // print the temperature data
    lcd.print("F");  // print an F for fahrenheit change to C if using centigrade
    lcd.setCursor(7,1);  // go to the eight position on the second line
    lcd.print(lightData);  // print the light data
    lcd.setCursor(13,1);  // got to the 14th position on the second line
    lcd.print(soundData);  // print the sound data
    lcdMillis = millis();  // refresh the LCD time variable with the current time
  }  // end of LCD refresh

  // Write data to the serial port
  Serial.print(millis());  // print the current time to the serial port
  Serial.print(",");  // print a comma, for easy csv export
  Serial.print(tempData);  // print the temperature data
  Serial.print(",");  // print a comma, for easy csv export
  Serial.print(lightData);  // print the light data
  Serial.print(",");  // print a comma, for easy csv export
  Serial.println(soundData);  // print the sound data
  Serial.flush();  // wait for the serial port to finish sending information
}  // end of main loop


// ========================================================================================================================
// ======================================================================================> Display Menu Function <=========
// ========================================================================================================================
void displayMenu (int type) {  // display menu
  // type 0 is menu selection (temp, light, sound and extra functions)
  // type 1-3 is temp/light/sound menu
  // type 4 is for extra functions menu
  changed = true;  // refresh the screen next time it needs to
  currentSelection = 0;  // reset the current menu selection to 0
  digitalWrite(tempOutputPin, LOW);  // turn off the temp LED
  digitalWrite(lightOutputPin, LOW);  // turn off the light LED
  digitalWrite(soundOutputPin, LOW);  // turn off the sound LED
  while (currentSelection != 10) {  // latch into current menu, delatch when currentSelection is 10
    if (changed) {  // refresh the current selection is changed is true
      lcd.clear();  // clear the lcd screen
      if(type==0) {  // do if the type is 0
        lcd.print(menuData[0][currentSelection]);  // print the currentSelection element of first dimention
      }
      else if(type==4) {  // do if type is 4
        lcd.print("EXTRA FUNCTION:");  // print "EXTRAF FUNCTION:" to the lcd
        lcd.setCursor(0,1);  // go to the second line first position
        lcd.print(menuData[2][currentSelection]);  // print the currentSelection element of third dimention
      }
      else {  // do if type is not 0 or 4
        lcd.print("LED OUTPUT HIGH");  // print "LED OUTPUT HIGH" to the lcd
        lcd.setCursor(0,1);  // go to the second line first position
        lcd.print(menuData[1][currentSelection]);  // print the currentSelection element of second dimention
      }
      changed = false;  // reser the changed flag
    }

    // Check for button presses
    if(currentSelection > 0 && checkButtons(0)) {  // do if not 0 and check left button
      currentSelection--;  // decrement currentSelection
      changed = true;  // allow a screen update next time it needs to
    }
    else if(currentSelection < 3 && checkButtons(2)) {  // do if not 3 and check right button
      currentSelection++;  // increment currentSelection
      changed = true;  // allow a screen update next time it needs to
    }
    else if(type == 0 && checkButtons(1)) {  // do if in main menu and if menu button has been pressed
      displayMenu(currentSelection + 1);  // re enter the menu with the currentSelection + 1
      currentSelection = 10;  // exit the while loop and menu
    }
    else if(type == 1 && checkButtons(1)) {  // do if in temperature menu and if menu button has been pressed
      tempOutputType = currentSelection;  // set tempOutputType to currentSelections value
      tempOutputValue = setValue(tempRange[0], tempRange[1], 0, tempOutputType, tempOutputValue);  // get the output value
      eepromWrite(0, tempOutputType);  // write the data to eprom
      eepromWrite(3, tempOutputValue);  // write the data to eprom
      currentSelection = 10;  // exit the while loop and menu
    }
    else if(type == 2 && checkButtons(1)) {  // do if in light menu and if menu button has been pressed
      lightOutputType = currentSelection;  // set lightOutputType to currentSelections value
      lightOutputValue = setValue(0, 999, 1, lightOutputType, lightOutputValue);  // get the output value
      eepromWrite(1, lightOutputType);  // write the data to eprom
      eepromWrite(4, lightOutputValue);  // write the data to eprom
      currentSelection = 10;  // exit the while loop and menu
    }
    else if(type == 3 && checkButtons(1)) {  // do if in sound menu and if menu button has been pressed
      soundOutputType = currentSelection;  // set soundOutputType to currentSelections value
      soundOutputValue = setValue(0, 999, 2, soundOutputType, soundOutputValue);  // get the output value
      eepromWrite(2, soundOutputType);  // write the data to eprom
      eepromWrite(5, soundOutputValue);  // write the data to eprom
      currentSelection = 10;  // exit the while loop and menu
    }
    else if(type == 4 && checkButtons(1)) {  // do if in extra functions menu and if menu button has been pressed
      extraFunctionTotal = 0;  // reset the extraFunction's total count value
      extraFunction = currentSelection;  // set extraFunction to the current value of currentSelection
      eepromWrite(6, extraFunction);  // write the data to eprom
      currentSelection = 10;  // exit the while loop and menu
    }
  } //end of while loop

  // Clear and refresh lcd screen stuff
  lcd.clear();  // clear the lcd screen
  lcdMillis = 0;  // LCD will refresh the next time it needs to
  if(extraFunction==0) {  // only do this if extraFunction is disabled, 0
    lcd.print("TEMP  LDR#  MIC#");  // print the first line (for sensor value designation)
  }
  else if(extraFunction<3) {  // do if extraFunction is not disabled and not vu meter
    lcd.print("COUNT:");  // print "COUNT:" from current position
    lcd.setCursor(0,1);  // go to the first position on second line
    lcd.print("PM:");  // print "PS:" from current position
    lcd.setCursor(9,1);  // go to 10th position on second line
    lcd.print("PS:");  // print "PS:" from current position
  }  // end of LCD update section
}  // end of display menu


// ========================================================================================================================
// =========================================================================================> Set Value Function <=========
// ========================================================================================================================
int setValue(int valueMin, int valueMax, int sensor, int outputType, int currentValue) {  // setValue
  // valueMin - minimum allowed value
  // valueMax - maximum allowed value
  // sensor - sensor used to display now value
  // outputType - if its 0 or 1 it will bypass and return the current value
  // currentValue - the value to start with
  if(outputType > 1) {  // do if outputType is not 0 or 1
    lcd.clear();  // clear the lcd screen
    lcd.print("SET VALUE TENS");  // print "SET VALUE TENS" on the lcd screen
    lcd.setCursor(9,1);  // go to position 10 on line 2
    lcd.print("NOW:");  // print "NOW:" on the lcd
    for(int stepValue=10; stepValue>=1; stepValue-=9) {  // stepValue is 10 first time ans 1 second time
      lcdMillis = 0;  // reset the lcd time so it can refresh next time it needs to
      currentSelection = 0;  // reset current selection to 0, used to latch wile loop
      changed = true;  // set the changed flag, updates the current value
      while(currentSelection != 10) {  // do forever when not equal to 10

          // Update the lcd screen
        if(changed) {  // do if changed is true
          lcd.setCursor(0,1);  // go to first position on line 2
          lcd.print(currentValue); // print the current value
          lcd.print("  ");  // print two spaces to overwrite any trailing numbers
          changed = false;  // reset the changed flag
        }
        if(lcdMillis == 0 || (millis() - lcdMillis) > lcdRefreshRate) {  // do if lcd time is 0 or greater than refresh rate
          lcd.setCursor(13,1);  // goto position 14 on line 2
          lcd.print(getSensorData(sensor));  // refresh the current sensor value
          lcd.print("  ");  // print two spaces to overwrite any trailing numbers
          lcdMillis = millis();  // refresh the lcd time with the current time
        }

        // Check the buttons
        if(currentValue <= (valueMax - stepValue) && checkButtons(2)) {  // do if blow maxValue and right button is pressed
          currentValue += stepValue;  // add the step value to the current value
          changed = true;  // set the changed flag so lcd can refresh
        }
        else if(currentValue >= (valueMin + stepValue) && checkButtons(0)) {  // do if above minValue and left button is pressed
          currentValue -= stepValue;  // subtract the step value to the current value
          changed = true;  // set the changed flag so lcd can refresh
        }
        else if(stepValue == 10 && checkButtons(1)) {  // do if step is 10 and menu button is pressed
          lcd.setCursor(10,0);  // goto position 11 on line one of lcd
          lcd.print("ONES");  // print "ONES" on top of tens
          currentSelection = 10;  // set currentSelection to 10 to exit loop
        }
        else if(stepValue == 1 && checkButtons(1)) {  // do if step is 1 and menu button is pressed
          lcd.clear();  // clear the lcd screen
          currentSelection = 10;  // set currentSelection to 10 to exit loop
        }
      }  // end of while loop
    }  //end of for loop
  }
  return currentValue;  // return current value
}  // end of setValue


// ========================================================================================================================
// =================================================================================> EEPROM Read/Write Function <=========
// ========================================================================================================================
// eepromRead and eepromWrite seperate a 16 bit variable and put it in 2 eeprom locations.
// The higher byte of of the variable is in the first location and the lower is in the next location.
// Type 0 = tempOutputType                        Type 4 = lightOutputValue
// Type 1 = lightOutputType                       Type 5 = soundOutputValue
// Type 2 = soundOutputType                       Type 6 = extraFunction
// Type 3 = tempOutputValue                       Type 7 = vuSensitivity
int eepromRead(int type) {  // read variable type from its location and return it
  int value;  // initilize temporary integer variavle for the 16-bit value
  byte hiByte;  // initilize temporary byte variavle for the upper byte
  byte loByte;  // initilize temporary byte variavle for the lower byte
  hiByte = EEPROM.read(type*2);  // set the high byte to the first eprom location
  loByte = EEPROM.read(type*2+1);  // set the low byte to the first eprom location
  value = ((hiByte << 8) | loByte);  // shift the upper byte to left 8 and or the low byte to the lower byte
  return value;  // return the 16-bit value
}  // end of eeprom read
void eepromWrite(int type, word value) {  // write the type and its value to eeprom
  EEPROM.write(type*2, highByte(value));  // write the upper byte to the first eeprom location
  EEPROM.write(type*2+1, lowByte(value));  // write the lower byte to the second eeprom location
}  // end of eeprom write


// ========================================================================================================================
// ===================================================================================> Get Sensor Data function <=========
// ========================================================================================================================
// this function will overwrite tha value of <sensor>Data variable it also returns the value
int getSensorData(int sensor) {  // getSensorData
  int data;  // initilize a temporary data variable
  int soundMax = 0;  // initilize a temporary soundMax variable
  int soundMin = 1023;  // initilize a temporary soundMin variable

  // Choes the correct sensor to check based on sensor
  if(sensor == 0) {  // do if its the temp sensor
    data = map(analogRead(tempSensorPin), 51, 307, tempRange[0], tempRange[1])+tempCalibration;  // map the value to data
    tempData = data;  // set tempData to the value of data
  }
  else if(sensor == 1) {  // do if its the light sensor
    data = map(analogRead(lightSensorPin), 0, 1023, 0, 999);  // map the value to data
    lightData = data;  // set lightData to the value of data
  }
  else if(sensor == 2) {  // do if its the sound sensor
    soundMillis = millis();  // reset the sound millis variable to the current time
    while(millis() - soundMillis < soundSampleTime) {  // do while sound time is lees than sample rate
      data = analogRead(soundSensorPin);  // read the sound sensor value into data
      if(data > soundMax) {  // do if the data is larger than the current max
        soundMax = data;  // set the current max value to data's value
      }
      else if(data < soundMin) {  // do if the data is smaller than the current max
        soundMin = data;  // set the current min value to data's value
      }
    }  // end of while loop
    data = map((soundMax - soundMin), 0, 1023, 0, 999);  // map the difference in values to data
    soundData = data;  // set soundData to the value of data
  }
  return data;  // return the current value of data
}  // end of getSensorData


// ========================================================================================================================
//=================================================================================> Sensor LED Output Function <==========
// ========================================================================================================================
void sensorLED(int LED_Pin, int sensorData, int type, int outputValue) {  // lights the leds if needed
  if(type == 0) {  // do if the output type is always off
    digitalWrite(LED_Pin, LOW);  // turn off the led
  }
  else if(type == 1) {  // do if output type is always on
    digitalWrite(LED_Pin, HIGH);  // turn on the led
  }
  else if(type == 2 && sensorData <= outputValue) {  // do if below value
    digitalWrite(LED_Pin, LOW);  // turn off the led
  }
  else if(type == 2 && sensorData > outputValue) {  // do if above value
    digitalWrite(LED_Pin, HIGH);  // turn on the led
  }
  else if(type == 3 && sensorData >= outputValue) {  // do if above value
    digitalWrite(LED_Pin, LOW);  // turn off the led
  }
  else if(type == 3 && sensorData < outputValue) {  // do if below value
    digitalWrite(LED_Pin, HIGH);  // turn on the led
  }
}  // end of sensorLED


// ========================================================================================================================
// ==================================================================================> Extra Functions Function <==========
// ========================================================================================================================
void extraFunctions() {  // extra functions, 1 is light counter, 2 is sound counter, 3 is vu meter
  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Extra Functions Menu <<<<<<<<<<
  if(extraFunction != 3 && checkButtons(1)) {  // do if menu button is pressed and not in vu meter mode
    changed = true;  // set changed flag so lcd can refresh
    currentSelection = 2;  // set the current selection to 2, skip "never" and "always"
    lcd.clear();  // clear the lcd screen
    lcd.print("COUNT UP WHEN");  // print "COUNT UP WHEN" to the lcd screen
    digitalWrite(tempOutputPin, LOW);  // turn off the temp LED
    digitalWrite(lightOutputPin, LOW);  // turn off the light LED
    digitalWrite(soundOutputPin, LOW);  // turn off the sound LED
    while (currentSelection != 10) {  // do while not 10
      if (changed) {  // do if changed is true
        lcd.setCursor(0,1);  // goto position 1 line 2 of the lcd screen
        lcd.print(menuData[1][currentSelection]);  // print the currentSelection of dimention 1 of menuData
        changed = false;  // reset the changed flag
      }
      if(currentSelection == 3 && checkButtons(0)) {  // do if current slection is 3 and when right button is pressed
        currentSelection--;  // decrement currentSelection
        changed = true;  // set changed flag so lcd can refresh
      }
      else if(currentSelection == 2 && checkButtons(2)) {  // do if current slection is 2 and when left button is pressed
        currentSelection++;  // increment currentSelection
        changed = true;  // set changed flag so lcd can refresh
      }
      else if(extraFunction == 1 && checkButtons(1)) {  // do if light counter and menu button is pressed
        lightOutputType = currentSelection;  // set the light output type to the current selection
        lightOutputValue = setValue(0, 999, 1, lightOutputType, lightOutputValue);  // set the output value
        eepromWrite(1, lightOutputType);  // write the data to the eeprom
        eepromWrite(4, lightOutputValue);  // write the data to the eeprom
        currentSelection = 10;  // set current selection to 10, exiting the wile loop and menu
      }
      else if(extraFunction == 2 && checkButtons(1)) {  // do if sound counter and menu button is pressed
        soundOutputType = currentSelection;  // set the sound output type to the current selection
        soundOutputValue = setValue(0, 999, 2, soundOutputType, soundOutputValue);  // set the output value
        eepromWrite(2, soundOutputType);  // write the data to the eeprom
        eepromWrite(5, soundOutputValue);  // write the data to the eeprom
        currentSelection = 10;  // set current selection to 10, exiting the wile loop and menu
      }
    } //end of while loop

    // Refresh the lcd screen
    lcd.clear();  // clear the lcd screen
    lcd.print("COUNT:");  // print "COUNT:" from current position
    lcd.setCursor(0,1);  // go to the first position on second line
    lcd.print("PM:");  // print "PS:" from current position
    lcd.setCursor(9,1);  // go to 10th position on second line
    lcd.print("PS:");  // print "PS:" from current position
    lcdMillis = 0;  // reset the lcd time to 0
  }  // end of menu section of extraFunction

  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Light/Sound Counter Mode <<<<<<<<<<
  if(extraFunction != 3) {  // do if not in vu meter mode
    if(extraFunction == 1) {  // do if in light counter mode
      extraFunctionPin = lightOutputPin;  // set the extra function pin to light pin
      extraFunctionType = lightOutputType;  // set the extra function type to light type
      extraFunctionValue = lightOutputValue;  // set the extra function value to light value
    }
    else {  // do if not in light counter mode (sound counter mode)
      extraFunctionPin = soundOutputPin;  // set the extra function pin to sound pin
      extraFunctionType = soundOutputType;  // set the extra function type to sound type
      extraFunctionValue = soundOutputValue;  // set the extra function value to sound value
    }

    // Refresh the lcd screen
    if(lcdMillis == 0 || (millis() - lcdMillis) > lcdRefreshRate) {  // do if lcd time is reset or greater than refresh rate
      lcd.setCursor(6,0);  // goto position 7 line 1 of the lcd
      lcd.print("          ");  // print blank spaces to clear leftover characters
      lcd.setCursor(6,0);  // goto position 7 line 1 of the lcd
      lcd.print(extraFunctionTotal);  // print the total count to the lcd, this is a 32 bit value
      lcd.setCursor(3,1);  // goto position 4 line 2 of the lcd
      lcd.print("      ");  // print blank spaces to clear leftover characters
      lcd.setCursor(3,1);  // goto position 4 line 2 of the lcd
      extraFunctionCount = extraFunctionCurrent * 60000 / (millis() - lcdMillis);  // calculate counts per minute
      lcd.print(extraFunctionCount);  // print counts per minute
      lcd.setCursor(12,1);  // goto position 13 line 2 of the lcd
      lcd.print("    ");  // print blank spaces to clear leftover characters
      lcd.setCursor(12,1);  // goto position 13 line 2 of the lcd
      extraFunctionCount /= 60;  // calculate counts per second
      lcd.print(extraFunctionCount);  // print counts per second
      extraFunctionCurrent = 0;  // reset the current count value
      lcdMillis = millis();  // set the lcd time to the current time
    }  // end of lcd refresh

    // determine if triggered, light the led, and increment the counts
    extraFunctionData = getSensorData(extraFunction);  // refresh the sensor data
    if(extraFunctionData > extraFunctionValue && extraFunctionType == 2 && changed == false) {  // trigger on rising edge
      changed = true;  // set the changed flag true to indicate a trigger condition
      extraFunctionTotal++;  // increment the total count
      extraFunctionCurrent++;  // increment the current count
      digitalWrite(extraFunctionPin, HIGH);  // light the led to indicate a triggered condition
    }
    if(extraFunctionData < extraFunctionValue && extraFunctionType == 2 && changed == true) {  // trigger has to reset
      changed = false;  // set the changed flag false to indicate a non-trigger condition
      digitalWrite(extraFunctionPin, LOW);  // turn off the led to indicate a non-tirggered condition
    }
    if(extraFunctionData < extraFunctionValue && extraFunctionType == 3 && changed == false) {  // trigger on falling edge
      changed = true;  // set the changed flag true to indicate a trigger condition
      extraFunctionTotal++;  // increment the total count
      extraFunctionCurrent++;  // increment the current count
      digitalWrite(extraFunctionPin, HIGH);  // light the led to indicate a triggered condition
    }
    if(extraFunctionData > extraFunctionValue && extraFunctionType == 3 && changed == true) {  // trigger has to reset
      changed = false;  // set the changed flag false to indicate a non-trigger condition
      digitalWrite(extraFunctionPin, LOW);  // turn off the led to indicate a non-tirggered condition
    }
  }  // end of light/sound counter

  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> VU Meter Mode <<<<<<<<<<
  else {  // do if not in light or sound counter mode (vu meter mode)
    // vuSensitivity determines the sensitivity
    // extraFunctionCurrent determines the digits to light
    if (checkButtons(1)) {  // do if menu button is pressed
      vuSensitivity = setValue(0, 999, 2, 2, vuSensitivity);  // set the sensitivity of the vu meter
    }

    // Refresh the lcd screen
    if(lcdMillis == 0 || (millis() - lcdMillis) > 100) {  // do if lcd time is 0 or greater than 100
      lcd.clear();  // clear the lcd screen
      extraFunctionCurrent = map(getSensorData(2), 0, vuSensitivity, 0, 16);  // refresh the data and map it between 0 and 15
      for(int i = 0; i < extraFunctionCurrent; i++) {  // do this the amount of times as the maped earlier
        lcd.write(255);  // print a dark block to the lcd screen
      }
      lcd.setCursor(0,1);  // goto position 1 line 2 of the lcd screen
      for(int i = 0; i < extraFunctionCurrent; i++) {  // do this the amount of times as the maped earlier
        lcd.write(255);  // print a dark block to the lcd screen
      }
      lcdMillis = millis();  // set the lcd time to the current time
    }
  }

  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Exit Extra Functions <<<<<<<<<<
  if(checkButtons(3)) {  // do if left and right buttons are pressed
    extraFunction = 0;  // disable the extra function
    eepromWrite(6, 0);  // overwrite current EEPROM value for extraFunction
    lcdMillis = 0;  // reset the lcd time
    lcd.clear();  // clear the lcd screen
    lcd.print("TEMP  LDR#  MIC#");  // print the first line (for sensor value designation)
  }  // end of exit extra functions
}  // end of extra functions


// ========================================================================================================================
// =====================================================================================> Button Check Function <==========
// ========================================================================================================================
boolean checkButtons(int button) {  // check the buttons
  if(buttonMillis == 0 || (millis() - buttonMillis) > debounceDelay) {  // do if the button time is 0 or greater than debouce delay
    if(button==0 && digitalRead(leftButton) == HIGH) {  // do if 0 and left button is pressed
      buttonMillis = millis();  // set the button time to the current time
      return true;  // return a boolean true
    }
    else if(button==1 && digitalRead(menuButton) == HIGH) {  // do if 1 and menu button is pressed
      buttonMillis = millis();  // set the button time to the current time
      return true;  // return a boolean true
    }
    else if(button==2 && digitalRead(rightButton) == HIGH) {  // do if 2 and right button is pressed
      buttonMillis = millis();  // set the button time to the current time
      return true;  // return a boolean true
    }
    else if(button==3 && digitalRead(leftButton) == HIGH && digitalRead(rightButton) == HIGH) {  // do if 3 and both buttons are pressed
      buttonMillis = millis();  // set the button time to the current time
      return true;  // return a boolean true
    }
  }
  return false;  // return a boolean false
}  // end of button check

