
//#define DEBUG 1

#include <SPI.h>
#include <LowPower.h>
#include <Wire.h>
//#include <DHT.h>
#include <Adafruit_BMP085.h>

#include <RH_RF95.h>

#ifdef DEBUG == 1
//#include <printf.h>
#endif 

#define DHT_PIN 5     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)

#define POWER_DHT_PIN 6
#define POWER_BPM_PIN 7
#define POWER_VMETR_PIN 4
#define V_PIN 3
#define LED_PIN 8

    

// Hardware configuration

//DHT dht(DHT_PIN, DHTTYPE);
Adafruit_BMP085 bmp;
RH_RF95 rf95(SS, 3, hardware_spi);

struct Message {
   char id[8] = "NODE0001";
   int32_t pressure;
   int32_t humidity;
   int32_t temperature;
   int32_t voltage;
} msg;


void setup(void) {

  #ifdef DEBUG == 1
  Serial.begin(57600);  
  Serial.flush();
  Serial.println("===Setup===");
  Serial.flush();
  #endif 

  
  pinMode(POWER_DHT_PIN, OUTPUT);
  pinMode(POWER_BPM_PIN, OUTPUT);
  pinMode(POWER_VMETR_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  analogReference(INTERNAL);


  digitalWrite(POWER_BPM_PIN, HIGH);
  LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);
  

  #ifdef DEBUG == 1
  Serial.println("Init BMP");
  Serial.flush();
  #endif
   
  if (!bmp.begin(BMP085_ULTRAHIGHRES)) {
    #ifdef DEBUG == 1
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    Serial.flush();
    #else
    blinkError(2, 1, false);
    #endif
  }

  #ifdef DEBUG == 1
  Serial.println("done.");
  Serial.flush();
  #endif

  digitalWrite(POWER_BPM_PIN, LOW);


  #ifdef DEBUG == 1
  Serial.println("Init radio"); 
  Serial.flush();
  #endif 

  //  Init radio 


  
  if (!rf95.init()) {
    blinkError(2, 2, true);
  } else {
    blinkDebug(1);
  }
  

  rf95.setFrequency(868.45);
  //For RFM95/96/97/98 LORA with useRFO false, 
  // valid values are from +5 to +23.
  rf95.setTxPower(5);
 
  rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128);
  rf95.setPreambleLength(8);

  rf95.setThisAddress(10);
  rf95.setHeaderFrom(10);
  rf95.setHeaderTo(60);
  
  #ifdef DEBUG == 1
  Serial.println("done."); 
  Serial.flush();
  #endif 
  
  //rf95.setModeTx();  
 
  rf95.sleep();
  
  
  LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
}

uint8_t loopCount = 14;

void loop(void){
  
  // We only do something every 7 sleep cycles (15 * 8s = 2m)
  if(loopCount == 14)
  {

    #ifdef DEBUG == 1
    Serial.println("---Wake up---");
    Serial.flush();
    #endif

    readBMP();
    readDHT();

    digitalWrite(POWER_VMETR_PIN, HIGH);
    delay(1);
    // 1.1 / 1024 = 0.00107421875
    // H = 6.6 ( 7.454545454545455 experimental ) 
    // = 0.00708984375 ( 0.719730941704036 )
    // voltage = ((float)analogRead(V_PIN)) * 0.00708984375;
    // 
    
    msg.voltage = (analogRead(V_PIN) * 719730) / 1000000;
    digitalWrite(POWER_VMETR_PIN, LOW);
            
    #ifdef DEBUG == 1
    Serial.print("voltage = "); 
    Serial.println(msg.voltage); 
    Serial.flush();
    #endif


    sendData();
    
    loopCount = 0; 

    #ifdef DEBUG == 1
    Serial.println("sleep");
    Serial.flush(); 
    blinkDebug(1);
    #endif  
  }
 
  #ifdef DEBUG == 1
  Serial.flush();
  #endif
  
  loopCount++;
  
  // Using Low-Power library to put the MCU to Sleep
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}

