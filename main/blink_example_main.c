#include <stdio.h>
#include <inttypes.h>  // For PRIu32
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include <math.h>  // For log 

// For LEDs
#define GREEN_LED GPIO_NUM_27
#define BLUE_LED GPIO_NUM_33
#define RED_LED GPIO_NUM_32

// Define ADC1 Channel 6 for thermistor, Channel 3 for photocell, Channel 0 for battery
#define THERMISTOR_ADC_CHANNEL          ADC_CHANNEL_6
#define LIGHT_ADC_CHANNEL               ADC_CHANNEL_3  // Adjust this for your specific light sensor channel
#define BATTERY_ADC_CHANNEL             ADC_CHANNEL_0
#define EXAMPLE_ADC_ATTEN               ADC_ATTEN_DB_12  // Use 12 dB attenuation for up to 3.3V

// Button GPIOs
#define RESET_BUTTON_GPIO               GPIO_NUM_22  // Replace 25 with your actual GPIO number
#define TILT_SENSOR_GPIO                GPIO_NUM_23

// Thermistor and voltage divider constants
#define VCC                        3.3             // Supply voltage (V)
#define R_FIXED                    10000.0         // Fixed resistor value (Ohms)
#define BETA_COEFFICIENT           3435.0          // Beta value of thermistor (K)
#define TEMPERATURE_NOMINAL        298.15          // Nominal temperature (25°C in Kelvin)
#define RESISTANCE_NOMINAL         10000.0         // Resistance at nominal temperature (Ohms)
#define N_SAMPLES                  10              // Number of samples to average

// Photocell calibration constants 
#define CALIBRATION_M              -0.643    // Example slope from log-log regression
#define CALIBRATION_B              5.64     // Example intercept from log-log regression

// Light intensity threshold for state transitions
#define LIGHT_THRESHOLD            100.0   // Adjust this value based on your sensor's calibration

typedef enum {
    STATE_READY,
    STATE_SENSING,
    STATE_DONE
} pill_state_t;

static const char *TAG = "SENSOR_MONITOR";
static pill_state_t pill_state = STATE_READY;

static volatile int tilt_orientation = 0;  // 0 = Vertical, 1 = Horizontal

// Function for LED control
static void blink_leds(uint8_t state1, uint8_t state2, uint8_t state3)
{
    gpio_set_level(GREEN_LED, state1);
    gpio_set_level(BLUE_LED, state2);
    gpio_set_level(RED_LED, state3);
}

// Task to handle button press for tilt orientation
static void task_tilt_button(void *pvParameters)
{
    gpio_reset_pin(TILT_SENSOR_GPIO);
    gpio_set_direction(TILT_SENSOR_GPIO, GPIO_MODE_INPUT);

    int lastButtonState = 1;

    while (1) {
        int currentState = gpio_get_level(TILT_SENSOR_GPIO);  // Read button state

        if (currentState == 0 && lastButtonState == 1) {  // Button pressed
            tilt_orientation = !tilt_orientation;  // Toggle tilt state (0 = Vertical, 1 = Horizontal)

            vTaskDelay(50 / portTICK_PERIOD_MS);  // Short debounce delay
        }

        lastButtonState = currentState;

        vTaskDelay(10 / portTICK_PERIOD_MS);  // Polling delay
    }
}

