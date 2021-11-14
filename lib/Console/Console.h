/*
The MIT License (MIT)

Copyright (c) 2020-2022 riraosan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <Arduino.h>
#include <Button2.h>
#include <ESP32Servo.h>
#include <SimpleCLI.h>
#include <Ticker.h>

enum class MESSAGE : int {
  _NOTHING_PUSH,
  _SINGLE_CLICK,
  _DOUBLE_CLICK,
  _TRIPLE_CLICK,
  _LONG_CLICK,
  _TIMER_CLICK,
  _TIMER_ONCE,
  _USB_ON,
  _USB_OFF,
  _SET_INIT_ANGLE,
  _SET_PUSH_ANGLE,
  _SET_WIDTH,
  _SET_INTERVAL,
  _RESET,
  _PRINT_SETTINGS,
  _PRINT_HELP,
  _MSG_MAX
};

class Console {
 public:
  Console(HardwareSerial& serial) : _totalCount(0),
                                    _setCount(0),
                                    _interval(0),
                                    _pulseWidth(0),
                                    _initAngle(0),
                                    _pushAngle(0),
                                    _usbSwitchNum(0),
                                    _servoNum(0),
                                    _counts("\x1b[31m times : __VALUE__\x1b[0m\r\n") {
    _serial = &serial;
  }

  static void push_button(void) {
    _message = MESSAGE::_TIMER_CLICK;
  }

  static void handler(Button2& btn) {
    switch (btn.getClickType()) {
      case SINGLE_CLICK:
        _message = MESSAGE::_SINGLE_CLICK;
        break;
      case DOUBLE_CLICK:
        _serial->print("double ");
        _message = MESSAGE::_DOUBLE_CLICK;
        break;
      case TRIPLE_CLICK:
        _serial->print("triple ");
        _message = MESSAGE::_TRIPLE_CLICK;
        break;
      case LONG_CLICK:
        _serial->print("long");
        _message = MESSAGE::_LONG_CLICK;
        break;
    }
    _serial->print("click");
    _serial->print(" (");
    _serial->print(btn.getNumberOfClicks());
    _serial->println(")");
  }

  static void commandCallback(cmd* c) {
    Command cmd(c);
    String  cmdName(cmd.getName());

    Argument arg1 = cmd.getArgument(0);

    _value = arg1.getValue().toInt();

    if (cmdName == "once") {
      _message = MESSAGE::_TIMER_ONCE;
    } else if (cmdName == "init") {
      _message = MESSAGE::_SET_INIT_ANGLE;
    } else if (cmdName == "push") {
      _message = MESSAGE::_SET_PUSH_ANGLE;
    } else if (cmdName == "width") {
      _message = MESSAGE::_SET_WIDTH;
    } else if (cmdName == "interval") {
      _message = MESSAGE::_SET_INTERVAL;
    } else if (cmdName == "timer") {
      if (arg1.getValue() == "start") {
        _message = MESSAGE::_LONG_CLICK;
      } else if (arg1.getValue() == "stop") {
        _message = MESSAGE::_SINGLE_CLICK;
      }
    } else if (cmdName == "usb") {
      if (arg1.getValue() == "on") {
        _message = MESSAGE::_USB_ON;
      } else if (arg1.getValue() == "off") {
        _message = MESSAGE::_USB_OFF;
      }
    } else if (cmdName == "settings") {
      _message = MESSAGE::_PRINT_SETTINGS;
    } else if (cmdName == "reset") {
      _message = MESSAGE::_RESET;
    } else if (cmdName == "?") {
      _message = MESSAGE::_PRINT_HELP;
    }
  }

  // Callback in case of an error
  static void errorCallback(cmd_error* e) {
    CommandError cmdError(e);

    _serial->print("ERROR: ");
    _serial->println(cmdError.toString());

    if (cmdError.hasCommand()) {
      _serial->print("Did you mean \"");
      _serial->print(cmdError.getCommand().toString());
      _serial->println("\"?");
    }
  }

  void setup(uint8_t servoPin = 3, uint8_t buttonPin = 8, uint8_t usbswitchPin = 7) {
    _servoNum     = servoPin;
    _usbSwitchNum = usbswitchPin;

    _button.setClickHandler(handler);
    _button.setLongClickHandler(handler);
    _button.setDoubleClickHandler(handler);
    _button.setTripleClickHandler(handler);
    _button.begin(buttonPin);

    pinMode(_usbSwitchNum, OUTPUT);
    digitalWrite(_usbSwitchNum, LOW);

    _servo.attach(_servoNum);
  }

  void printCommandList(void) {
    _serial->println("# AutoPush Application");
    _serial->println("Type: once");
    _serial->println("Type: init {{degree}}, push {{degree}}");
    _serial->println("Type: width {{ms}}, interval {{ms}} (800ms < width <= interval)");
    _serial->println("Type: timer {{start / stop}}");
    _serial->println("Type: usb {{on / off}}");
    _serial->println("Type: settings");
    _serial->println("Type: reset");
    _serial->println("Type: ?");
    _serial->print("$ ");
  }

  void printSettings(void) {
    _serial->println("");
    _serial->println("Init Angle    : " + String(_initAngle) + "[degree]");
    _serial->println("Push Angle    : " + String(_pushAngle) + "[degree]");
    _serial->println("On Time       : " + String(_pulseWidth) + "[ms]");
    _serial->println("Interval Time : " + String(_interval) + "[ms]");
    _serial->println("Period        : " + String(_interval + _pulseWidth) + "[ms]");
    _serial->print("$ ");
  }

  void begin(uint32_t width = 800, uint32_t interval = 0, uint32_t initAngle = 85, uint32_t pushAngle = 65) {
    _pulseWidth = width;
    _interval   = interval;
    _initAngle  = initAngle;
    _pushAngle  = pushAngle;

    _cli.setOnError(errorCallback);
    _tool        = _cli.addSingleArgCmd("once", commandCallback);
    _cmdInit     = _cli.addSingleArgCmd("init", commandCallback);
    _cmdPush     = _cli.addSingleArgCmd("push", commandCallback);
    _cmdWidth    = _cli.addSingleArgCmd("width", commandCallback);
    _cmdInterval = _cli.addSingleArgCmd("interval", commandCallback);
    _cmdTimer    = _cli.addSingleArgCmd("timer", commandCallback);
    _usb         = _cli.addSingleArgCmd("usb", commandCallback);
    _cmdSettings = _cli.addSingleArgCmd("settings", commandCallback);
    _cmdReset    = _cli.addSingleArgCmd("reset", commandCallback);
    _cmdHelp     = _cli.addSingleArgCmd("?", commandCallback);

    _servo.write(_initAngle);  // off

    delay(500);

    printCommandList();
  }

  void parse(String command) {
    _cli.parse(command);
  }

  void update() {
    _button.loop();

    switch (_message) {
      case MESSAGE::_TIMER_ONCE:
        _servo.write(_pushAngle);
        delay(_pulseWidth / 2);  // ON Time
        _servo.write(_initAngle);
        delay(_pulseWidth / 2);  // Off Time

        _message = MESSAGE::_NOTHING_PUSH;
        break;
      case MESSAGE::_LONG_CLICK:
        _timer.attach_ms(_pulseWidth + _interval, push_button);
        _serial->println("Starting");

        _message = MESSAGE::_NOTHING_PUSH;
        break;
      case MESSAGE::_SINGLE_CLICK:
      case MESSAGE::_DOUBLE_CLICK:
      case MESSAGE::_TRIPLE_CLICK:
        _serial->print("Total Count: ");
        _serial->println(_totalCount);
        _serial->println("end");
        _serial->print("$ ");

        _setCount   = 0;
        _totalCount = 0;

        _timer.detach();

        _message = MESSAGE::_NOTHING_PUSH;
        break;
      case MESSAGE::_TIMER_CLICK:
        _servo.write(_pushAngle);
        delay(_pulseWidth / 2);  // ON Time
        _servo.write(_initAngle);
        delay(_pulseWidth / 2);  // Off Time

        _counts.replace("__VALUE__", String(_totalCount++));
        _serial->print(_counts);

        _message = MESSAGE::_NOTHING_PUSH;
        break;
      case MESSAGE::_USB_ON:
        digitalWrite(_usbSwitchNum, HIGH);

        _serial->println("");
        _serial->println("USB connection on.");
        _serial->print("$ ");

        _message = MESSAGE::_NOTHING_PUSH;
        break;
      case MESSAGE::_USB_OFF:
        digitalWrite(_usbSwitchNum, LOW);

        _serial->println("");
        _serial->println("USB connection off.");
        _serial->print("$ ");

        _message = MESSAGE::_NOTHING_PUSH;
        break;
      case MESSAGE::_SET_INIT_ANGLE:
        if (90 <= _value && _value <= 120) {
          _serial->println("OK");
          _initAngle = _value;
          _servo.write(_initAngle);
        } else {
          _serial->println("invalid value");
        }
        _serial->print("$ ");

        _message = MESSAGE::_NOTHING_PUSH;
        break;
      case MESSAGE::_SET_PUSH_ANGLE:
        if (5 < (_initAngle - _value) && (_initAngle - _value) < 30) {
          _serial->println("OK");
          _pushAngle = _value;
        } else {
          _serial->println("invalid value");
        }
        _serial->print("$ ");

        _message = MESSAGE::_NOTHING_PUSH;
        break;
      case MESSAGE::_SET_WIDTH:
        if (700 < _value) {
          _serial->println("OK");
          _pulseWidth = _value;
        } else {
          _serial->println("invalid value");
        }
        _serial->print("$ ");

        _message = MESSAGE::_NOTHING_PUSH;
        break;
      case MESSAGE::_SET_INTERVAL:
        if (_pulseWidth <= _value) {
          _serial->println("OK");
          _interval = _value;
        } else {
          _serial->println("invalid value");
        }
        _serial->print("$ ");

        _message = MESSAGE::_NOTHING_PUSH;
        break;
      case MESSAGE::_RESET:
        ESP.restart();
        _message = MESSAGE::_RESET;
        break;
      case MESSAGE::_PRINT_HELP:
        printCommandList();
        _message = MESSAGE::_NOTHING_PUSH;
        break;
      case MESSAGE::_PRINT_SETTINGS:
        printSettings();
        _message = MESSAGE::_NOTHING_PUSH;
        break;
      default:
        break;
    }
    delay(1);
  }

 private:
  uint32_t        _totalCount;
  uint32_t        _setCount;
  uint32_t        _interval;
  uint32_t        _pulseWidth;
  uint32_t        _initAngle;
  uint32_t        _pushAngle;
  uint8_t         _usbSwitchNum;
  uint8_t         _servoNum;
  static uint32_t _value;

  String _counts;

  // for callback
  static MESSAGE _message;

  // lib
  Button2                _button;
  Servo                  _servo;
  Ticker                 _timer;
  static HardwareSerial* _serial;

  SimpleCLI _cli;
  Command   _tool;
  Command   _usb;
  Command   _cmdTimer;
  Command   _cmdInit;
  Command   _cmdPush;
  Command   _cmdPeriod;
  Command   _cmdReset;
  Command   _cmdSettings;
  Command   _cmdWidth;
  Command   _cmdInterval;
  Command   _cmdHelp;
};

HardwareSerial* Console::_serial  = nullptr;
uint32_t        Console::_value   = 0;
MESSAGE         Console::_message = MESSAGE::_NOTHING_PUSH;
