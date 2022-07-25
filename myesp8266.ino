
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ArduinoJson.h>

#ifndef STASSID
#define STASSID "WIFI"
#define STAPSK  "password"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

WiFiUDP ntpUDP;
// By default 'pool.ntp.org' is used with 60 seconds update interval and
//  no offset 
const int time_offset =  8 * 3600;
NTPClient timeClient(ntpUDP, "ntp.aliyun.com", time_offset, 60000);
// You can specify the time server pool and the offset, (in seconds)
// additionally you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);


long getTimeStamp(){
  // fuck this
  return timeClient.getEpochTime() - time_offset;
}


// used gpio for switch
const int SWITCH_D1 = 5;
const int SWITCH_D2 = 4;
const int MONITOR_D5 = 14;
const int MONITOR_D6 = 12;


ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

void handleRoot() {
  String out = "{}";
  server.send(200, "application/json", out);
}

void handleNotFound() {
  String message = "{\"status\":\"404\"}";
  server.send(404, "application/json", message);
}

void turn_on() {
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
  String out = "{\"switch\":\"on\"}";
  server.send(200, "application/json", out);
}

void turn_off() {
  digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED on (Note that LOW is the voltage level
  String out = "{\"switch\":\"off\"}";
  server.send(200, "application/json", out);
}

void favicon_ico() {
   server.send(200, "text/html", "");
}

void health() {
  // TODO uptime
  String out = "{\"uptime\":\"\",\"timestamp\":\""+ String(getTimeStamp()) +"\"}";
  server.send(200, "application/json", out);
}


void setup_gpio(){
  pinMode(SWITCH_D1, OUTPUT);     // Initialize the SWITCH_D1 pin as an output
  pinMode(SWITCH_D2, OUTPUT);     // Initialize the SWITCH_D2 pin as an output

  pinMode(MONITOR_D5, INPUT);     // Initialize the MONITOR_D5 pin as an input
  pinMode(MONITOR_D6, INPUT);     // Initialize the MONITOR_D6 pin as an input
  
  digitalWrite(SWITCH_D1, LOW);   // init LOW
  digitalWrite(SWITCH_D2, LOW);   // init LOW
}

void switch_api(){

  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"status\":\"Method Not Allowed\"}");
  } else {
    String request = server.arg("plain");

    char json[1024];
    request.toCharArray(json,50);

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, json);
    
    String switchName = doc["switch"];
    int    status     = doc["status"];

    // status check
    if (!(status == 1 || status == 0 )){
      server.send(200, "application/json", "{\"status\":\"param error\",\"message\":\"status err\"}");
      return;
    }

    // switch 
    if (switchName.equals("SWITCH_D1")) {
      digitalWrite(SWITCH_D1, status);
    }else if (switchName.equals("SWITCH_D2")) {
      digitalWrite(SWITCH_D2, status);
    }else {
      server.send(200, "application/json", "{\"status\":\"switch error\",\"message\":\"not sopport switchName\"}");
      return;
    }
    
    // result
    server.send(200, "application/json", "{\"status\":\"success\",\"message\":\""+switchName+" switch to "+ status +" success\"}");
  }
}


String getSettings() {
  
    String response = "{";
 
    response+= "\"ip\": \""+WiFi.localIP().toString()+"\"";
    response+= ",\"gw\": \""+WiFi.gatewayIP().toString()+"\"";
    response+= ",\"nm\": \""+WiFi.subnetMask().toString()+"\"";
 
    response+= ",\"rssi\": \""+String(WiFi.RSSI())+"\"";
 
    response+= ",\"chipId\": \""+String(ESP.getChipId())+"\"";
    response+= ",\"flashChipId\": \""+String(ESP.getFlashChipId())+"\"";
    response+= ",\"flashChipSize\": \""+String(ESP.getFlashChipSize())+"\"";
    response+= ",\"flashChipRealSize\": \""+String(ESP.getFlashChipRealSize())+"\"";
    
    response+= ",\"freeHeap\": \""+String(ESP.getFreeHeap())+"\"";

    response+="}";
    return response;
}


void switch_state(){
  if (server.method() != HTTP_GET) {
    server.send(405, "application/json", "{\"status\":\"Method Not Allowed\"}");
  } else {
    int switch_d1 = digitalRead(SWITCH_D1);
    int switch_d2 = digitalRead(SWITCH_D2);
    int monitor_d5 = digitalRead(MONITOR_D5);
    int monitor_d6 = digitalRead(MONITOR_D6);

    DynamicJsonDocument doc(1024);
    
    doc["switch_d1"] =  digitalRead(SWITCH_D1);
    doc["switch_d2"] =  digitalRead(SWITCH_D2);
    doc["monitor_d5"] = digitalRead(MONITOR_D5);
    doc["monitor_d6"] = digitalRead(MONITOR_D6);
    doc["chipInfo"] = getSettings();

    char json_string[256];
    serializeJson(doc, json_string);

    server.send(200, "application/json",  String(json_string));
  }
}


void setup(void) {
 
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  // default off
  digitalWrite(LED_BUILTIN, HIGH); 

  // init gpio for switch
  setup_gpio();
  
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  // biz
  server.on("/", handleRoot);
  server.on("/favicon.ico", favicon_ico);
  
  // test switch led
  server.on("/on", turn_on);
  server.on("/off", turn_off);

  // switch on/off post
  server.on("/api/switch", switch_api);
  // switch state get
  server.on("/api/state", switch_state);
  // health check
  server.on("/health", health);
  
  // not found
  server.onNotFound(handleNotFound);

  // update for ota
  //Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host);
  httpUpdater.setup(&server);
  
  server.begin();
  
  Serial.println("HTTP server started");
}

void loop(void) {
  // time update by interval
  timeClient.update();
  server.handleClient();
  MDNS.update();
}
