
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <ESP8266HTTPUpdateServer.h>

#ifndef STASSID
#define STASSID "WIFI"
#define STAPSK  "password"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
//  no offset 
NTPClient timeClient(ntpUDP, "ntp.aliyun.com", 0, 60000);

// You can specify the time server pool and the offset, (in seconds)
// additionally you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

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
  String out = "{\"uptime\":\"\",\"timestamp\":\""+ String(timeClient.getEpochTime()) +"\"}";
  server.send(200, "application/json", out);
}

// crontab set get run (option) or api trigger


void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  // default off
  digitalWrite(LED_BUILTIN, HIGH); 
  
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
  server.on("/on", turn_on);
  server.on("/off", turn_off);
  server.on("/health", health);
  // not found
  server.onNotFound(handleNotFound);

  // update for ota
  httpUpdater.setup(&server);
  server.begin();
  //Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host);
    
  Serial.println("HTTP server started");
}
          
void loop(void) {
  // time update by interval
  timeClient.update();
  server.handleClient();
  MDNS.update();
}
