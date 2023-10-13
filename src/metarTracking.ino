// SYSTEM_THREAD(ENABLED);

// blynk
#define BLYNK_TEMPLATE_ID "TMPL237Th9ELE"
#define BLYNK_TEMPLATE_NAME "Metar"
#define BLYNK_AUTH_TOKEN "Nj0_zsdLBkKbOsu_iybFFNaDcrDhr3fQ"

#include "MQTT.h"
#include "blynk.h"

// defining pins for the rgb LED
#define red A0
#define blue A1
#define green A2

// variables
std::string status;

unsigned long moment = millis();

std::string cloudCatagory;
std::string cloudHeight;
int height;

String airports[34] = {"ATL", "BOS", "BWI", "CLE", "CLT", "CVG", "DCA", "DEN", "DFW","DTW", "EWR", "FLL",
                      "IAD", "IAH", "JFK", "LAS", "LAX", "LGA", "MCO", "MDW", "MEM", "MIA","MSP",
                      "ORD", "PDX", "PHL", "PHX", "PIT", "SAN", "SEA", "SFO", "SLC", "STL", "TPA"};

String airport = "ORD"; //Chicago is the default airport

std::string statureMiles;
float stature;

int statureMilesLocation;

std::string metarCode;
std::string metarCodeReversed;

// this is a blynk slider because I would need to upgrade to get a text input
BLYNK_WRITE(V1)
{
  // gets the airport code from the list
  airport = airports[param.asInt() - 1];
  // expects a single line of the airport
  Serial.println(airport);
}

// function for the rgb led
void LED(int r, int b, int g)
{
  analogWrite(red, 255 - r);
  analogWrite(blue, 255 - b);
  analogWrite(green, 255 - g);
}

void callback(char *topic, byte *payload, unsigned int length)
{
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;

  // takes the recieved code and reverses it
  metarCode = std::string(p);
  metarCodeReversed = metarCode;
  reverse(metarCodeReversed.begin(), metarCodeReversed.end());
  Serial.println("Metar Code: " + String(metarCode.c_str()));

  // uses the non reversed code to find the cloud type
  if (metarCode.find("SCT") != -1)
  {
    cloudCatagory = "SCT";
  }
  else if (metarCode.find("SKC") != -1)
  {
    cloudCatagory = "SKC";
  }
  else if (metarCode.find("FEW") != -1)
  {
    cloudCatagory = "FEW";
  }
  else if (metarCode.find("BKN") != -1)
  {
    cloudCatagory = "BKN";
  }
  else if (metarCode.find("OVC") != -1)
  {
    cloudCatagory = "OVC";
  }
  else
  {
    cloudCatagory = "CLR";
  }

  Serial.println("Cloud Category: " + String(cloudCatagory.c_str()));

  // finds the cloud height and stores it as an int
  if (cloudCatagory != "CLR")
  {
    cloudHeight = metarCode.substr((metarCode.find(cloudCatagory)) + 3, 3);
    height = stoi(cloudHeight);
  }

  Serial.println(String(height) + " * 100ft high clouds");

  // takes the reversed metar code to find the amount of stature miles (stores it reversed in a string)
  statureMiles = metarCodeReversed.substr(
      (metarCodeReversed.find("MS") + 2),
      (metarCodeReversed.find(" ",
                              metarCodeReversed.find("MS")) -
       (metarCodeReversed.find("MS") + 2)));

  // converts stature miles into the correct order and a float
  reverse(statureMiles.begin(), statureMiles.end());

  // Makes the conversion from a string fraction to someting that can be converted to a float
  if (statureMiles.find("/") != -1)
  {
    statureMiles.insert(statureMiles.find("/"), ".0");
    statureMiles.append(".0");
    float numerator = atof((statureMiles.substr(0, statureMiles.find("/"))).c_str());
    float denominator = atof((statureMiles.substr(statureMiles.find('/') + 1, statureMiles.length())).c_str());
    stature = numerator / denominator;
  }
  else
  {
    stature = atof(statureMiles.c_str());
  }
  Serial.println(String(stature) + " Stature miles");

  // determing the status based on the regulations and setting the onboard RGB LED
  if (stature < 1.0 || ((cloudCatagory == "OVC" || cloudCatagory == "BKN") && height < 5))
  {
    status = "LIFR";
    LED(255, 255, 0);
    Serial.println("LIFR");
  }
  else if (stature < 3.0 || ((cloudCatagory == "OVC" || cloudCatagory == "BKN") && height < 10))
  {
    status = "IFR";
    LED(255, 0, 0);
    Serial.println("IFR");
  }
  else if ((stature >= 3.0 && stature < 5) || ((cloudCatagory == "BKN" || cloudCatagory == "OVC") && (height >= 10 && height < 30)))
  {
    status = "MVFR";
    LED(0, 255, 0);
    Serial.println("MVFR");
  }
  else
  {
    /*Note: if this were to actually get used in an airport, I would not want to have
    VFR be the else. It would be better to have LIFR be the default to not be at risk.
    This should be sufficient for a demonstration though.*/
    status = "VFR";
    LED(0, 0, 255);
    Serial.println("VFR");
  }
}

MQTT client("lab.thewcl.com", 1883, callback, true);

void setup()
{
  Serial.begin(9600);
  Serial.println("connected");
  delay(5000); // Allow board to settle

  Blynk.begin(BLYNK_AUTH_TOKEN);

  // pins
  pinMode(red, OUTPUT);
  pinMode(blue, OUTPUT);
  pinMode(green, OUTPUT);

  LED(0, 0, 0); // just setting the LED off
}

void loop()
{
  Blynk.run();

  // connecting to mqtt
  if (client.isConnected())
  {
    client.loop();
  }
  else
  {
    client.connect(System.deviceID());
    client.subscribe("airport/receive");
  }

  // gets new metar code every 10 seconds
  if (moment + 10000 <= millis())
  {
    client.publish("airport/request", airport);
    Serial.println("Sent Airport Code");
    moment = millis();
  }
}