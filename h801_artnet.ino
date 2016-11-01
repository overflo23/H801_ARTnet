/*

v0.1 works, but needs cleanup ..
universe and offset from setup gui are not used yet

ARTnet support for the H801 Led controller

You need the following libs to compile this  (Arduino 1.6.4)
 - ArtnetWifi   (https://github.com/rstephan/ArtnetWifi)
 - WifiManager  (https://github.com/tzapu/WiFiManager)
 - Arduino JSON (https://github.com/bblanchon/ArduinoJson) 

..and the ESP8266 environment for Arduino of course



The controller i got has 1MB of flash (8 Megabits) compile for generic ESP8266 module with 1M (512k SPIFFS)


Links:
 https://metalab.at/wiki/Metalights


Author: overflo
Date of release: 11/2016


(Ab)use as you like this is freeware, but make sure to comply with the libraries licenses.
If you are using this for military equipment you are out of your head.
Also you might consider your life choices.


*/


#include <FS.h>     


#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArtnetWifi.h>


//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          

#include <ArduinoJson.h>        



#define redPin    15 
#define greenPin  13 
#define bluePin   12 
#define w1Pin     14
#define w2Pin     4


// onbaord green LED D1
#define LEDPIN    5
// onbaord red LED D2
#define LED2PIN   1




int redVal =0;
int greenVal =0;
int blueVal =0;
int w1Val =0;
int w2Val =0;



const int numberOfChannels =5;


// Artnet settings
ArtnetWifi artnet;
const int startUniverse = 0; // CHANGE FOR YOUR SETUP most software this is 1, some software send out artnet first universe as 0.

// Check if we got all universes
const int maxUniverses = numberOfChannels / 512 + ((numberOfChannels % 512) ? 1 : 0);
bool universesReceived[maxUniverses];
bool sendFrame = 1;
int previousDataLength = 0;






//define your default values here, if there are different values in config.json, they are overwritten.
char artnet_universe[3];
char id_offset[3];

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial1.println("Should save config");
  shouldSaveConfig = true;
}








boolean StartWifiManager(void)
{
  

      Serial1.println("StartWifiManager() called");   
  
  
  WiFiManagerParameter custom_artnet_universe("artnet_universe", "ARTnet Universe", artnet_universe, 3);
  WiFiManagerParameter custom_id_offset("id_offset", "ID offset", id_offset, 3);

  
    //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;



  //reset settings - for testing
  //wifiManager.resetSettings();



  wifiManager.setSaveConfigCallback(saveConfigCallback);
  
  
    
  //set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(192,168,1,200), IPAddress(192,168,1,1), IPAddress(255,255,255,0));


  wifiManager.addParameter(&custom_artnet_universe);
  wifiManager.addParameter(&custom_id_offset);




  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(60);
  
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if(!wifiManager.autoConnect("ARTNET-NODE")) {
    Serial1.println("failed to connect and hit timeout");



    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
 //   delay(5000);
  } 

  //if you get here you have connected to the WiFi
  Serial1.println("connected...yeey :)");

  
 //read updated parameters
  strcpy(artnet_universe, custom_artnet_universe.getValue());
  strcpy(artnet_universe, custom_id_offset.getValue());
  
  
  if (shouldSaveConfig) {
    
    
    
    
    Serial1.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["artnet_universe"] = artnet_universe;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial1.println("failed to open config file for writing");
    }

    json.printTo(Serial1);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
  
  
  
  
}





