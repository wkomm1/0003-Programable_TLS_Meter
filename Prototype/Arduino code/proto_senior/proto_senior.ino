#include <LiquidCrystal.h>
#include <dht11.h>

dht11 DHT11;
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  DHT11.attach(10);
  
  lcd.setCursor(0,0);
  lcd.print("Temp:  Lux:");
}

void loop() {
  float temp = DHT11.fahrenheit();
  int light = analogRead(2);  
  
  Serial.print("Temp:");
  Serial.print(temp);
  Serial.print("F  Light:");
  Serial.println(light);
  
  lcd.setCursor(0,1);
  lcd.print(temp);
  lcd.setCursor(4,1);
  lcd.print("F       ");
  lcd.setCursor(8,1);
  lcd.print(light);
  lcd.setCursor(12,1);
  delay(1000);
}
