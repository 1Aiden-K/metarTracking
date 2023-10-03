SYSTEM_THREAD(ENABLED);

//blynk
#define BLYNK_TEMPLATE_ID "TMPL237Th9ELE"
#define BLYNK_TEMPLATE_NAME "Metar"
#define BLYNK_AUTH_TOKEN "Nj0_zsdLBkKbOsu_iybFFNaDcrDhr3fQ"

#include "MQTT.h"
#include "blynk.h"

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

  //determing the status based on the regulations and setting the onboard RGB LED
  if (stature < 1.0 || ((cloudCatagory == "OVC" || cloudCatagory == "BKN") && height < 5)){
    status = "LIFR";
    RGB.color(139, 139, 0);
  }else if (stature < 3.0 || ((cloudCatagory == "OVC" || cloudCatagory == "BKN") && height < 10)){
    status = "IFR";
    RGB.color(255,0,0);
  }else if (stature >= 3.0 || ((cloudCatagory == "BKN" || cloudCatagory == "OVC") && height >= 10)){
    status = "MVFR";
    RGB.color(0,255,0);
  }else{
    /*Note: if this were to actually get used in an airport, I would not want to have 
    VFR be the else. It would be better to have LIFR be the default to not be at risk.
    This should be sufficient for a demonstration though.*/
    status = "VFR";
    RGB.color(0,0,255);
  }

  
}

MQTT client("lab.thewcl.com", 1883, callback);

void setup() {
  Serial.begin(9600);
  Serial.println("connected");
  delay(5000); // Allow board to settle

  Blynk.begin(BLYNK_AUTH_TOKEN);

  //taking control of the onboard LED
  RGB.control(true);
  RGB.color(0,0,0); //sets to white

  //connecting and subscribing to airport/request and airport/receive
  client.connect(System.deviceID());
    if (client.isConnected())
    {
        client.subscribe("airport/#");
    }

  
}

void loop() {
  Blynk.run();

  //gets new metar code every 10 seconds
  if (moment >= millis() + 10000){
    client.publish("airport/request", airport);
  }
 }