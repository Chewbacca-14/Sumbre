#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ESP8266Ping.h>
#include "Sensor.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

const char *ssid = "sumbre";
const char *password = "adaptine";

const char *devDomain = "patrol.adaptine.com"; // replace with your server
const int httpsPort = 443;

// PatrolControl device token
const char *bearerToken = "68becdb258b33e320e00cd04|VDgCF3Ni6e1U7qdq624eWK6lty9cvHzPnhHeULXc5d9ac7be";

Sensor sensor;

unsigned long lastRead = 0;
float lastTemp = 0;
float lastHum = 0;

ESP8266WebServer server(80);

String sendHumidity(float lastTemp);
void checkForUpdates();

unsigned long lastUpdateCheck = 0;          // timestamp of last update check
const unsigned long updateInterval = 10000; // 1 minute in milliseconds

void setup()
{
  Serial.begin(115200);

  // Initialize sensor
  sensor.begin();

  if (!LittleFS.begin())
  {
    Serial.println("LittleFS mount failed");
    return;
  }
  Serial.println("LittleFS mounted successfully");

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);

  Serial.println();
  Serial.print("Hotspot IP: ");
  Serial.println(WiFi.softAPIP());

  checkForUpdates();

  // Serve files
  server.on("/", []()
            {
    File f = LittleFS.open("/index.html", "r");
    server.streamFile(f, "text/html");
    f.close(); });
  server.on("/style.css", []()
            {
    File f = LittleFS.open("/style.css", "r");
    server.streamFile(f, "text/css");
    f.close(); });
  server.on("/script.js", []()
            {
    File f = LittleFS.open("/script.js", "r");
    server.streamFile(f, "application/javascript");
    f.close(); });

  // Scan Wi-Fi
  server.on("/scan", []()
            {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; ++i) {
      json += "\"" + WiFi.SSID(i) + "\"";
      if (i < n - 1) json += ",";
    }
    json += "]";
    server.send(200, "application/json", json); });

  // Connect
  server.on("/connect", []()
            {
    String ssidInput = server.arg("ssid");
    String passInput = server.arg("pass");

    Serial.printf("Connecting to SSID: %s\n", ssidInput.c_str());

    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssidInput.c_str(), passInput.c_str());

    int timeout = 30;
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
      delay(1000);
      Serial.print(".");
      timeout--;
    }

    if (WiFi.status() == WL_CONNECTED) {
      String json = "{";
      json += "\"status\":\"connected\",";
      json += "\"ssid\":\"" + ssidInput + "\",";
      json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
      json += "\"gateway\":\"" + WiFi.gatewayIP().toString() + "\",";
      json += "\"dns\":\"" + WiFi.dnsIP().toString() + "\"";
      json += "}";
      server.send(200, "application/json", json);
    } else {
      server.send(200, "application/json", "{\"status\":\"failed\"}");
    } });

  // Ping
  server.on("/ping", []()
            {
    String result = sendHumidity(lastTemp);
    Serial.println("[Ping] Result: " + result);
    server.send(200, "text/plain", result); });

  // Sensor data
  server.on("/sensor", []()
            {
    String json = "{";
    json += "\"temperature\":44,";
    json += "\"humidity\":44";
    json += "}";
    server.send(200, "application/json", json); });

  // server.on("/sensor", []()
  //         {
  // String json = "{";
  // json += "\"temperature\":" + String(lastTemp, 1) + ",";
  // json += "\"humidity\":" + String(lastHum, 1);
  // json += "}";
  // server.send(200, "application/json", json); });

  // Status
  server.on("/status", []()
            {
    if (WiFi.status() == WL_CONNECTED) {
      String json = "{";
      json += "\"status\":\"connected\",";
      json += "\"ssid\":\"" + WiFi.SSID() + "\",";
      json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
      json += "\"gateway\":\"" + WiFi.gatewayIP().toString() + "\",";
      json += "\"dns\":\"" + WiFi.dnsIP().toString() + "\"";
      json += "}";
      server.send(200, "application/json", json);
    } else {
      server.send(200, "application/json", "{\"status\":\"disconnected\"}");
    } });

  // Reconnect (delete saved Wi-Fi)
  server.on("/reconnect", []()
            {
    WiFi.disconnect(true);
    server.send(200, "text/plain", "ok"); });

  // Serve your index.html for all unknown URLs
  server.onNotFound([]()
                    {
    File f = LittleFS.open("/index.html", "r");
    if (f) {
        server.streamFile(f, "text/html");
        f.close();
    } else {
        server.send(404, "text/plain", "Page not found");
    } });

  // Android
  server.on("/generate_204", []()
            { server.sendHeader("Location", "/"); server.send(302, "text/plain", ""); });

  // Windows
  server.on("/connecttest.txt", []()
            { server.sendHeader("Location", "/"); server.send(302, "text/plain", ""); });

  // iOS / macOS
  server.on("/hotspot-detect.html", []()
            { server.sendHeader("Location", "/"); server.send(302, "text/plain", ""); });

  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  unsigned long currentMillis = millis();

  // Check for OTA update every 1 minute
  if (currentMillis - lastUpdateCheck >= updateInterval)
  {
    Serial.println("Checking for OTA update...");
    checkForUpdates();
    lastUpdateCheck = currentMillis;
  }
  // Read DHT11 every 2 seconds
  if (millis() - lastRead > 2000)
  {
    sensor.read(lastTemp, lastHum);
    lastRead = millis();
    Serial.printf("Temp: %.1f °C, Hum: %.1f %%\n", lastTemp, lastHum);
  }
  server.handleClient();
}

