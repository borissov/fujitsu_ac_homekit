/*
 * HomeKit Remote control for old Fujitsu AC via Infrared
 * 
 * hadware list:
 * controller: NodeMCU v2
 * transistor: BC547 
 * ir led: TSAL6100 
 * temperature sensor: DHT11
 *
 * credits to https://blog.judgelight.xyz/2022/06/arduinoesp8266%E5%BC%80%E5%8F%91homekit%E5%85%A5%E9%97%A8%E6%8C%87%E5%8D%97/
 */


#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <arduino_homekit_server.h> //fetch latest from gh
#include <IRremoteESP8266.h> 
#include <ir_Fujitsu.h>
#include <DHT.h>
#include "wifi_info.h"


#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

extern "C" homekit_server_config_t accessory_config;
extern "C" homekit_characteristic_t currentHeatingCoolingState;
extern "C" homekit_characteristic_t targetHeatingCoolingState;
extern "C" homekit_characteristic_t currentTemperature;
extern "C" homekit_characteristic_t targetTemperature;
extern "C" homekit_characteristic_t temperatureDisplayUnit;
extern "C" homekit_characteristic_t currentRelativeHumidity;
static long long nextNotifyTime = 0;
const int IRPin = 4; //d2.
const int DHTPin = 5; //d1.
const int LEDpin = 2; //led


IRFujitsuAC AC(IRPin);
DHT DHTSensor(DHTPin, DHT11);

void ACModeSetter(const homekit_value_t value)
{
    currentHeatingCoolingState.value.uint8_value = value.uint8_value;
    targetHeatingCoolingState.value.uint8_value = value.uint8_value;
    updateac();
}

void temperatureDisplayUnitSetter(const homekit_value_t value)
{
    temperatureDisplayUnit.value.uint8_value = value.uint8_value;
    homekitNotify();
}

void updateac()
{
    int state = round(targetHeatingCoolingState.value.uint8_value);
    int targetTemp = round(targetTemperature.value.float_value);

    switch (state)
    {
        case 0:
            AC.off();
            AC.send();
            digitalWrite(LEDpin, HIGH);
            break;
        case 1:
            AC.on();
            AC.setSwing(kFujitsuAcSwingOff);
            AC.setMode(kFujitsuAcModeHeat);
            AC.setFanSpeed(kFujitsuAcFanQuiet);
            AC.setTemp(targetTemp);
            AC.setCmd(kFujitsuAcCmdTurnOn);
            AC.send();
            digitalWrite(LEDpin, LOW);
            break;
        case 2:
            AC.on();
            AC.setSwing(kFujitsuAcSwingOff);
            AC.setMode(kFujitsuAcModeCool);
            AC.setFanSpeed(kFujitsuAcFanQuiet);
            AC.setTemp(targetTemp);
            AC.setCmd(kFujitsuAcCmdTurnOn);
            AC.send();
            digitalWrite(LEDpin, LOW);
            break;
        case 3:
            AC.on();
            AC.setSwing(kFujitsuAcSwingOff);
            AC.setMode(kFujitsuAcModeAuto);
            AC.setFanSpeed(kFujitsuAcFanQuiet);
            AC.setTemp(targetTemp);
            AC.setCmd(kFujitsuAcCmdTurnOn);
            AC.send();
            digitalWrite(LEDpin, LOW);            
            break;
    }

    LOG_D("AC settings: %s", AC.toString().c_str());
    homekitNotify();
}

void ACTemperatureSetter(const homekit_value_t value)
{
    targetTemperature.value.float_value = value.float_value;
    updateac();
}

void homekitNotify()
{
    currentTemperature.value.float_value = (round(DHTSensor.readTemperature() * 10.0) / 10.0) -1;
    currentRelativeHumidity.value.float_value = round(DHTSensor.readHumidity()) * 1.0;
    
    if (!(currentTemperature.value.float_value > 0 && currentTemperature.value.float_value < 100)) currentTemperature.value.float_value = 24.0;//fix for wrong sensor data
    if (!(currentRelativeHumidity.value.float_value > 0 || currentRelativeHumidity.value.float_value < 100)) currentRelativeHumidity.value.float_value = 50.0;
    
    homekit_characteristic_notify(&currentHeatingCoolingState, currentHeatingCoolingState.value);
    homekit_characteristic_notify(&targetHeatingCoolingState, targetHeatingCoolingState.value);
    homekit_characteristic_notify(&targetTemperature, targetTemperature.value);
    homekit_characteristic_notify(&temperatureDisplayUnit, temperatureDisplayUnit.value);
    //memory leak after this...
    homekit_characteristic_notify(&currentTemperature, currentTemperature.value);
    homekit_characteristic_notify(&currentRelativeHumidity, currentRelativeHumidity.value);
}

void homekitSetup()
{
    targetHeatingCoolingState.setter = ACModeSetter;
    targetTemperature.setter = ACTemperatureSetter;
    temperatureDisplayUnit.setter = temperatureDisplayUnitSetter;
    arduino_homekit_setup(&accessory_config);
}

void homekitLoop()
{
    arduino_homekit_loop();
    const uint32_t timer = millis();
    if (timer > nextNotifyTime)
    {
        nextNotifyTime = timer + 2 * 1000;
        homekitNotify();
        LOG_D(
            "Free heap: %d, HomeKit clients: %d Uptime: %d s", 
            ESP.getFreeHeap(), 
            arduino_homekit_connected_clients_count(), 
            int(timer/1000)
        );
    }
}

void setup()
{
    //  homekit_storage_reset();

    wifi_connect();
    AC.begin();
    DHTSensor.begin();
    Serial.begin(115200);

    pinMode(LEDpin, OUTPUT);
    digitalWrite(LEDpin, HIGH);

    delay(1000);
    homekitSetup();
}

void loop()
{
    homekitLoop();
    delay(10);
}
