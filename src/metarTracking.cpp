/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/aiden/Desktop/IoT-Engineering/metarTracking/src/metarTracking.ino"
void callback(char *topic, byte *payload, unsigned int length);
void setup();
void loop();
#line 1 "c:/Users/aiden/Desktop/IoT-Engineering/metarTracking/src/metarTracking.ino"
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(MANUAL);

//blynk
#define BLYNK_TEMPLATE_ID "TMPL237Th9ELE"
#define BLYNK_TEMPLATE_NAME "Metar"
#define BLYNK_AUTH_TOKEN "Nj0_zsdLBkKbOsu_iybFFNaDcrDhr3fQ"

#include "MQTT.h"
#include "oled-wing-adafruit.h"
#include "blynk.h"

OledWingAdafruit display;

//variables
std::string status;

unsigned long moment = millis();

std::string cloudCatagory;
std::string cloudHeight;
int height;

String airports[34] = {"ATL", "BOS", "BWI", "CLE", "CLT", "CVG", "DCA", "DEN","DFW",
"DTW","EWR","FLL","IAD","IAH","JFK","LAS","LAX","LGA","MCO","MDW","MEM","MIA",
"MSP","ORD","PDX","PHL","PHX","PIT","SAN","SEA","SFO","SLC","STL","TPA"};

String airport;

std::string statureMiles;
float stature;

int statureMilesLocation;

std::string metarCode;
std::string metarCodeReversed;

//this is a blynk slider because I would need to upgrade to get a text input
BLYNK_WRITE(V1) {
  //gets the airport code from the list
  airport = airports[param.asInt() - 1];
}

void callback(char *topic, byte *payload, unsigned int length)
{
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = NULL;

    //takes the recieved code and reverses it
    metarCode = p;
    metarCodeReversed = metarCode;
    reverse(metarCodeReversed.begin(), metarCodeReversed.end());


  //uses the non reversed code to find the cloud type
  if (metarCode.find("SCT") != -1){
    cloudCatagory = "SCT";
  }else if (metarCode.find("SKC") != -1){
    cloudCatagory = "SKC";
  }else if (metarCode.find("FEW") != -1){
    cloudCatagory = "FEW";
  }else if (metarCode.find("BKN") != -1){
    cloudCatagory = "BKN";
  }else if (metarCode.find("OVC") != -1){
    cloudCatagory = "OVC";
  }
  
  //finds the cloud height and stores it as an int
  cloudHeight = metarCode.substr((metarCode.find(cloudCatagory))+3, 3);
  height = stoi(cloudHeight);

  //takes the reversed metar code to find the amount of stature miles (stores it reversed in a string)
  statureMiles = metarCodeReversed.substr(
  (metarCodeReversed.find("MS") + 2),
  (metarCodeReversed.find(" ",
   metarCodeReversed.find("MS")) - (metarCodeReversed.find("MS") + 2)));

  //converts stature miles into the correct order and a float
  reverse(statureMiles.begin(),statureMiles.end());
  stature = atof(statureMiles.c_str());

  if (stature < 1.0 || ((cloudCatagory == "OVC" || cloudCatagory == "BKN") && height < 5)){
    status = "LIFR";
  }else if (stature < 3.0 || ((cloudCatagory == "OVC" || cloudCatagory == "BKN") && height < 10)){
    status = "IFR";
  }else if (stature >= 3.0 || ((cloudCatagory == "BKN" || cloudCatagory == "OVC") && height >= 10)){
    status = "MVFR";
  }else{
    /*Note: if this were to actually get used in an airport, I would not want to have 
    VFR be the else. It would be better to have LIFR be the default to not be at risk.
    This should be sufficient for a demonstration though.*/
    status = "VFR";
  }

  display.setCursor(0,0);
  display.println(status.c_str());
  display.display();
}

MQTT client("lab.thewcl.com", 1883, callback);

void setup() {
  Serial.begin(9600);
  Serial.println("connected");
  delay(5000); // Allow board to settle

  Blynk.begin(BLYNK_AUTH_TOKEN);

  //setting up the display
  display.setup();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.display();

  //subscribing to the channels
  client.connect(System.deviceID());
    if (client.isConnected())
    {
        client.subscribe("airport/#");
    }

  
}

void loop() {
  display.loop();
  Blynk.run();

  //gets new metar code every 10 seconds
  if (moment >= millis() + 10000){
    client.publish("airport/request", airport);
  }
 }