void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{
 
 
 

 
 
  
   //TODO
   /*
   if UNIVERSE == myuniverse
   
   parse data from offset X
   
   if data != last data change data
   
   
   
   
   */
   
  
  
  
  /*
  
  
//  DEBUG DMX FRAMES

  boolean tail = false;
  
  Serial1.print("DMX: Univ: ");
  Serial1.print(universe, DEC);
  Serial1.print(", Seq: ");
  Serial1.print(sequence, DEC);
  Serial1.print(", Data (");
  Serial1.print(length, DEC);
  Serial1.print("): ");
  
  if (length > 16) {
    length = 16;
    tail = true;
  }
  // send out the buffer
  for (int i = 0; i < length; i++)
  {
    Serial1.print(data[i], HEX);
    Serial1.print(" ");
  }
  if (tail) {
    Serial1.print("...");
  }
  Serial1.println();
  */
  
  

  
  
  
  
  
  
  
  
  
  /*
  
//  Serial1.println("i got some data");
  
  sendFrame = 1;


  // Store which universe has got in
  if ((universe - startUniverse) < maxUniverses)
    universesReceived[universe - startUniverse] = 1;

  for (int i = 0 ; i < maxUniverses ; i++)
  {
    if (universesReceived[i] == 0)
    {
      //Serial1.println("Broke");
      sendFrame = 0;
      break;
    }
  }

*/




// I KNOW!!
// this is not nice.. but a quick proof of concept this formware needs some fine tuning..
// lots of repitation below. i am ashamed.
    
    if( data[0] != redVal)
    {
      
      digitalWrite(LED2PIN,LOW); 
      
      
     redVal = data[0];
     
     Serial1.print("Setting red to: ");
     Serial1.println(redVal);
     
     
     analogWrite(redPin, redVal);
    } 


    if( data[1] != greenVal)
    {
      
      digitalWrite(LED2PIN,LOW); 
      
      
     greenVal = data[1];
     
     
     Serial1.print("Setting green to: ");
     Serial1.println(greenVal);
     
     
     analogWrite(greenPin, greenVal);
    } 


    if( data[2] != blueVal)
    {
      
      
      digitalWrite(LED2PIN,LOW); 
      
     blueVal = data[2];
     
     Serial1.print("Setting blue to: ");
     Serial1.println(blueVal);
     
     
     analogWrite(bluePin, blueVal);
    } 
     
     
     
    if( data[3] != w1Val)
    {
      
      digitalWrite(LED2PIN,LOW); 
      
      
     w1Val = data[3];
     
     Serial1.print("Setting w1 to: ");
     Serial1.println(w1Val);
     
     analogWrite(w1Pin, w1Val);
    } 
     
     
     
    if( data[4] != w2Val)
    {
      
      digitalWrite(LED2PIN,LOW); 
      
      
     w2Val = data[4];
     
     Serial1.print("Setting w2 to: ");
     Serial1.println(w2Val);
     
     analogWrite(w2Pin, w2Val);
    } 
     
     
 

/*
  if (sendFrame)
  {
    // Reset universeReceived to 0
    memset(universesReceived, 0, maxUniverses);
  }
  */
  
  
  // onboard leds off 
// digitalWrite(LEDPIN,HIGH); 

 // GREEN onboard led off
 digitalWrite(LED2PIN,HIGH); 
  
  
}


void setupFS()
{
  
  
     Serial1.println("SetupFs() called");   
    delay(500); 
 
  
   //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial1.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial1.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial1.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial1.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial1.println("\nparsed json");

          strcpy(artnet_universe, json["artnet_universe"]);


        } else {
          Serial1.println("failed to load json config");
        }
      }
    }
  } else {
    Serial1.println("failed to mount FS");
  }
  //end read 
  
}


void setup()
{
  Serial1.begin(115200);
  
  Serial1.println("---"); 
  Serial1.println("START");    
  
  pinMode(redPin,OUTPUT);
  pinMode(bluePin,OUTPUT);
  pinMode(greenPin,OUTPUT);
  pinMode(w1Pin,OUTPUT);
  pinMode(w2Pin,OUTPUT);  
  
  
  pinMode(LEDPIN,OUTPUT);
  pinMode(LED2PIN,OUTPUT);
  
  
digitalWrite(redPin,LOW); 
digitalWrite(bluePin,LOW); 
digitalWrite(greenPin,LOW); 
digitalWrite(w1Pin,LOW); 
digitalWrite(w2Pin,LOW); 


digitalWrite(LEDPIN,HIGH); 
digitalWrite(LED2PIN,HIGH); 
 
 
   Serial1.println("PINS SET TO OUTPUT");    
 

  setupFS();
    Serial1.println("SetupFS() done.");    

  
  StartWifiManager();
    Serial1.println("StartWifiManager() done.");   
   
    
  artnet.begin();
      Serial1.println("artnet.begin() done.");  
  


  // this will be called for each packet received
  artnet.setArtDmxCallback(onDmxFrame);
}

void loop()
{
  

  // we call the read function inside the loop
  artnet.read();
}