void sendData() {

    #ifdef DEBUG == 1
    Serial.println("Powering up radio");
    Serial.flush(); 
    #endif

    //Power up the radio    
                 
    LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);

    rf95.send((uint8_t*) &msg, sizeof(msg));
    rf95.waitPacketSent();
    rf95.sleep();


    #ifdef DEBUG == 1
    Serial.println("Transfer done");
    Serial.flush();
    #endif
 
}

void readBMP() {

    #ifdef DEBUG == 1
    Serial.print("reading BMP\r\n"); 
    Serial.flush();
    #endif

    digitalWrite(POWER_BPM_PIN, HIGH);
    //delay(3);
    LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);

    msg.pressure = bmp.readPressure();

    #ifdef DEBUG == 1
    Serial.print("Pressure = ");
    Serial.print(msg.pressure);
    Serial.println(" Pa\r\n");
    Serial.flush();
    #endif

    digitalWrite(POWER_BPM_PIN, LOW);

}

/*
void readDHT () {

    dht.begin();
    //delay(310);
    LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF);
    
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    
    #ifdef DEBUG
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor!");
    } else { 
      Serial.print("Temperature = ");
      Serial.print(temperature);
      Serial.println(" *C");

      Serial.print("Humidity = ");
      Serial.print(humidity);
      Serial.println(" %");
    }
    Serial.flush();
    #endif
}
*/

uint8_t _pin;
uint32_t _maxcycles = microsecondsToClockCycles(1000);
 
#ifdef __AVR
  // Use direct GPIO access on an 8-bit AVR so keep track of the port and bitmask
  // for the digital pin connected to the DHT.  Other platforms will use digitalRead.
  uint8_t _bit, _port;
#endif


class InterruptLock {
  public:
   InterruptLock() {
    noInterrupts();
   }
   ~InterruptLock() {
    interrupts();
   }

};

uint32_t expectPulse(bool level) {
  uint32_t count = 0;
  // On AVR platforms use direct GPIO port access as it's much faster and better
  // for catching pulses that are 10's of microseconds in length:
  #ifdef __AVR
    uint8_t portState = level ? _bit : 0;
    while ((*portInputRegister(_port) & _bit) == portState) {
      if (count++ >= _maxcycles) {
        return 0; // Exceeded timeout, fail.
      }
    }
  // Otherwise fall back to using digitalRead (this seems to be necessary on ESP8266
  // right now, perhaps bugs in direct port access functions?).
  #else
    while (digitalRead(_pin) == level) {
      if (count++ >= _maxcycles) {
        return 0; // Exceeded timeout, fail.
      }
    }
  #endif

  return count;
}

