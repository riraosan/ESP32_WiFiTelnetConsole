
#include <WiFiBridge.h>

#define M5_ATOM

//Serial, Serial1, Serial2
static WiFiBridge app(Serial1);

void setup() {
#if defined(M5_ATOM)
  app.begin();
#elif defined(M5_STICK)
  app.begin(115200, 33, 32, SERIAL_8N1);
#endif
}

void loop() {
  app.update();
}
