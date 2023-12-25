#include <homekit/homekit.h>
#include <homekit/characteristics.h>


char serial[16] = "XXXXXX\0";

void my_accessory_identify(homekit_value_t _value)
{
    printf("accessory identify\n");
}
 
homekit_characteristic_t ACName = HOMEKIT_CHARACTERISTIC_(NAME, "Fujitsu_AC");
homekit_characteristic_t currentHeatingCoolingState = HOMEKIT_CHARACTERISTIC_(CURRENT_HEATING_COOLING_STATE, 0);
homekit_characteristic_t targetHeatingCoolingState = HOMEKIT_CHARACTERISTIC_(TARGET_HEATING_COOLING_STATE, 0);
homekit_characteristic_t currentTemperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 24.0);
homekit_characteristic_t targetTemperature = HOMEKIT_CHARACTERISTIC_(
    TARGET_TEMPERATURE, 24.0,
    .min_value = (float[]) {18},
    .max_value = (float[]) {30},
    .min_step = (float[]) {1},
);
homekit_characteristic_t temperatureDisplayUnit = HOMEKIT_CHARACTERISTIC_(TEMPERATURE_DISPLAY_UNITS, 0);
homekit_characteristic_t currentRelativeHumidity = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 50.0);
 
homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id = 1, .category = homekit_accessory_category_air_conditioner, .services = (homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics = (homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, serial),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Fujitsu"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, serial),
            HOMEKIT_CHARACTERISTIC(MODEL, "RB Script"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
            NULL
        }),//Define the accessory.
        HOMEKIT_SERVICE(THERMOSTAT, .primary = true, .characteristics = (homekit_characteristic_t*[]) {
            &ACName,
            &currentHeatingCoolingState,
            &targetHeatingCoolingState,
            &currentTemperature,
            &targetTemperature,
            &temperatureDisplayUnit,
            &currentRelativeHumidity,
            NULL
        }),//Add a thermostat to this accessory.
        NULL
    }),
    NULL
};
 
homekit_server_config_t accessory_config = {
    .accessories = accessories,
    .password = "111-11-111"
};
