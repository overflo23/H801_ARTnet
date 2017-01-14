/*

v 0.9 .. almos there..

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




// where are all the led(strips) attached?
#define redPin    15
#define greenPin  13
#define bluePin   12
#define w1Pin     14
#define w2Pin     4




// onbaord GREEN LED D1
#define LEDPIN    5
// onbaord RED LED D2
#define LED2PIN   1



// current value of each channel (0-255) used with analogWrite()
// You can set a default light configuration here that appears when you turn the leds on and tehre is nothing sent from the network
int redVal = 0;
int greenVal =0;
int blueVal = 0;
// normally full white when we turn on
int w1Val = 255;
int w2Val = 0 ;



// each controller should parse 5 bytes from the 512 bytes universe data
// the first controller reads 0-4, the second should have "5" as offset and read from 5-9 and so on..
// these are changeed from the webinterface
int idOffset = 0;
int artnetUniverse = 0;





//network stuff
//default custom static IP, changeable from the webinterface
char ip[16] = ""; // could be 1.3.3.7
char gw[16] = ""; // could be 1.3.3.1
char sn[16] = ""; // could be 255.255.255.0







/* ----------------- DON'T touch the defines below unless you now what you are up to.---------------------------- */



// Artnet settings
ArtnetWifi artnet;


// how many? 5.
const int numberOfChannels = 5;

// a flag for ip setup
boolean set_ip = 0;



// TOODO:  should check if i still need this ..
// Check if we got all universes  
const int maxUniverses = numberOfChannels / 512 + ((numberOfChannels % 512) ? 1 : 0);
bool universesReceived[maxUniverses];
bool sendFrame = 1;
int previousDataLength = 0;





// temporary buffer for webinterface
char artnet_universe[3];
char id_offset[3];

//flag for saving data
bool shouldSaveConfig = false;




#define WL_MAC_ADDR_LENGTH 6


//creates the string that shows if the device goes into accces pint mode
String getUniqueSystemName()
{
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);


  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) + String("-") + String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);

  macID.toUpperCase();
  String UniqueSystemName = String("LOUNGE_CEILING_") + macID;

  return UniqueSystemName;
}





// displays mac address on serial port
void printMac()
{
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);

  Serial1.print("MAC: ");
  for (int i = 0; i < 5; i++)
  {
    Serial1.print(mac[i], HEX);
    Serial1.print(":");
  }
  Serial1.println(mac[5],HEX);

}








//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial1.println("Should save config");
  shouldSaveConfig = true;
}






// this does a lot.