void readDHT() {

  #ifdef DEBUG == 1
  Serial.print("reading BMP"); 
  #endif
  
  digitalWrite(POWER_DHT_PIN, HIGH);
  LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF);
  
  uint8_t _pin = DHT_PIN;
  bool _lastresult;
  
  #ifdef __AVR
    _bit = digitalPinToBitMask(_pin);
    _port = digitalPinToPort(_pin);
  #endif
  pinMode(_pin, INPUT_PULLUP);


  uint8_t data[5];
  data[0] = data[1] = data[2] = data[3] = data[4] = 0;

  // Send start signal.  See DHT datasheet for full signal diagram:
  //   http://www.adafruit.com/datasheets/Digital%20humidity%20and%20temperature%20sensor%20AM2302.pdf

  // Go into high impedence state to let pull-up raise data line level and
  // start the reading process.
  digitalWrite(_pin, HIGH);
  //delay(250);
  LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF);

  // First set data line low for 20 milliseconds.
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
  delay(20);
  //LowPower.powerDown(SLEEP_30MS, ADC_OFF, BOD_OFF);

  uint32_t cycles[80];
  {
    // Turn off interrupts temporarily because the next sections are timing critical
    // and we don't want any interruptions.
    InterruptLock lock;

    // End the start signal by setting data line high for 40 microseconds.
    digitalWrite(_pin, HIGH);
    delayMicroseconds(40);

    // Now start reading the data line to get the value from the DHT sensor.
    pinMode(_pin, INPUT_PULLUP);
    delayMicroseconds(10);  // Delay a bit to let sensor pull data line low.

    // First expect a low signal for ~80 microseconds followed by a high signal
    // for ~80 microseconds again.
    if (expectPulse(LOW) == 0) {
      blinkError(4, 1, false);
      _lastresult = false;
      return _lastresult;
    }
    if (expectPulse(HIGH) == 0) {
      blinkError(4, 2, false);
      _lastresult = false;
      return _lastresult;
    }

    // Now read the 40 bits sent by the sensor.  Each bit is sent as a 50
    // microsecond low pulse followed by a variable length high pulse.  If the
    // high pulse is ~28 microseconds then it's a 0 and if it's ~70 microseconds
    // then it's a 1.  We measure the cycle count of the initial 50us low pulse
    // and use that to compare to the cycle count of the high pulse to determine
    // if the bit is a 0 (high state cycle count < low state cycle count), or a
    // 1 (high state cycle count > low state cycle count). Note that for speed all
    // the pulses are read into a array and then examined in a later step.
    for (int i=0; i<80; i+=2) {
      cycles[i]   = expectPulse(LOW);
      cycles[i+1] = expectPulse(HIGH);
    }
  } // Timing critical code is now complete.

  // Inspect pulses and determine which ones are 0 (high state cycle count < low
  // state cycle count), or 1 (high state cycle count > low state cycle count).
  for (int i=0; i<40; ++i) {
    uint32_t lowCycles  = cycles[2*i];
    uint32_t highCycles = cycles[2*i+1];
    if ((lowCycles == 0) || (highCycles == 0)) {
      blinkError(4, 3, false);
      _lastresult = false;
      return _lastresult;
    }
    data[i/8] <<= 1;
    // Now compare the low and high cycle times to see if the bit is a 0 or 1.
    if (highCycles > lowCycles) {
      // High cycles are greater than 50us low cycle count, must be a 1.
      data[i/8] |= 1;
    }
    // Else high cycles are less than (or equal to, a weird case) the 50us low
    // cycle count so this must be a zero.  Nothing needs to be changed in the
    // stored data.
  }

  // Check we read 40 bits and that the checksum matches.
  if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
    blinkError(4, 4, false);
  }


  int32_t t, h;


      t = data[2] & 0x7F;
      t *= 256;
      t += data[3];
 //     t *= 0.1;
      if (data[2] & 0x80) {
        t *= -1;
      }


      h = data[0];
      h *= 256;
      h += data[1];
//      h *= 0.1;

  msg.humidity = h;
  msg.temperature = t;
  
  digitalWrite(POWER_DHT_PIN, LOW);

  #ifdef DEBUG
  if (isnan(msg.humidity) || isnan(msg.temperature)) {
    Serial.println("Failed to read from DHT sensor!");
  } else { 
    Serial.print("Temperature = ");
    Serial.print(msg.temperature);
    Serial.println(" *C");

    Serial.print("Humidity = ");
    Serial.print(msg.humidity);
    Serial.println(" %");
  }
  Serial.flush();
  #endif

  //pinMode(_pin, INPUT);
 
}



void blinkDebug(byte e) {
    for(byte i = 0; i<e; i++) {
      digitalWrite(LED_PIN, 1);
      LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
      digitalWrite(LED_PIN, 0);
      LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF);
    }
    LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF);
}


void blinkError(byte e, byte f, bool halt) {

  digitalWrite(POWER_DHT_PIN, 0);
  digitalWrite(POWER_BPM_PIN, 0);
  digitalWrite(POWER_VMETR_PIN, 0);
  
  do {
    for(byte i = 0; i<e; i++) {
      digitalWrite(LED_PIN, 1);
      LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);
      digitalWrite(LED_PIN, 0);
      LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF);
    }
    LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
    for(byte i = 0; i<f; i++) {
      digitalWrite(LED_PIN, 1);
      LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);
      digitalWrite(LED_PIN, 0);
      LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF);
    }
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
  } while (halt);
}

void blink() {
      digitalWrite(LED_PIN, 1);
      LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);
      digitalWrite(LED_PIN, 0);
      LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF);
}



