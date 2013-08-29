#include <string.h>
#include <Ethernet.h>
#include <SPI.h>
#include <HttpClient.h>
#include <EthernetClient.h>


byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char server[] = "morphia.mhv";

IPAddress ip(10,0,0,51);
EthernetClient client;

const int kNetworkTimeout = 30*1000;
const int kNetworkDelay = 1000;

#include <SoftwareSerial.h>

byte query[8] = {
  0xAA, 0x00, 0x03, 0x25, 0x26, 0x00, 0x00, 0xBB};
byte led1[8] = {
  0xAA, 0x00, 0x03, 0x87, 0x18, 0x0a, 0x96, 0xBB};
byte led2[8] = {
  0xAA, 0x00, 0x03, 0x88, 0x18, 0x0a, 0x99, 0xBB};
byte buzzer[8] = {
  0xAA, 0x00, 0x03, 0x89, 0x18, 0x0a, 0x98, 0xBB};
  
byte idle[7] = {
  0xAA, 0x00, 0x02, 0x01, 0x83, 0x80, 0xBB};

#define rxPin 2
#define txPin 3
SoftwareSerial rfid( rxPin, txPin );

#include <LiquidCrystal.h>
LiquidCrystal lcd(4, 5, 6, 7, 8, 9);

void setup() 
{
  // set up the LCD's number of columns and rows: 
  lcd.begin(20, 4);
  // Print a message to the LCD.
  lcd.print("hello, world!");
  
  Serial.begin(115200);
  Serial.println("Starting the RFID Checkin thing");
  while (Ethernet.begin(mac) != 1)
  {
    Serial.println("Error getting IP address via DHCP, trying again...");
    delay(15000);
  }
  Serial.println("We got an IP");
  delay(1000);

  rfid.begin(9600);
  Serial.println("BOOM!");
  
  lcd.clear();
  lcd.print("We're all good to go!");
}
byte val;
int i;

byte last_seen[7] = {0,0,0,0,0,0,0};

void loop()
{
  lcd.clear();
  lcd.print("Scan an RFID card!");
  
  for (i=0 ; i<8 ; i++){rfid.write(query[i]);}
  delay(50);
 
  while(!rfid.available()>=7){}
  
  byte buffer[7];
  for(i = 0; i < 7; ++i ) {buffer[i]=rfid.read();}
  
  if( memcmp(last_seen,buffer,7) !=0 ) {
    memcpy(last_seen,buffer,sizeof(last_seen));
    if( memcmp(buffer,idle,7) != 0) {
      char asciiID[15];
      sprintf(asciiID, "%02x%02x%02x%02x%02x%02x%02x",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7]);
      
      lcd.clear();
      lcd.print("HI: ");
      lcd.print(asciiID);
      
      Serial.println(asciiID);

      
      lcd.setCursor(0,1);
      lcd.print("Checking in...");      
      lcd.setCursor(0,2);
      
      EthernetClient c;
      HttpClient http(c);
      char path[40];
      sprintf(path,"/rfid-checkin.php?id=%s",asciiID);
      
      int err = http.get(server,path);
      if (err == 0)
      {    
        err = http.responseStatusCode();
        if (err >= 0) {
          err = http.skipResponseHeaders();
         
          if (err >= 0) {
            int bodyLen = http.contentLength();
          
            unsigned long timeoutStart = millis();
            char c;
            while (
              (http.connected() || http.available()) &&
              ((millis() - timeoutStart) < kNetworkTimeout) &&
              bodyLen > 0 
            ) {
                if (http.available()) {
                    c = http.read();
                    Serial.print(c);
                    lcd.print(c);
                    bodyLen--;
                    timeoutStart = millis();
                } else {
                    delay(kNetworkDelay);
                }
            }
            delay(1000);
          } else {
            Serial.print("Failed to skip response headers: ");
            Serial.println(err);
            lcd.print("ERROR!");
          }
        } else {    
          Serial.print("Getting response failed: ");
          Serial.println(err);
          lcd.print("ERROR!");
        }
      } else {
        Serial.print("Connect failed: ");
        Serial.println(err);
        lcd.print("ERROR!");
      }
      http.stop();
    }
  }
  
  while(rfid.available()>0){rfid.read();}
}