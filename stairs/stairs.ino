#include <SPI.h>
#include <RH_NRF24.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
RH_NRF24 nrf24(7, 8);

int vibrPin = A0;
int DEFAULT_LIGHT_TIME = 2000;
int DEFAULT_FADE_SPEED = 3;
int DEFAULT_MAX_LIGHT = 400;
int fadeSpeed = DEFAULT_FADE_SPEED;
int MAX_LIGHT = DEFAULT_MAX_LIGHT;
int MIN_LIGHT = 0;
int pir1Pin = 5;
int pir2Pin = 6;
int pir1State = LOW;
int pir2State = LOW;
int enabled = 1;
int valLightSensor = 0;
int fadeAmount = 0;
int lightDirection = 0;
int startFrom = -1;
int pocTimer = 0;
int brightness = MIN_LIGHT;
long prevMillis;
long currMillis;
int blik = 100;
int blikSchod = 0;

void sendMessage(String data) {
  char buf[RH_NRF24_MAX_MESSAGE_LEN];
  data.toCharArray(buf, RH_NRF24_MAX_MESSAGE_LEN);
  nrf24.send((uint8_t *)buf, sizeof(buf));
  nrf24.waitPacketSent();
}

String getValue(String data, char separator, int index, String def) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : def;
}

void setup() {
  Serial.begin(9600);

  if (!nrf24.init())
    Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  if (!nrf24.setChannel(1))
    Serial.println("setChannel failed");
  if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm))
    Serial.println("setRF failed");

  SPI.begin();

  pwm.begin();
  pwm.setPWMFreq(400);

  pinMode(pir1Pin, INPUT);
  pinMode(pir2Pin, INPUT);
  pinMode(vibrPin, INPUT) ;

  prevMillis = 0;
}

void loop() {
  /*knockVal = digitalRead(knockPin);
    if (knockVal == LOW)
    {
    Serial.println("knock");
    }*/

  /*
      for (uint8_t x=0; x < 14; x++) {
       for (uint8_t a=0; a < 14; a++) {
         if (a==x) {
           pwm.setPWM(a, 0, 2000 );
           delay(200);
         }
         else {
           pwm.setPWM(a, 0, 0 );
         }
       }
     }

    for (uint8_t x=1; x < 13; x++) {
       for (uint8_t a=1; a < 13; a++) {
         if (a==x) {
           pwm.setPWM(13-a, 0, 2000 );
           delay(200);
         }
         else {
           pwm.setPWM(a, 0, 0 );
         }
       }
     }
  */

  if (nrf24.available()) {
    uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (nrf24.recv(buf, &len)) {
      String data = String((char*)buf);
      Serial.println(data);
      if (data.startsWith("schody-blik")) {
        blik = 1000;
        sendMessage(data);
      }
      else if (data.startsWith("schody-lt")) {
        DEFAULT_LIGHT_TIME = getValue(data, ':', 1, "2000").toInt();
        sendMessage(data);
      }
      else if (data.startsWith("schody-l")) {
        MAX_LIGHT = getValue(data, ':', 1, "400").toInt();
        sendMessage(data);
      }
      else if (data.startsWith("schody-s")) {
        DEFAULT_FADE_SPEED = getValue(data, ':', 1, "5").toInt();
        sendMessage(data);
      }
      else if (data.startsWith("schody-e")) {
        enabled = getValue(data, ':', 1, "1").toInt();;
        sendMessage(data);
      }
    }
  }

  if (blik > 0) {
    blik--;
    if (blik > 500) {
      for (int x = 0; x < 14; x++) pwm.setPWM(x, 0, 4000);
      delay(100);
      for (int x = 0; x < 14; x++) pwm.setPWM(x, 0, 0);
      delay(90);
    }
    else if (blik > 200) {
      if (blikSchod > 13) {
        blikSchod = 0;
      }
      for (int x = 0; x < 14; x++) {
        if (x == blikSchod) {
          pwm.setPWM(x, 0, 4000);
        }
        else {
          pwm.setPWM(x, 0, 0);
          //Serial.println("x");
        }
      }

      blikSchod++;
      delay(100);
    }
  }
  else {
    if (fadeAmount == 1 || fadeAmount == -1) {
      long measurement = pulseIn(vibrPin, HIGH);
      if (measurement > 100) {
          fadeSpeed = DEFAULT_FADE_SPEED * 5;
      }
      
      brightness = brightness + (fadeAmount * (DEFAULT_MAX_LIGHT/MAX_LIGHT)  * fadeSpeed);

      if (brightness <= MIN_LIGHT) brightness = MIN_LIGHT;
      if (brightness >= MAX_LIGHT) brightness = MAX_LIGHT;
      //Serial.println(brightness);

      if (startFrom < 0) {
        fadeAmount = 0;
        startFrom = 0;
      }
      else if (startFrom > 13) {
        fadeAmount = 0;
        startFrom = 13;
      }
      else {
        double x = brightness;
        x = sqrt(x);
  
        pwm.setPWM(startFrom, 0, (int)(x * 200)); // 204.8

        if (lightDirection > 0) {
          if (brightness <= MIN_LIGHT && fadeAmount < 0) {
            startFrom += lightDirection;
            brightness = MAX_LIGHT;
            if (startFrom == 0) lightDirection = 0;
          }
          if (brightness >= MAX_LIGHT && fadeAmount > 0) {
            startFrom += lightDirection;
            brightness = MIN_LIGHT;
          }
        }
        else if (lightDirection < 0) {
          if (brightness <= MIN_LIGHT && fadeAmount < 0) {
            startFrom += lightDirection;
            brightness = MAX_LIGHT;
          }
          if (brightness >= MAX_LIGHT && fadeAmount > 0) {
            startFrom += lightDirection;
            brightness = MIN_LIGHT;
          }
        }
      }
    }
    else {
      if (pocTimer == 0) {
        fadeSpeed = DEFAULT_FADE_SPEED;
        pir1State = LOW;
        pir2State = LOW;
      }
    }

    if (pocTimer > 0) {
      pocTimer--;

      if (pocTimer <= 0) {
        Serial.println("vypinam ");
        fadeAmount = -1;
        brightness = MAX_LIGHT;

        if (startFrom == 0) {
          startFrom = 13;
          lightDirection = -1;
        } else if (startFrom == 13) {
          startFrom = 0;
          lightDirection = 1;
        }
      }
    }

    int valPir1Sensor = digitalRead(pir1Pin);
    int valPir2Sensor = digitalRead(pir2Pin);

    if (enabled) {
      if (valPir1Sensor == HIGH && pir1State == LOW) {
        Serial.println("xx1");
        pir1State = HIGH;
        pir2State = LOW;

        //if (pocTimer >= 5000) {} //startTurnOff = 1;
        if (pocTimer == 0) {
          fadeAmount = 1;
          lightDirection = +1;
          startFrom = 0;
          pocTimer = DEFAULT_LIGHT_TIME;

          sendMessage("schody-pirup");
        }
      }
      else if (valPir2Sensor == HIGH && pir2State == LOW) {
        Serial.println("xx2");
        pir1State = LOW;
        pir2State = HIGH;
        //Serial.print("-> "); Serial.print(pocTimer); Serial.print("  "); Serial.println(fadeAmount);
        //if () {}  //startTurnOff = 1;
        if (pocTimer == 0) {

          fadeAmount = 1;
          lightDirection = -1;
          startFrom = 13;
          pocTimer = DEFAULT_LIGHT_TIME;

          sendMessage("schody-pirdown");
        }
      }
    }
  }

  delay(10);
}
