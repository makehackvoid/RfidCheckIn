#include <string.h>

#include <Ethernet.h>
#include <SPI.h>
#include <HttpClient.h>
#include <EthernetClient.h>


byte mac[] = { 0xDE, 0xAD, 0xBE, 0xE0, 0xFE, 0xED };
char server[] = "morphia.mhv";

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
  
byte idle[2] = {0x01, 0x83};

#define rxPin 2
#define txPin 3
SoftwareSerial rfid( rxPin, txPin );

#include <LiquidCrystal.h>
LiquidCrystal lcd(4, 5, 6, 7, 8, 9);

void reset_lcd() {
    lcd.clear();
    lcd.print("Welcome to:");
    lcd.setCursor(4,1);
    lcd.print("MakeHackVoid");
    lcd.setCursor(0,3);    
    lcd.print("RFID checkin station");
}

void setup() 
{
  Serial.begin(115200);
  Serial.println("Starting the RFID Checkin thing");
  
  lcd.begin(20, 4);
  lcd.print("MakeHackVoid");
  lcd.setCursor(0,1);
  lcd.print("Waiting for an IP");
  
  while (Ethernet.begin(mac) != 1)
  {
    Serial.println("Error getting IP address via DHCP, trying again...");
    lcd.setCursor(0,2);
    lcd.print("No IP yet. Waiting.");
    delay(5000);
  }
  
  rfid.begin(9600);
  
  Serial.println("Ready to go!");
  lcd.setCursor(0,3);
  lcd.print("Ready to go!");
  delay(1000);
  reset_lcd();
}


byte* last_seen;
byte last_seen_length = 0;


int checkin(const char* asciiID) {
  EthernetClient c;
  HttpClient http(c);
  char* path;
  path = (char*)malloc(strlen("/rfid-checkin.php?id=") + strlen(asciiID) +1);
  if( !path ) {
    return -1;
  }
  
  sprintf(path,"/rfid-checkin.php?id=%s",asciiID);
  int err;
  if (err = http.get(server,path) != 0) {
    free(path);
    Serial.print("Connect failed: ");
    Serial.println(err);
    lcd.print("ERROR!");
    return -1;
  }
  
  free(path);
  
  if( err = http.responseStatusCode() < 0){          
    Serial.print("Getting response failed: ");
    Serial.println(err);
    lcd.print("ERROR!");
  }
  
  if( err = http.skipResponseHeaders() < 0 ) {
    Serial.print("Failed to skip response headers: ");
    Serial.println(err);
    lcd.print("ERROR!");
  }
     
  int bodyLen = http.contentLength();
  if(bodyLen > 255) {          
    Serial.print("got way more content than we expected: ");
    Serial.println(bodyLen);
    lcd.print("ERROR!");
  }
  
  char* body;
  body = (char*)malloc(bodyLen+1);
  body[bodyLen]=0;
  
  char* bodyP = body;
  
  unsigned long timeoutStart = millis();
  char next_byte;
  
  while (
    (http.connected() || http.available()) &&
    ((millis() - timeoutStart) < kNetworkTimeout) &&
    bodyLen > 0 
  ) {
      if (http.available()) {
          next_byte = http.read();
          *bodyP = next_byte;
          bodyP++;
          bodyLen--;
          timeoutStart = millis();
      } else {
          delay(kNetworkDelay);
      }
  }
  
  Serial.print(body);
  lcd.print(body);
  
  free(body);
  http.stop();   
  return http.contentLength();
}

void loop()
{
  int i;
  for(i=0;i<8;i++){rfid.write(query[i]);} // ask the rfid reader to return a card id
  while(rfid.read() != 0xAA){delay(10);} // wait for a start of text marker. TODO: include a timeout here
  while(rfid.available() < 2 ) {delay(10);} // wait until we can read the address and count bytes
  byte address = rfid.read(); // first byte is the address of the reader. 0x00 is the default broadcast address
  byte count = rfid.read(); // how many bytes in this packet?
  byte checksum = address ^ count; // the checksum is the xor of all non-start, non-stop bytes
  
  if( 0xff == address || 0xff == count ) {
    Serial.println("bad read on address or count");
    return;
  }
  
  byte *data;
  
  data = (byte*)malloc(count);
  if(!data ) {
    Serial.print("couldn't allocate memory: ");
    Serial.println(count);
    goto vacuumbuffer;
  }
  
  while(rfid.available() < count +1){delay(10);} // wait for the data and end of text message
  for(i = 0; i < count; ++i ) { // read the data and stuff it in our buffer
    data[i] = rfid.read();
    checksum ^= data[i];
  }
  
  if( rfid.read() != checksum ) {
    Serial.print("Checksum failed: ");
    Serial.println(checksum);
    goto freerfiddata;
  }
  
  if( rfid.read() != 0xbb ) {
    Serial.println("expected end of text marker");
    goto freerfiddata;
  }
  
  if( !(2 == count && memcmp(data,idle,2) == 0)) {
  Serial.print("Recived packet from: ");
    Serial.print(address);
    Serial.print(" of length: ");
    Serial.print(count);
    Serial.print(" => ");
    for(i =0; i < count; i++) {
      Serial.print(data[i],HEX);
      Serial.print(" ");
      char buffer[4];
      sprintf(buffer,"%02x ",data[i]);
    }
    Serial.println("");
    
    if(
      !(last_seen && last_seen_length == count && 
        memcmp( data, last_seen, count ) == 0)
    ) {
      if( last_seen ) { free(last_seen); }
      last_seen = (byte*)malloc(count);
      if( last_seen ) {
        last_seen_length = count;
        memcpy(last_seen, data, last_seen_length);
      } else {
        last_seen_length = 0;
      }
      
      char * asciiID;
      asciiID = (char*)malloc(last_seen_length *2 + 1);
      if(!asciiID) {
        Serial.print("couldn't allocate ram for asciiID");
        goto freerfiddata;
      }
      asciiID[0] = 0;
      
      for(i=0; i < count; i++){
        sprintf(asciiID,"%s%02x",asciiID,data[i]); 
      }
      
      lcd.clear();
      lcd.print("HI!");
      lcd.setCursor(0,1);
      lcd.print(asciiID);
      lcd.setCursor(0,2);
      checkin(asciiID);
      
      Serial.println(asciiID);
      free(asciiID);
      delay(1000);
    }
    
  } else if( last_seen ) { 
    reset_lcd();
    free(last_seen);
    last_seen = 0;
    last_seen_length = 0;
  }
    
freerfiddata:
  free(data);
vacuumbuffer:
  while(rfid.available() > 0){rfid.read();}
}