// **New Task to handle reset button press**
static void task_reset_button(void *pvParameters)
{
    gpio_reset_pin(RESET_BUTTON_GPIO);
    gpio_set_direction(RESET_BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_pullup_en(RESET_BUTTON_GPIO);    // Enable internal pull-up resistor
    gpio_pulldown_dis(RESET_BUTTON_GPIO); // Disable internal pull-down resistor

    int lastButtonState = 1;  // Assuming pull-up, so unpressed is high, pressed is low

    while (1) {
        int currentState = gpio_get_level(RESET_BUTTON_GPIO);  // Read button state

        if (currentState == 0 && lastButtonState == 1) {  // Button press detected (falling edge)
            
            pill_state = STATE_READY;
            // Reset LEDs
            blink_leds(1, 0, 0);  // Turn on green LED
            ESP_LOGI(TAG, "Reset button pressed, resetting to STATE_READY");
        }

        lastButtonState = currentState;

        vTaskDelay(10 / portTICK_PERIOD_MS);  // Polling delay
    }
}

// Task to read temp, battery voltage, light intensity, and handle states
static void sensor_task(void *pvParameters)
{
    // Initialize LEDs
    gpio_reset_pin(GREEN_LED);
    gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);

    gpio_reset_pin(BLUE_LED);
    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT);

    gpio_reset_pin(RED_LED);
    gpio_set_direction(RED_LED, GPIO_MODE_OUTPUT);

    // Initialize ADC for sensors
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // Configure ADC channels
    adc_oneshot_chan_cfg_t config = {
        .atten = EXAMPLE_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, THERMISTOR_ADC_CHANNEL, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, BATTERY_ADC_CHANNEL, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, LIGHT_ADC_CHANNEL, &config));

    int led_state = 0;  // State for blinking the blue LED (0: OFF, 1: ON)

    while (1) {

        uint32_t time_sec = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;

        // temp reading
        int sum_adc_raw = 0;
        for (int i = 0; i < N_SAMPLES; ++i) {
            int adc_value;
            ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, THERMISTOR_ADC_CHANNEL, &adc_value));
            sum_adc_raw += adc_value;
        }
        int adc_raw = sum_adc_raw / N_SAMPLES;

        // Calculate temperature
        float voltage_v = (float)adc_raw / 4095 * VCC;
        float resistance_thermistor = R_FIXED * voltage_v / (VCC - voltage_v);
        float temperature_kelvin = 1.0 / ((1.0 / BETA_COEFFICIENT) * log(resistance_thermistor / RESISTANCE_NOMINAL) + (1.0 / TEMPERATURE_NOMINAL));
        float temperature_celsius = temperature_kelvin - 273.15;
        float temperature_fahrenheit = (temperature_celsius * 9.0 / 5.0) + 32.0;

        // battery voltage reading
        int battery_adc_raw;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, BATTERY_ADC_CHANNEL, &battery_adc_raw));

        // Calculate battery voltage
        float battery_voltage_v = (float)battery_adc_raw / 4095 * VCC;

        // light reading
        int light_adc_raw;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, LIGHT_ADC_CHANNEL, &light_adc_raw));

        // Convert raw data to voltage then calculate lux (light intensity)
        float light_voltage_v = (float)light_adc_raw / 4095 * VCC;
        float resistance_light = R_FIXED * ((VCC - light_voltage_v) / light_voltage_v);
        double log_r = log10(resistance_light);
        double log_lux = (log_r - CALIBRATION_B) / CALIBRATION_M;
        double lux = pow(10, log_lux);

        // Update the state machine
        switch (pill_state) {
            case STATE_READY:
                // Green LED ON
                blink_leds(1, 0, 0);

                // Transition to STATE_SENSING if the light sensor detects darkness
                if (lux < LIGHT_THRESHOLD) {
                    pill_state = STATE_SENSING;
                    led_state = 1;  // Start with LED ON
                }
                break;

            case STATE_SENSING:
                blink_leds(0, led_state, 0);
                led_state = !led_state;  // Toggle LED state

                // Transition to STATE_DONE if the light sensor detects light again
                if (lux > LIGHT_THRESHOLD) {
                    pill_state = STATE_DONE;
                }
                break;

            case STATE_DONE:
                // Red LED ON
                blink_leds(0, 0, 1);
                break;
        }

        //print data
        ESP_LOGI(TAG, "Time: %" PRIu32 " s, Light: %.2f Lux, Temp: %.2f °F, Battery: %.2f V, Tilt: %s",
                 time_sec, lux, temperature_fahrenheit, battery_voltage_v, tilt_orientation ? "Horizontal" : "Vertical");

        // 2s delay between reports
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    // Clean up (this part will never be reached in this loop, but included for completeness)
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
}

void app_main(void)
{
    // Create tasks
    xTaskCreate(sensor_task, "Sensor Task", 4096, NULL, 5, NULL);
    xTaskCreate(task_tilt_button, "Task Tilt Button", 2048, NULL, 5, NULL);
    xTaskCreate(task_reset_button, "Task Reset Button", 2048, NULL, 5, NULL);  
}
