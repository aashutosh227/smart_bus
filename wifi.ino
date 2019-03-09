 #include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <FirebaseArduino.h>
#include <math.h>

#define FIREBASE_HOST "sbus-5d6c0.firebaseio.com"

#define FIREBASE_AUTH ""

#define pi 3.14159265358979323846

SoftwareSerial NodeMCU(D2,D3);

double deg2rad(double);
double rad2deg(double);

char myssid[] = "Don't care";         // your network SSID (name)
char mypass[] = "Rishi@222";          // your network password


//Credentials for Google GeoLocation API...
const char* Host = "www.googleapis.com";
String thisPage = "/geolocation/v1/geolocate?key=";
String key = "AIzaSyCId1L5bdwZNjKpfO1CuHUterCQYPWksgc";

int status = WL_IDLE_STATUS;
String jsonString = "{\n";

double latitude    = 0.0;
double longitude   = 0.0;
double slat,slong;
double accuracy    = 0.0;
int more_text = 1; // set to 1 for more debug output

int lf=0;
int srcList[10];
int destList[10];
int k=0;

void setup(){
	Serial.begin(9600);
	NodeMCU.begin(4800);
	pinMode(D2,INPUT);
	pinMode(D3,OUTPUT);

//Geolocation
  Serial.println("Start WiFi");
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.println("Setup done");
  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(myssid);
  WiFi.begin(myssid, mypass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(".");
  
  Serial.println();
  Firebase.begin(FIREBASE_HOST);
}

//Calculate distance
double distance(double lat1, double lon1, double lat2, double lon2, char unit) {
  double theta, dist;
  theta = lon1 - lon2;
  dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
  dist = acos(dist);
  dist = rad2deg(dist);
  dist = dist * 60 * 1.1515;
  switch(unit) {
    case 'M':
      break;
    case 'K':
      dist = dist * 1.609344;
      break;
    case 'N':
      dist = dist * 0.8684;
      break;
  }
  Serial.println(dist);
  return (dist);
}

double deg2rad(double deg) {
  return (deg * pi / 180);
}

double rad2deg(double rad) {
  return (rad * 180 / pi);
}

//Calculate Fare
double fare(double lat1, double lon1, double lat2, double lon2, char unit){
  delay(3000);
  double dist,fare;
  int fareInt;
  double fareDec;
  dist=distance(lat1,lon1,lat2,lon2,'K');
  fare=dist*10;
  Serial.println(fare);
  fareInt=fare;
  Serial.println(fareInt);
  fareDec=fare-fareInt;
  Serial.println(fareDec);
  if(fareDec<0.5){
    fare=floor(fare);
  }
  else{
    fare=ceil(fare);
  }
  if(fare==0.00){fare=10;}
  Serial.print("Fare:");
  Serial.println(fare);
  return (fare);
}


void updateBus()
{


String buspath = "/bus/207/";

Firebase.setFloat(buspath+"latitude",float(latitude));
Firebase.setFloat(buspath+"longitude",float(longitude));

}

void geolocation() {

  char bssid[6];
  DynamicJsonBuffer jsonBuffer;
  Serial.println("scan start");
  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found...");

    if (more_text) {
      // Print out the formatted json...
      Serial.println("{");
      Serial.println("\"homeMobileCountryCode\": 234,");  // this is a real UK MCC
      Serial.println("\"homeMobileNetworkCode\": 27,");   // and a real UK MNC
      Serial.println("\"radioType\": \"gsm\",");          // for gsm
      Serial.println("\"carrier\": \"Vodafone\",");       // associated with Vodafone
      //Serial.println("\"cellTowers\": [");                // I'm not reporting any cell towers
      //Serial.println("],");
      Serial.println("\"wifiAccessPoints\": [");
      for (int i = 0; i < n; ++i)
      {
        Serial.println("{");
        Serial.print("\"macAddress\" : \"");
        Serial.print(WiFi.BSSIDstr(i));
        Serial.println("\",");
        Serial.print("\"signalStrength\": ");
        Serial.println(WiFi.RSSI(i));
        if (i < n - 1)
        {
          Serial.println("},");
        }
        else
        {
          Serial.println("}");
        }
      }
      Serial.println("]");
      Serial.println("}");
    }
    Serial.println(" ");
  }
  // now build the jsonString...
  jsonString = "{\n";
  jsonString += "\"homeMobileCountryCode\": 234,\n"; // this is a real UK MCC
  jsonString += "\"homeMobileNetworkCode\": 27,\n";  // and a real UK MNC
  jsonString += "\"radioType\": \"gsm\",\n";         // for gsm
  jsonString += "\"carrier\": \"Vodafone\",\n";      // associated with Vodafone
  jsonString += "\"wifiAccessPoints\": [\n";
  for (int j = 0; j < n; ++j)
  {
    jsonString += "{\n";
    jsonString += "\"macAddress\" : \"";
    jsonString += (WiFi.BSSIDstr(j));
    jsonString += "\",\n";
    jsonString += "\"signalStrength\": ";
    jsonString += WiFi.RSSI(j);
    jsonString += "\n";
    if (j < n - 1)
    {
      jsonString += "},\n";
    }
    else
    {
      jsonString += "}\n";
    }
  }
  jsonString += ("]\n");
  jsonString += ("}\n");
  //--------------------------------------------------------------------

  Serial.println("");

  WiFiClientSecure client;

  //Connect to the client and make the api call
  Serial.print("Requesting URL: ");
  Serial.println("https://" + (String)Host + thisPage + "AIzaSyCYNXIYINPmTNIdusMjJloS4_BXSOff1_g");
  Serial.println(" ");
  if (client.connect(Host, 443)) {
    Serial.println("Connected");
    client.println("POST " + thisPage + key + " HTTP/1.1");
    client.println("Host: " + (String)Host);
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.println("User-Agent: Arduino/1.0");
    client.print("Content-Length: ");
    client.println(jsonString.length());
    client.println();
    client.print(jsonString);
    delay(500);
  }

  //Read and parse all the lines of the reply from server
  while (client.available()) {
    String line = client.readStringUntil('\r');
    if (more_text) {
      Serial.print(line);
    }
    JsonObject& root = jsonBuffer.parseObject(line);
    if (root.success()) {
      latitude    = root["location"]["lat"];
      longitude   = root["location"]["lng"];
      accuracy   = root["accuracy"];
    }
  }
  
  Serial.println("closing connection");
  Serial.println();
  client.stop();

  //Update the longitude and latitude to firebase
  updateBus();
  delay(4000);
  Serial.print("Latitude = ");
  Serial.println(latitude, 6);
  Serial.print("Longitude = ");
  Serial.println(longitude, 6);
  Serial.print("Accuracy = ");
  Serial.println(accuracy);
}