boolean StartWifiManager(void)
{


  Serial1.println("StartWifiManager() called");



 
  // add parameter for artnet setup in GUI
  WiFiManagerParameter custom_artnet_universe("artnet_universe", "ARTnet Universe (Default: 0)", artnet_universe, 3);
  WiFiManagerParameter custom_id_offset("id_offset", "ID offset  (Default: 0, +5)", id_offset, 3);


  // add parameters for IP setuyp in GUI
  WiFiManagerParameter custom_ip("ip", "Static IP (Blank for DHCP)", ip, 16);
  WiFiManagerParameter custom_gw("gw", "Static Gateway (Blank for DHCP)", gw, 16);
  WiFiManagerParameter custom_sn("sn", "Static Netmask (Blank for DHCP)", sn, 16);



  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;




  //DEBUG - SETUP
  //reset settings - for testing
//  wifiManager.resetSettings();


  // this is what is called if the webinterface want to save data, callback is right above this function and just sets a flag.
  wifiManager.setSaveConfigCallback(saveConfigCallback);


  // this actually adds the parameters defined above to the GUI
  wifiManager.addParameter(&custom_artnet_universe);
  wifiManager.addParameter(&custom_id_offset);




  // if the flag is set we configure a STATIC IP!
  if (set_ip)
  {
    //set static ip
    IPAddress _ip, _gw, _sn;
    _ip.fromString(ip);
    _gw.fromString(gw);
    _sn.fromString(sn);
    
    // this adds 3 fields to the GIU for ip, gw and netmask, but IP needs to be defined for this fields to show up.
    wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);



    Serial1.println("Setting IP to:");
    Serial1.print("IP: ");
    Serial1.println(ip);
    Serial1.print("GATEWAY: ");
    Serial1.println(gw);
    Serial1.print("NETMASK: ");
    Serial1.println(sn);






  }
  else
  {
    
   // i dont want to fill these fields per default so i had to implement this workaround .. its ugly.. but hey. whatever.  
    wifiManager.addParameter(&custom_ip);
    wifiManager.addParameter(&custom_gw);
    wifiManager.addParameter(&custom_sn);
  }









  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep in seconds
  // also really annoying if you just connected and the damn thing resets in the middel of filling in the GUI..
  // 5 minuts seems reasonable
  wifiManager.setTimeout(300);
    Serial1.println("STARTED ACCESS POINT!");
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(getUniqueSystemName().c_str())) {
    Serial1.println("failed to connect and hit timeout for config :(");


    Serial1.println("Good bye kids! I am all outta here.");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    //   we never get here.
    
    
  }

  //everything below here is only executed once we are connected to a wifi.


  //if you get here you have connected to the WiFi
  Serial1.println("CONNECTED");



  // connection worked so lets save all those parameters to the config file
  strcpy(ip, custom_ip.getValue());
  strcpy(gw, custom_gw.getValue());
  strcpy(sn, custom_sn.getValue());



  // if we defined something in the gui before that does not work we might to get rid of previous settings in the config file
  // so if the form is transmitted empty, delete the entries.
  if (strlen(ip) < 8)
  {
    strcpy(ip, "");
    strcpy(gw, "");
    strcpy(sn, "");
    Serial1.println("RESETTING IP/GW/SUBNET EMPTY!");
  }

  strcpy(artnet_universe, custom_artnet_universe.getValue());
  strcpy(id_offset, custom_id_offset.getValue());


  // this is true if we come from the GUI and clicked "save"
  // the flag is created in the callback above..
  if (shouldSaveConfig) {






    Serial1.println("saving config");
    
    // JSON magic
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();


    // things the JSON should save
    json["artnet_universe"] = artnet_universe;
    json["id_offset"] = id_offset;

    json["ip"] = ip;
    json["gw"] = gw;
    json["sn"] = sn;



    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial1.println("failed to open config file for writing");
    }

    // dump config to Serial
    json.printTo(Serial1);

    Serial1.println("");

    // dump config to file in FS
    json.printTo(configFile);
    Serial1.println("");

    configFile.close();
    //end save
  }







}




// called on each frame, this function is dead-bug-ugly. sorry, i might fix it if i get some money for it.

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{

  
  /*

  //  DEBUG DMX FRAMES - left here for reference  if something does not work out for you, here is the raw data to peek around.

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





  // I KNOW!!
  // this is not nice.. but a quick proof of concept this formware needs some fine tuning..
  // lots of repitation below. i am ashamed.



// DEBUG

/*
Serial1.print(" onDmxFrame() -  ");
Serial1.print(" UNIVERSE: ");
Serial1.print(universe);
Serial1.print(" (our universe is ");
Serial1.print(artnetUniverse);
Serial1.print(") OFFSET: ");
Serial1.println(idOffset);
*/



  // not our universe? good bye!
  if (universe != artnetUniverse)  return;



  // only change values if thez differ from what hey are at the moment
  if ( data[0 + idOffset] != redVal)
  {

    digitalWrite(LED2PIN, LOW);
    redVal = data[0 + idOffset];

    Serial1.print("Setting red to: ");
    Serial1.println(redVal);


    analogWrite(redPin, redVal);
  }


  if ( data[1 + idOffset] != greenVal)
  {

    digitalWrite(LED2PIN, LOW);
    greenVal = data[1 + idOffset];


    Serial1.print("Setting green to: ");
    Serial1.println(greenVal);


    analogWrite(greenPin, greenVal);
  }


  if ( data[2 + idOffset] != blueVal)
  {


    digitalWrite(LED2PIN, LOW);

    blueVal = data[2 + idOffset];

    Serial1.print("Setting blue to: ");
    Serial1.println(blueVal);


    analogWrite(bluePin, blueVal);
  }



  if ( data[3 + idOffset] != w1Val)
  {

    digitalWrite(LED2PIN, LOW);


    w1Val = data[3 + idOffset];

    Serial1.print("Setting w1 to: ");
    Serial1.println(w1Val);

    analogWrite(w1Pin, w1Val);
  }



  if ( data[4 + idOffset] != w2Val)
  {

    digitalWrite(LED2PIN, LOW);


    w2Val = data[4 + idOffset];

    Serial1.print("Setting w2 to: ");
    Serial1.println(w2Val);

    analogWrite(w2Pin, w2Val);
  }



  // GREEN onboard led off
  digitalWrite(LED2PIN, HIGH);


}

