#include <time.h>

String sendHumidity(float humidity)
{
  WiFiClientSecure client;
  client.setInsecure(); // skip certificate verification for testing

  // Get current UTC time as ISO8601
  time_t now = time(nullptr);
  struct tm *ptm = gmtime(&now);
  char isoTime[25];
  strftime(isoTime, sizeof(isoTime), "%Y-%m-%dT%H:%M:%SZ", ptm);

  if (!client.connect(devDomain, httpsPort))
  {
    Serial.println("[Humidity] Connection failed!");
    return "❌ Connection failed";
  }

  // GraphQL mutation JSON with actual time and temperature, signalStrength as int
  int signalStrength = (int)humidity;
  String json =
      "{"
      "\"query\":\"mutation { "
      "ingestDevState(data: [{"
      "firedAt: \\\"" +
      String(isoTime) + "\\\", "
                        "signalStrength: " +
      String(signalStrength) +
      "}]) { success }"
      "}\""
      "}";

  // Build HTTP POST request
  String request = "POST /graphql HTTP/1.1\r\n";
  request += "Host: " + String(devDomain) + "\r\n";
  request += "Content-Type: application/json\r\n";
  request += "Authorization: Bearer " + String(bearerToken) + "\r\n"; // Bearer token
  request += "Content-Length: " + String(json.length()) + "\r\n";
  request += "Connection: close\r\n\r\n";
  request += json;

  client.print(request);

  // Read response headers
  while (client.connected())
  {
    String line = client.readStringUntil('\n');
    if (line == "\r")
      break;
  }

  // Read response body
  String response = client.readString();
  Serial.println("[Humidity] Server response: " + response);

  // Basic check if mutation succeeded
  if (response.indexOf("\"status\"") != -1)
  {
    return "✅ Humidity sent successfully";
  }
  else
  {
    return "❌ Failed to send humidity";
  }
}

void checkForUpdates()
{
  WiFiClientSecure client;
  client.setInsecure(); // for now, skip SSL verification

  String url = "https://github.com/Chewbacca-14/Sumbre/blob/main/firmware.bin";

  Serial.println("Checking for updates from: " + url);

  t_httpUpdate_return ret = ESPhttpUpdate.update(client, url);

  switch (ret)
  {
  case HTTP_UPDATE_FAILED:
    Serial.printf("❌ Update failed: %s\n", ESPhttpUpdate.getLastErrorString().c_str());
    break;
  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("ℹ️ No updates available");
    break;
  case HTTP_UPDATE_OK:
    Serial.println("✅ Update successful, rebooting...");
    break;
  }
}
