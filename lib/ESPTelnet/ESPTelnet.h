
#pragma once

#include <Arduino.h>
#include <functional>
#if defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
using WebServerClass = WebServer;
#elif defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#endif

class ESPTelnet : public Stream {
  typedef std::function<void(String str)> CallbackFunction;

 public:
  ESPTelnet();
  explicit ESPTelnet(WebServerClass &webServer);

  bool begin(uint16_t port, int reuse_enable = 1, bool nodelay = true);
  void loop();
  void stop();

  int    available();
  int    read();
  int    peek();
  void   flush();
  size_t write(uint8_t);

  String getIP() const;
  String getLastAttemptIP() const;

  void onConnect(CallbackFunction f);
  void onConnectionAttempt(CallbackFunction f);
  void onReconnect(CallbackFunction f);
  void onDisconnect(CallbackFunction f);

 protected:
  WebServerClass _server;
  WiFiServer     server;
  WiFiClient     client;
  boolean        isConnected;
  String         ip;
  String         attemptIp;

  bool isClientConnected(WiFiClient client);

  CallbackFunction _on_connect;
  CallbackFunction _on_reconnect;
  CallbackFunction _on_disconnect;
  CallbackFunction _on_connection_attempt;
};
