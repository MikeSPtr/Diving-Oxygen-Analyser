#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <RunningAverage.h>
#include <ezButton.h>

// Version
#define VERSION "0.9"

// ADC
#define ADC_GAIN GAIN_SIXTEEN
#define ADC_GAIN_MULTIPLIER 0.0078125f

// Oxygen voltage average
#define RA_OXYGEN_SIZE 20

// Battery voltage average
#define RA_BATTERY_SIZE 20

// Battery
#define BATTERY_LOW_VOLTAGE_V 5.0f
// Battery voltage divider composed with R1 = 10kOhm and R2 = 480kOmm, value measured.
#define BATTER_VOLTAGE_DIVIDER_MULTIPLIER 48.0f

// Calibration
#define AMBIENT_AIR_OXYGEN_FRACTION 20.95
#define MINIMAL_VALID_SENSOR_VOLTAGE_MV 5.0

#define PRE_CALIBRATION_ITERATION_NUMBER 10
#define PRE_CALIBRATION_DELAY_MS 1000
#define PRE_CALIBRATION_INTERVAL_MS 75

#define CALIBRATION_MAX_DEVIATION 1.0
#define CALIBRATION_MAX_ITERATION_NUMBER 100
#define CALIBRATION_ITERATION_INTERVAL_MS 250
#define CALIBRATION_REQUIRED_CONSECUTIVE_CORRECT_VALUE 10

#define CALIBRATION_RESULT_SUCCESS 0
#define CALIBRATION_RESULT_ERROR 1
#define CALIBRATION_RESULT_ERROR_SENSOR_CONNECTION_PROBLEM 2

// Oxygen average
RunningAverage RA_OXYGEN(RA_OXYGEN_SIZE);

// Battery average
RunningAverage RA_BATTERY(RA_BATTERY_SIZE);

// ADC initialization
// Oxygen sensor conneted to differentaial inputs 0 and 1.
// Battery connected to single ended input 3.
Adafruit_ADS1115 ads1115;

// Buttons
ezButton calibrate_button(2);

// Oxygen calculation
double sensor_calibration_voltage;

// Status variables
bool is_calibrating = false;
bool is_calibration_error_occured = false;

void setup_buttons() {
  calibrate_button.setDebounceTime(50);
}

void setup_adc() {
  ads1115.setGain(ADC_GAIN);
  ads1115.begin();
}

void setup_sensor_calculation() {
  RA_OXYGEN.clear();
}

void loop_buttons() {
  if (!is_calibrating || is_calibration_error_occured) {
    calibrate_button.loop();
    if (calibrate_button.isPressed()) {
      calibrate_sensor();
    }
  }
}

void read_sensor_voltage() {
  int16_t millivolts = 0;
  millivolts = ads1115.readADC_Differential_2_3();
  RA_OXYGEN.addValue(millivolts);
}

void read_battery_voltage() {
  int16_t millivolts = 0;
  millivolts =  ads1115.readADC_SingleEnded(0);
  RA_BATTERY.addValue(millivolts);
}

double get_sensor_voltage() {
  return abs(RA_OXYGEN.getAverage());
}

double get_sensor_voltage_mv() {
  return get_sensor_voltage() * (double) ADC_GAIN_MULTIPLIER;
}

float get_battery_voltage_mv() {
  return RA_BATTERY.getAverage() * ADC_GAIN_MULTIPLIER * BATTER_VOLTAGE_DIVIDER_MULTIPLIER;
}