// called when you turn the device on
// it opens files, but if it can not open files it creates the filesystem ..

void setupFS()
{


  Serial1.println("SetupFs() called");


  //read configuration from FS json
  Serial1.println("mounting FS...");




  if (SPIFFS.begin()) {
    Serial1.println("mounted file system");


    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial1.println("file is there, reading config.json");
      File configFile = SPIFFS.open("/config.json", "r");

      if (configFile) {
        Serial1.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        
        // dump contents to serial..
        json.printTo(Serial1);
        Serial1.println("");
        
        if (json.success()) {
          Serial1.println("\nparsed json");


        // are you looking for exploitable buffer overflows?
        // look no further!  but.. whatever :)
        // the IOT is fun fun fun!

        // make int from string
        strcpy(id_offset, json["id_offset"]);
        idOffset = atoi(id_offset);

        // make int from string
        strcpy(artnet_universe, json["artnet_universe"]);
        artnetUniverse = atoi(artnet_universe);


    
        // so there are IP settings in the config file.
        if (json["ip"]) {

            // lets use the IP settings from the config file for network config.
            set_ip = 1;


            Serial1.println("setting custom ip from config");
           
            // tehse strings are parsed when the network is setup .. this is way up there, you already saw it.
            strcpy(ip, json["ip"]);
            strcpy(gw, json["gw"]);
            strcpy(sn, json["sn"]);




          } else {
            Serial1.println("no custom ip in config");
          }




        } else {
          Serial1.println("failed to load json config");
        }
      }
    }
  } else {
    Serial1.println("failed to mount FS");


    Serial1.println("UNABLE TO MOUNT FS! calling SPIFFS.format();");
    
    //filesystem not readable? You probably turned on the device for the first time. Let's create the filesystem.
    SPIFFS.format();



  }
  //end read

}


void setup()
{
  
  
  // Always use Serial1 not Serial on the H801 boards, took me some time to figure this one out :)
  Serial1.begin(115200);



  Serial1.println("---");
  Serial1.println("START");

 // all outpunts
  pinMode(redPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(w1Pin, OUTPUT);
  pinMode(w2Pin, OUTPUT);

  // onboard leds also outputs
  pinMode(LEDPIN, OUTPUT);
  pinMode(LED2PIN, OUTPUT);



  Serial1.println("PINS SET TO OUTPUT");
 
 // default is 1kHz, we set it to 25kHz to get rid of nasty sounds in the power supply during PWM
 analogWriteFreq(25000);
 
 
 //  analogWrite() supports 10 bit resolution (0-1023) on ESP8266 instead of the usual 0-255 by default. but artnet is 0-255 only..
 // we fix this by setting the resolution to 10 bit.
 analogWriteRange(255);

  // set default colors as long as there is no data over network coming in..
  analogWrite(redPin, redVal);
  analogWrite(bluePin, blueVal);
  analogWrite(greenPin, greenVal);
  analogWrite(w1Pin, w1Val);
  analogWrite(w2Pin, w2Val);

  

  Serial1.println("Leds set to default states");

  // on boards led are wired for LOW to turn them on and HIGH turning them off..
  
  // red led ON
  digitalWrite(LEDPIN, LOW);
  
  // green led OFF
  digitalWrite(LED2PIN, HIGH);






  // display the MAC on Serial1
  printMac();

  
  Serial1.print("System Name: ");
  Serial1.println(getUniqueSystemName());





  setupFS();
  Serial1.println("SetupFS() done.");



  StartWifiManager();
  Serial1.println("StartWifiManager() done.");




  Serial1.println("Starting the ARTnet party");
  
  
  
  Serial1.print("ART-net UNIVERSE: ");
  Serial1.println(artnetUniverse);
  Serial1.print("ART-net OFFSET "); 
  Serial1.println(idOffset);
 
 


  artnet.begin();
  Serial1.println("artnet.begin() done.");



  // this will be called for each packet received
  artnet.setArtDmxCallback(onDmxFrame);
}





// life in loops.
void loop()
{
  // we call the read function inside the loop
  artnet.read();
}






// ane they lived happily ever after
// the end.
