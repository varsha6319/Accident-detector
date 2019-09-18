#include <ELClient.h>
#include <ELClientRest.h>

char buff[128];
String sendData = "";
boolean sendMail = true;
long previousMillis = 0;
long interval = 60000;
//const byte waterLevelSensor = 7;// digital pin 7 has a water level sensor attached to it
const byte moistureSensor = 8;  // digital pin 5 has a water level sensor attached to it
//const byte motorPin = 3;   // digital pin 3 has a motor attached to it

// replace with your channel's thingspeak API key
String API_KEY = "G582CI6D9D45ZE0R";
// Initialize a connection to esp-link using the normal hardware serial port both for
// SLIP and for debug messages.
ELClient esp(&Serial, &Serial);

// Initialize a REST client on the connection to esp-link
ELClientRest rest(&esp);

boolean wifiConnected = false;

// Callback made from esp-link to notify of wifi status changes
// Here we print something out and set a global flag
void wifiCb(void *response)
{
  ELClientResponse *res = (ELClientResponse*)response;
  if (res->argc() == 1)
  {
    uint8_t status;
    res->popArg(&status, 1);

    if (status == STATION_GOT_IP)
    {
      Serial.println("WIFI CONNECTED");  //Wifi gets connected at this place
      wifiConnected = true;
    }
    else
    {
      Serial.print("WIFI NOT READY: ");//Wifi not connected,check connection
      Serial.println(status);
      wifiConnected = false;
    }
  }
}




void setup()
{
  Serial.begin(9600);   // the baud rate here needs to match the esp-link config
  Serial.println("EL-Client starting!");

  // Sync-up with esp-link, this is required at the start of any sketch and initializes the
  // callbacks to the wifi status change callback. The callback gets called with the initial
  // status right after Sync() below completes.
  esp.wifiCb.attach(wifiCb); // wifi status change callback, optional (delete if not desired)
  bool ok;
  do
  {
    ok = esp.Sync();  // sync up with esp-link, blocks for up to 2 seconds
    if (!ok) Serial.println("EL-Client sync failed!");
  } while (!ok);
  Serial.println("EL-Client synced!");

  // Get immediate wifi status info for demo purposes. This is not normally used because the
  // wifi status callback registered above gets called immediately.
  esp.GetWifiStatus();
  ELClientPacket *packet;
  if ((packet = esp.WaitReturn()) != NULL)
  {
    Serial.print("Wifi status: ");
    Serial.println(packet->value);
  }

  pinMode(moistureSensor, INPUT);
  pinMode(A1,OUTPUT);

}

// the loop routine runs over and over again forever:
void loop()
{
  unsigned long currentMillis = millis();
  //digitalWrite(motorPin, LOW);  // motor off

  // read the input pins:
  int moistureSensorState = digitalRead(moistureSensor);
  delay(50);


  if (moistureSensorState == HIGH)  // if soil is dry then water the plant
  {
    char moisture[6];
    dtostrf(moistureSensorState, 4, 2, moisture);
    // Set up the REST client to talk to www.thingspeak.com, this doesn't connect to that server,
    // it just sets-up stuff on the esp-link side
    int err = rest.begin("api.thingspeak.com");
    if (err != 0)
    {
      Serial.print("REST begin failed: ");
      Serial.println(err);
      while (1) ;
    }
    Serial.println("EL-REST ready");
    sprintf(buff, "/update?api_key=%s&field1=%s", API_KEY.c_str(), moisture); // log values to thingspeak

    logToThingspeak();  //Log to thingspeak using commands under void LogToThingspeak()
    digitalWrite(A1,HIGH);
    delay(1000);
    digitalWrite(A1,LOW);
    delay(1000);
    delay(15000);


  }



}

void logToThingspeak()
{
  // process any callbacks coming from esp_link
  esp.Process();


  // if we're connected make an HTTP request
  if (wifiConnected)
  { 
    Serial.println("wifi connected!!");
    // Request /utc/now from the previously set-up server
    rest.get((const char*)buff);

    char response[266];
    uint16_t code = rest.waitResponse(response, 266);
    if (code == HTTP_STATUS_OK)    //check for response for HTTP request
    {
      Serial.println("ARDUINO: GET successful:");
      Serial.println(response);
    }
    else
    {
      Serial.print("ARDUINO: GET failed: ");
      Serial.println(code);
    }

  }

}