void calibrate_sensor() {
  on_calibration_start();

  /*
    Wait specified time to stabilize oxygen sensor.
  */
  delay(PRE_CALIBRATION_DELAY_MS);

  /*
     Clear current ADC measurements.
  */
  RA_OXYGEN.clear();
  for (int i = 0; i <= PRE_CALIBRATION_ITERATION_NUMBER; i++) {
    delay(PRE_CALIBRATION_INTERVAL_MS);
    read_sensor_voltage();
  }

  /*
     Calibrate sensor by checking if sensor value meet calibration requirement (see below)
     consecutive number (CALIBRATION_REQUIRED_CONSECUTIVE_CORRECT_VALUE) of times.
  */
  int calibration_iteration = 0;
  int consecutive_correct_values = 0;
  do {
    int left_calibration_iteration = CALIBRATION_MAX_ITERATION_NUMBER - calibration_iteration;
    if ((left_calibration_iteration + consecutive_correct_values) < CALIBRATION_REQUIRED_CONSECUTIVE_CORRECT_VALUE) {
      on_calibration_end(CALIBRATION_RESULT_ERROR, NULL);
      return;
    }
    calibration_iteration++;

    /*
       If ADC is MINIMAL_CORRECT_SENSOR_VOLTAGE_VALUE_MV then it means that sensor is not
       connected correctly.
    */
    if (get_sensor_voltage_mv() <= MINIMAL_VALID_SENSOR_VOLTAGE_MV) {
      double sensor_voltage_mv = get_sensor_voltage_mv();
      on_calibration_end(CALIBRATION_RESULT_ERROR_SENSOR_CONNECTION_PROBLEM, &sensor_voltage_mv);
      return;
    }

    /*
       Calibration requirements to state if the sensor is calibrated.

       The sensor values are checked with statistic, standard deviation of measured values
       must be smaller than maximum acceptable deviation (CALIBRATION_MAX_DEVIATION).
    */
    read_sensor_voltage();
    double sensor_standard_deviation = RA_OXYGEN.getStandardDeviation();
    bool meet_calibration_requirements = sensor_standard_deviation < CALIBRATION_MAX_DEVIATION;

    if (meet_calibration_requirements) {
      consecutive_correct_values++;
    } else {
      consecutive_correct_values = 0;
    }

    on_calibration_progress(
      calibration_iteration,
      CALIBRATION_MAX_ITERATION_NUMBER,
      sensor_standard_deviation
    );

    delay(CALIBRATION_ITERATION_INTERVAL_MS);
  } while (
    consecutive_correct_values < CALIBRATION_REQUIRED_CONSECUTIVE_CORRECT_VALUE
  );

  /*
     Read calibrated sensor values.
  */
  for (int i = 0; i <= RA_OXYGEN_SIZE; i++) {
    read_sensor_voltage();
  }
  double calibration_result = RA_OXYGEN.getAverage();
  sensor_calibration_voltage = abs(calibration_result);
  double sensor_calibration_voltage_mv = get_sensor_voltage_mv();
  on_calibration_end(CALIBRATION_RESULT_SUCCESS, &sensor_calibration_voltage_mv);
}

double analyse_sensor_voltage_to_oxygen_procentage(
  double sensor_calibration_voltage,
  double sensor_current_voltage
) {
  double result = (sensor_current_voltage / sensor_calibration_voltage) * AMBIENT_AIR_OXYGEN_FRACTION;
  if (result > 99.9) result = 99.9;
  if (get_sensor_voltage_mv() < MINIMAL_VALID_SENSOR_VOLTAGE_MV) result = -1;
  return result;
}

void setup() {
  display_setup();
  setup_adc();
  setup_sensor_calculation();

  on_start();
  on_show_battery_status();
  calibrate_sensor();
}

void loop() {
  loop_buttons();

  if (!is_calibrating && !is_calibration_error_occured) {
    read_sensor_voltage();
    double oxygen_procentage = analyse_sensor_voltage_to_oxygen_procentage(
                                 sensor_calibration_voltage,
                                 get_sensor_voltage()
                               );
    on_result(oxygen_procentage);
    delay(100);
  }
}

void on_calibration_start() {
  is_calibrating = true;
  is_calibration_error_occured = false;

  display_on_calibration_start();
}

void on_calibration_progress(
  int iteration,
  int max_number_of_iteration,
  double std_deviation
) {
  display_on_calibration_progress(iteration, max_number_of_iteration, std_deviation);
}

void on_calibration_end(
  int result,
  void* data
) {
  display_on_calibration_end(result, data);
  switch (result) {
    case CALIBRATION_RESULT_SUCCESS:
      setup_default_display();
      break;
    case CALIBRATION_RESULT_ERROR_SENSOR_CONNECTION_PROBLEM:
      is_calibration_error_occured = true;
      break;
    case CALIBRATION_RESULT_ERROR:
    default:
      is_calibration_error_occured = true;
  }

  is_calibrating = false;
}

void on_start() {
  display_on_start(VERSION);
}

void on_show_battery_status(){
  RA_BATTERY.clear();
  for (int i = 0; i <= RA_BATTERY_SIZE; i++) {
    read_battery_voltage();
  }
  float battery_voltage_volts = get_battery_voltage_mv() / 1000.0f;
  bool battery_voltage_ok = battery_voltage_volts > BATTERY_LOW_VOLTAGE_V;

  display_on_show_battery_status(battery_voltage_volts, battery_voltage_ok);
}

void setup_default_display() {
  display_on_setup_entry_display();
}

void on_result(double oxygen_procentage) {
  double oxygen_fraction = oxygen_procentage / 100.0;
  double mod_1_4 = calculate_mod(oxygen_fraction, 1.4);
  double mod_1_6 = calculate_mod(oxygen_fraction, 1.6);
  display_on_result(oxygen_procentage, mod_1_4, mod_1_6);
}

double calculate_mod(
  double oxygen_fraction,
  double max_ppo2
) {
  return max_ppo2 / oxygen_fraction * 10.0 - 10.0;
}