void updateSource(String rfidNo) { 
  //Firebase.setString("/coach/coach5", "14");
  String path = "/rfid/" + rfidNo;
  String buspath = "/bus/207/";
  Serial.print("Updating Source....");
  //Updating source coordinates in other variable
  //slatitude=latitude;
  //slongitude=longitude;
  //delay(1000);
  float slatitude = Firebase.getFloat(buspath+"latitude");
  //delay(1000);  
  float slongitude=Firebase.getFloat(buspath+"longitude");
  //delay(1000);
  slat=double(slatitude);
  slong=double(slongitude);
  //delay(1000);
  String date=Firebase.getString(buspath+"date");
  Serial.print("\nDate: "+date);
  //delay(1000);
  String timee = Firebase.getString(buspath+"time");
  Serial.print("\nTime: "+timee);
  //delay(1000);

  Firebase.setFloat(path+"/slat",slatitude);
  //delay(1000);
  Firebase.setFloat(path+"/slon",slongitude);
  //delay(1000); 
  Firebase.setString(path+"/date",date);
  //delay(1000);
  Firebase.setString(path+"/time",timee);
  delay(1000);
  Firebase.setInt(path+"/fare",0);
  //delay(1000);
  Firebase.setFloat(path+"/dlat",0.0);
  Firebase.setFloat(path+"/dlon",0.0);
  Serial.print("Source Updated!!!");
}

void updateDestination(String rfidNo){
  String path = "/rfid/" + rfidNo;
  String buspath = "/bus/207/";
  Serial.print("Updating Destination....");
  //delay(1000);
  float dlatitude = Firebase.getFloat(buspath+"latitude");
  //delay(1000);  
  float dlongitude=Firebase.getFloat(buspath+"longitude");
  //delay(1000);
  Firebase.setFloat(path+"/dlat",dlatitude);
  //delay(1000);
  Firebase.setFloat(path+"/dlon",dlongitude); 
  //delay(1000);
  int balance = Firebase.getInt(path+"/balance");
  //delay(5000);
  //calling fare function
  //delay(3000);
  double fare1 = fare(slat,slong,double(dlatitude),double(dlongitude),'K');//This fare needs to be calculatd based on laitude and longitude Here I m taking default fare as 10 Rs
  //delay(3000);
  int f=fare1;
  //delay(3000);
  Firebase.setInt(path+"/fare",f);
  //delay(2000);
  Firebase.setInt(path+"/balance",balance-f);
  //delay(1000);
  Serial.print("Destination Updated!!!!");
}

int checkList(int x){
  int i,f=0;
  for(i=0;i<10;i++){
    if(x==srcList[i]){
      f=1;
    }
  }
  return f;
}

void delete_src(int x){
  int i;
  for(i=0;i<10;i++){
    if(x==srcList[i]){
      k--;
      srcList[i]=srcList[k];
      srcList[k]=0;
    }
    else{
      continue;
    }
  }
}

void loop(){
  
  while(NodeMCU.available()>0){
    int val = NodeMCU.parseInt();
    if(NodeMCU.read()== '\n'){
    Serial.println(val);
    geolocation();
    delay(10000);
    if(checkList(val)!=1){
      srcList[k]=val;
      k++;
      updateSource(String(val));
      delay(5000);
    }
    else{
      delete_src(val);
      updateDestination(String(val));
      delay(5000);
    }
    }
   }
  
  delay(30);
  /*
	int i = 10;
	NodeMCU.print(i);
	NodeMCU.println("\n");
	delay(30);
  */
}


