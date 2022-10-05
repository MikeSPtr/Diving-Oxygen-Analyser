#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Display
Adafruit_SSD1306 display(4);

void display_setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE, BLACK);
}

void display_on_calibration_start() {
  display.clearDisplay();

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("CALIBRATE");

  display.setTextSize(1);
  display.setCursor(0, 24);
  display.println("Wait... ");

  display.display();
}

void display_on_start(
  char* version
) {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Oxygen Analyser");

  display.setCursor(0, 12);
  display.print("Version: "); display.println(version);
  display.println("Created by Poteralski");

  display.display();

  delay(1000);
}

void display_on_calibration_progress(
  int iteration,
  int max_number_of_iteration,
  double std_deviation
) {
  display.setTextSize(1);
  display.setCursor(0, 24);
  display.print("Wait: ");
  display.print(iteration);
  display.print("/");
  display.print(max_number_of_iteration);
  display.print(" SD:");
  display.print(std_deviation);

  display.display();
}

void display_on_setup_entry_display() {
  display.clearDisplay();

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("O2:      %");

  display.setCursor(0, 17);
  display.setTextSize(1);
  display.println("MOD 1.4:      m");
  display.println("MOD 1.6:      m");

  display.display();
}

void display_on_calibration_end(
  int result,
  void* data
) {
  switch (result) {
    case CALIBRATION_RESULT_SUCCESS:
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 0);
      display.println("CALIBRATED");
      display.setCursor(0, 17);
      display.setTextSize(1);
      display.print("Ambient voltage:");
      display.setCursor(0, 25);
      display.print(*(double *) data); display.println("mV");
      display.display();
      delay(2000);
      break;
    case CALIBRATION_RESULT_ERROR_SENSOR_CONNECTION_PROBLEM:
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 0);
      display.println("ERROR!");
      display.setCursor(0, 17);
      display.setTextSize(1);
      display.print("Check sensor!");
      display.setCursor(0, 25);
      display.print("Sensor out: "); display.print(*(double *) data); display.println("mV");
      display.display();
      break;
    case CALIBRATION_RESULT_ERROR:
    default:
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 0);
      display.println("ERROR!");
      display.setTextSize(1);
      display.setCursor(0, 17);
      display.println("Could NOT calibrate.");
      display.setCursor(0, 25);
      display.println("'Cal' to retry ");
      display.display();
      break;
  }
}

void display_on_show_battery_status(
  float battery_voltage_volts,
  bool battery_voltage_ok
) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("BATTERY");

  display.setTextSize(1);
  display.setCursor(0, 24);
  display.print(battery_voltage_volts); display.print("V");

  if (battery_voltage_ok) {
    display.print(" is OK.");
  } else {
    display.print(" is LOW!");
  }
  display.display();

  if (battery_voltage_ok) {
    delay(1000);
  } else {
    delay(3000);
  }
}

void display_on_result(
  double oxygen_procentage,
  double mod_1_4,
  double mod_1_6
) {
  display.setTextSize(2);
  display.setCursor(42, 0);
  display.println(oxygen_procentage);

  display.setTextSize(1);
  display.setCursor(52, 17);
  display.println(mod_1_4);

  display.setCursor(52, 25);
  display.println(mod_1_6);

  display.display();
}
