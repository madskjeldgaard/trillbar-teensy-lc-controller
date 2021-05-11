#include <Trill.h>
#include <array>

#define DEBUG false

Trill trillSensor;
boolean touchActive = false;

constexpr uint8_t speed = 2; // 0 to 3 (fast to slowest)
constexpr uint8_t bits = 11; // 9 to 16 bits

// Calibrate per sensor
constexpr auto maxLength = 3200;
constexpr auto maxFingerSize = 4000;

// Store state in array
std::array<int, 10> state;
std::array<int, 10> touchTrigger;

constexpr auto triggerChannel = 1;
constexpr auto locationChannel = 2;
constexpr auto touchChannel = 3;

constexpr auto maxMidiVal = 16363;

// Expects 14 bit values in range 0 - 16383
void send14BitMidi(int ccNum, int value, int channel) {
  // Throw away everything above 14 bits;
  // Clip values
  if (value > maxMidiVal) {
    value = maxMidiVal;
  } else if (value < 0) {
    value = 0;
  }

  // Lower 7 bits of signal
  auto lowBitVal = value & 0x7F;

  // Upper 7 bits of signal
  auto highBitVal = (value >> 7) & 0x7F;

  usbMIDI.sendControlChange(ccNum + 32, lowBitVal, channel);
  usbMIDI.sendControlChange(ccNum, highBitVal, channel);
}

;

// Linear scaling
template <typename T> T linlin(T x, T in_min, T in_max, T out_min, T out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Midi functionality
void sendTouch(size_t fingerNum, int touch) {
  auto value = linlin(touch, 0, maxFingerSize, 0, maxMidiVal);
  send14BitMidi(fingerNum, value, touchChannel);
};

void sendLocation(size_t fingerNum, int location) {
  const auto value = linlin(location, 0, maxLength, 0, maxMidiVal);
  send14BitMidi(fingerNum, value, locationChannel);
};

void sendTrigger(size_t fingerNum, bool triggerOn) {
  const auto value = triggerOn ? 127 : 0;
  usbMIDI.sendControlChange(fingerNum, value, triggerChannel);
};

void setup() {
#if DEBUG
  while (!Serial)
    ;
  // Initialise serial and touch sensor
  Serial.begin(115200);
#endif

  int ret = trillSensor.setup(Trill::TRILL_BAR);
  if (ret != 0) {
#if DEBUG
    Serial.println("failed to initialise trillSensor");
    Serial.print("Error code: ");
    Serial.println(ret);
#endif
  }

  trillSensor.setScanSettings(speed, bits);

  // Fill state with 0's
  state.fill(0);
  touchTrigger.fill(0);
}

void loop() {
  trillSensor.read();
  touchTrigger.fill(0);

  if (trillSensor.getNumTouches() > 0) {
    for (size_t touchNum = 0; touchNum < trillSensor.getNumTouches();
         touchNum++) {

      auto location = trillSensor.touchLocation(touchNum);
      auto touch = trillSensor.touchSize(touchNum);

      touchTrigger[touchNum] = 1;

      sendTouch(touchNum, touch);
      sendLocation(touchNum, location);

#if DEBUG
      auto normalizedLocation = static_cast<float>(location) / maxLength;
      auto normalizedTouch = static_cast<float>(touch) / maxFingerSize;
      Serial.print(touchNum);
      Serial.print(" ");
      Serial.print(location);
      Serial.print(" ");
      Serial.print(normalizedLocation);
      Serial.print(" ");
      Serial.print(touch);
      Serial.print(" ");
      Serial.print(normalizedTouch);
      Serial.println(" ");
#endif
    }
    touchActive = true;
  } else if (touchActive) {

#if DEBUG
    // Print a single line when touch goes off
    Serial.println("0 0");
#endif

    touchActive = false;
  }

  // Triggers
  for (size_t fingerNum = 0; fingerNum < touchTrigger.size(); fingerNum++) {

    if (touchTrigger[fingerNum] != state[fingerNum]) {
      auto trigUp = (touchTrigger[fingerNum] == 1) ? true : false;

      sendTrigger(fingerNum, trigUp);

#if DEBUG
      Serial.print("Touch ");
      Serial.print(fingerNum);
      Serial.print(" triggered.");
      Serial.print("state: ");
      Serial.println(touchTrigger[fingerNum]);
#endif
    }

    state[fingerNum] = touchTrigger[fingerNum];
  }
}
