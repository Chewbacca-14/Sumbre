#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ESP8266Ping.h>

const char *ssid = "sumbre";
const char *password = "adaptine";

ESP8266WebServer server(80);

void setup()
{
  Serial.begin(115200);

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
    if (Ping.ping("8.8.8.8", 3))
      server.send(200, "text/plain", "✅ Ping success");
    else
      server.send(200, "text/plain", "❌ Ping failed"); });

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
  server.handleClient();
}
