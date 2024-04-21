/*
 * Apple HomeKit remote control for my old Fujitsu AC via infrared
 * 
 * hadware list:
 * controller: NodeMCU v2
 * transistor: BC547 
 * ir led: TSAL6100 
 * temperature sensor: DHT11
 *
 *
 */


#include <Arduino.h>
#include <arduino_homekit_server.h>
#include <homekit/characteristics.h>
#include <IRremoteESP8266.h> 
#include <ir_Fujitsu.h>
#include <DHT.h>
#include "wifi_info.h"

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);





const int IRPin = 4; //d2.
const int DHTPin = 5; //d1.
const int LEDpin = 2; //led
const int buttonPin = 0; //flashButton



extern "C" char serial[16];
extern "C" homekit_server_config_t accessory_config;
extern "C" homekit_characteristic_t currentHeatingCoolingState;
extern "C" homekit_characteristic_t targetHeatingCoolingState;
extern "C" homekit_characteristic_t currentTemperature;
extern "C" homekit_characteristic_t targetTemperature;
extern "C" homekit_characteristic_t temperatureDisplayUnit;
extern "C" homekit_characteristic_t currentRelativeHumidity;
static long long nextNotifyTime = 0;
static long long nextWifiCheckTime = 0;
static long long nextTempNotifyTime = 0;


IRFujitsuAC AC(IRPin);
DHT DHTSensor(DHTPin, DHT11);

void ACModeSetter(const homekit_value_t value)
{
    if(currentHeatingCoolingState.value.uint8_value != value.uint8_value)
    {
        currentHeatingCoolingState.value.uint8_value = value.uint8_value;
        homekit_characteristic_notify(&currentHeatingCoolingState, currentHeatingCoolingState.value);
    }
    if(targetHeatingCoolingState.value.uint8_value != value.uint8_value)
    {
        targetHeatingCoolingState.value.uint8_value = value.uint8_value;
        homekit_characteristic_notify(&targetHeatingCoolingState, targetHeatingCoolingState.value);
    }
    updateac();
}

void ACTemperatureSetter(const homekit_value_t value)
{
    if(targetTemperature.value.float_value != value.float_value)
    {
        targetTemperature.value.float_value = value.float_value;
        homekit_characteristic_notify(&targetTemperature, targetTemperature.value);
        updateac();
    }
}


void temperatureDisplayUnitSetter(const homekit_value_t value)
{
    if(temperatureDisplayUnit.value.uint8_value != value.uint8_value)
    {
        temperatureDisplayUnit.value.uint8_value = value.uint8_value;
        homekit_characteristic_notify(&temperatureDisplayUnit, temperatureDisplayUnit.value);
    }
}

void updateac()
{
    const int targetTemp = round(targetTemperature.value.float_value);

    switch (targetHeatingCoolingState.value.uint8_value)
    {
        case HOMEKIT_TARGET_HEATING_COOLING_STATE_OFF:
            AC.off();
            AC.send();
            digitalWrite(LEDpin, HIGH);
            break;
        case HOMEKIT_TARGET_HEATING_COOLING_STATE_HEAT:
            AC.on();
            AC.setSwing(kFujitsuAcSwingOff);
            AC.setMode(kFujitsuAcModeHeat);
            AC.setFanSpeed(kFujitsuAcFanQuiet);//kFujitsuAcFanAuto
            AC.setTemp(targetTemp);
            AC.setCmd(kFujitsuAcCmdTurnOn);
            AC.send();
            digitalWrite(LEDpin, LOW);
            break;
        case HOMEKIT_TARGET_HEATING_COOLING_STATE_COOL:
            AC.on();
            AC.setSwing(kFujitsuAcSwingOff);
            AC.setMode(kFujitsuAcModeCool);
            AC.setFanSpeed(kFujitsuAcFanQuiet);//kFujitsuAcFanAuto
            AC.setTemp(targetTemp);
            AC.setCmd(kFujitsuAcCmdTurnOn);
            AC.send();
            digitalWrite(LEDpin, LOW);
            break;
        case HOMEKIT_TARGET_HEATING_COOLING_STATE_AUTO:
            AC.on();
            AC.setSwing(kFujitsuAcSwingOff);
            AC.setMode(kFujitsuAcModeAuto);
            AC.setFanSpeed(kFujitsuAcFanAuto);//kFujitsuAcFanQuiet
            AC.setTemp(targetTemp);
            AC.setCmd(kFujitsuAcCmdTurnOn);
            AC.send();
            digitalWrite(LEDpin, LOW);            
            break;
    }

    LOG_D("AC settings: %s", AC.toString().c_str());
}


void homekitTempNotify()
{
    float newTemperature = DHTSensor.readTemperature() -1;
    float newHumidity = DHTSensor.readHumidity();
    
    if (!(newTemperature > 0 && newTemperature < 100)) 
        newTemperature = 24.0;
    if (!(newHumidity > 0 && newHumidity < 100)) 
        newHumidity = 50.0;

    if (currentTemperature.value.float_value != newTemperature)
    {
        currentTemperature.value.float_value = newTemperature;
        homekit_characteristic_notify(&currentTemperature, currentTemperature.value);
    }

    if (currentRelativeHumidity.value.float_value != newHumidity)
    {
        currentRelativeHumidity.value.float_value = newHumidity;
        homekit_characteristic_notify(&currentRelativeHumidity, currentRelativeHumidity.value);
    }
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
        LOG_D(
            "Free heap: %d, HomeKit clients: %d Uptime: %d s", 
            ESP.getFreeHeap(), 
            arduino_homekit_connected_clients_count(), 
            int(timer/1000)
        );
    }

    if (timer > nextTempNotifyTime)
    {
        nextTempNotifyTime = timer + 25 * 1000;
        homekitTempNotify();
        LOG_D(
                "Free heap: %d, HomeKit clients: %d Uptime: %d s", 
                ESP.getFreeHeap(), 
                arduino_homekit_connected_clients_count(), 
                int(timer/1000)
             );
    }

    if (timer > nextWifiCheckTime)
    {
        nextWifiCheckTime = timer + 60 * 1000;
        if(WiFi.status() != WL_CONNECTED)
        {
            delay(10000);
            ESP.restart();
        }
    }


    int buttonState = digitalRead(buttonPin);

    if (buttonState == LOW) 
    {
        homekit_storage_reset();
        ESP.restart();
    }
    



}

void setup()
{
    sprintf(serial, "SN%X\0", ESP.getChipId());

    wifi_connect();
    AC.begin();
    DHTSensor.begin();
    Serial.begin(115200);

    pinMode(LEDpin, OUTPUT);
    pinMode(buttonPin, INPUT);
    digitalWrite(LEDpin, HIGH);

    delay(1000);
    homekitSetup();
}

void loop()
{
    homekitLoop();
    delay(10);
}
