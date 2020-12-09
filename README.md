# LoRa Gateway 

LoRa to MQTT Gateway based on Raspberry Pi

## Schematic

Radio module: RFM95W Low Power Long Range Transceiver Module

wiring:

RFM95W | Rapberry Pi 1 Model B 
------------ | -------------
3.3 | 1 ( 3.3 VDC Power )
GND | 6 ( 0V Ground )
DIO0 | 7 ( GPIO 7 )
RESET | 11 ( GPIO 0 )
NSS | 22 ( GPIO 6 )
MOSI | 19 ( MOSI )
MISO | 21 ( MISO )
SCK | 23 ( SCLK )
       
Transmitter node https://github.com/denysobukh/environment-sensor-node
