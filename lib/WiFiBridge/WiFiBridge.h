
#pragma once

#include <Arduino.h>
#include <AutoConnect.h>
#include <ESPTelnet.h>
#include <WebServer.h>
#include <Console.h>

class WiFiBridge {
 public:
  WiFiBridge(HardwareSerial &serial) : _portal(_server),
                                       _console(serial) {
    _serial = &serial;
  }

  void setupSerial(unsigned long speed, uint32_t config, int8_t rxPin, int8_t txPin) {
    _serial->begin(speed, config, rxPin, txPin);
    _serial->flush();
    delay(500);
  }

  bool isConnected() {
    return (WiFi.status() == WL_CONNECTED);
  }

  void setupTelnet() {
    _telnet.onConnect([&](String ip) {
      _telnet.println("\n- Telnet: Welcome. Your IP address is " + ip);
    });

    _telnet.onConnectionAttempt([&](String ip) {
      _telnet.println("- Telnet: " + ip + " tried to connected");
    });

    _telnet.onReconnect([&](String ip) {
      _telnet.println("- Telnet: " + ip + " reconnected");
    });

    _telnet.onDisconnect([&](String ip) {
      _telnet.println("- Telnet: " + ip + " disconnected");
    });

    if (!_telnet.begin(23)) {
      ESP.restart();
    } else {
      log_i("Success to begin telnet.");
    }
  }

  void begin(unsigned long speed = 115200, uint32_t config = SERIAL_8N1, int8_t rxPin = 32, int8_t txPin = 26) {
    setupSerial(speed, config, rxPin, txPin);

    _server.on("/", [&]() {
      char content[] = "Welcome to ATOM Lite";
      _server.send(200, "text/plain", content);
    });

    if (_portal.begin()) {
      setupTelnet();
    } else {
      ESP.restart();
    }
  }

  void update() {
    _portal.handleClient();
    _telnet.loop();
    _console.update();

    if (_telnet.available() > 0) {
      _console.parse(_telnet.readStringUntil('\n'));
      _serial->print("$ ");
    }

    if (_serial->available() > 0) {
      _telnet.print(_serial->readStringUntil('\n'));
    }
  }

 private:
  WebServer       _server;
  ESPTelnet       _telnet;
  AutoConnect     _portal;
  HardwareSerial *_serial;
  Console         _console;
};
