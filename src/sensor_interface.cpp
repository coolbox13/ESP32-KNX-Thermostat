#include "sensor_interface.h"
#include <esp_log.h>

static const char* TAG = "SensorInterface";

// This file is intentionally empty as all sensor implementations should be
// in their own respective .cpp files (e.g., bme280_sensor_interface.cpp)

// The base SensorInterface is an abstract class, so no implementation needed here