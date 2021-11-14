
// https://github.com/LennartHennigs/ESPTelnet
// I was inspired by @LenartHennigs ESPTelnet Library
// Thank you, @LenartHennigs!

#include "ESPTelnet.h"

ESPTelnet::ESPTelnet() : isConnected(false),
                         ip(""),
                         attemptIp("") {
}

bool ESPTelnet::begin(uint16_t port, int reuse_enable, bool noDelay) {
  if (WiFi.status() == WL_CONNECTED) {
    server.begin(port, reuse_enable);
    server.setNoDelay(noDelay);
    return true;
  }

  return false;
}

void ESPTelnet::stop() {
  server.stop();
}

bool ESPTelnet::isClientConnected(WiFiClient client) {
#if defined(ARDUINO_ARCH_ESP8266)
  return client.status() == ESTABLISHED;
#elif defined(ARDUINO_ARCH_ESP32)
  return client.connected();
#endif
}

void ESPTelnet::loop() {
  // check if there are any new clients
  if (server.hasClient()) {
    isConnected = true;
    // already a connection?
    if (client && client.connected() && isClientConnected(client)) {
      WiFiClient newClient = server.available();
      attemptIp            = newClient.remoteIP().toString();
      // reconnected?
      if (attemptIp == ip) {
        if (_on_reconnect != NULL)
          _on_reconnect(ip);
        client.stop();
        client = newClient;
        // disconnect the second connection
      } else {
        if (_on_connection_attempt != NULL)
          _on_connection_attempt(ip);
        return;
      }
      // first connection
    } else {
      client = server.available();
      ip     = client.remoteIP().toString();
      if (_on_connect != NULL)
        _on_connect(ip);
      client.setNoDelay(true);
      client.flush();
    }
  }
  // check whether to disconnect
  if (client && isConnected && !isClientConnected(client)) {
    if (_on_disconnect != NULL)
      _on_disconnect(ip);
    isConnected = false;
    ip          = "";
  }
  yield();
}

int ESPTelnet::available() {
  if (client && isClientConnected(client)) {
    return client.available();
  }

  return -1;
}

int ESPTelnet::read() {
  if (client && isClientConnected(client)) {
    return client.read();
  }

  return -1;
}

int ESPTelnet::peek() {
  if (client && isClientConnected(client)) {
    return client.peek();
  }

  return -1;
}

void ESPTelnet::flush() {
  if (client && isClientConnected(client)) {
    client.flush();
  }
}

size_t ESPTelnet::write(uint8_t data) {
  if (client && isClientConnected(client)) {
    return client.write(data);
  }

  return -1;
}

String ESPTelnet::getIP() const {
  return ip;
}

String ESPTelnet::getLastAttemptIP() const {
  return attemptIp;
}

void ESPTelnet::onConnect(CallbackFunction f) {
  _on_connect = f;
}

void ESPTelnet::onConnectionAttempt(CallbackFunction f) {
  _on_connection_attempt = f;
}

void ESPTelnet::onReconnect(CallbackFunction f) {
  _on_reconnect = f;
}

void ESPTelnet::onDisconnect(CallbackFunction f) {
  _on_disconnect = f;
}
