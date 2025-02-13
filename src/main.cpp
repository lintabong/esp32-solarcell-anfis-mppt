#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_INA219.h>

#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "model.h"

#define MA_WINDOW_SIZE 5
#define LDR_PIN 34
#define ONE_WIRE_BUS 4
#define PWM_PIN 5
#define PWM_MAX 255

LiquidCrystal_I2C lcd(0x27, 16, 2);

Adafruit_INA219 ina219;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float voltage_buffer[MA_WINDOW_SIZE] = {0};
int buffer_index = 0;
bool buffer_filled = false;

namespace {
  tflite::ErrorReporter* error_reporter = nullptr;
  const tflite::Model* model = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* input = nullptr;
  TfLiteTensor* output = nullptr;

  constexpr int kTensorArenaSize = 4096;
  uint8_t tensor_arena[kTensorArenaSize];
}

float calculate(float new_value) {
  voltage_buffer[buffer_index] = new_value;
  buffer_index = (buffer_index + 1) % MA_WINDOW_SIZE;
  
  float sum = 0;
  int count = buffer_filled ? MA_WINDOW_SIZE : buffer_index;
  for (int i = 0; i < count; i++) {
    sum += voltage_buffer[i];
  }
  return sum / count;
}

void setup() {
  Serial.begin(115200);
  
  lcd.begin();
  lcd.backlight();
  lcd.print("Initializing...");

  pinMode(PWM_PIN, OUTPUT);

  if (!ina219.begin()) {
    Serial.println("Gagal menginisialisasi INA219!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("INA219 Error");
    while (1);
  }

  sensors.begin();

  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  model = tflite::GetModel(model_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model versi %d tidak cocok dengan versi %d.",
                         model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "Gagal AllocateTensors()");
    return;
  }

  input = interpreter->input(0);
  output = interpreter->output(0);

  Serial.println("Setup selesai. TensorFlow Lite siap digunakan.");
  lcd.clear();
}

void loop() {
  float voltage = ina219.getBusVoltage_V();  // Tegangan dari INA219
  float current = ina219.getCurrent_mA();    // Arus dari INA219
  sensors.requestTemperatures();            // Minta data suhu dari DS18B20
  float temperature = sensors.getTempCByIndex(0);  // Suhu dalam derajat Celsius
  int intensity = analogRead(LDR_PIN);      // Nilai intensitas cahaya dari LDR

  float voltageFinal = calculate(voltage);
  if (buffer_index == 0) buffer_filled = true; 

  // Tampilkan nilai sensor untuk debugging
  // Serial.println("");
  // Serial.println("Nilai Sensor:");
  // Serial.print("Tegangan          : "); Serial.print(voltage); Serial.println("(V)");
  // Serial.print("Tegangan MA       : "); Serial.print(voltageFinal); Serial.println("(V)");
  // Serial.print("Arus              : "); Serial.print(current); Serial.println("(mA)");
  // Serial.print("Suhu              : "); Serial.print(temperature); Serial.println("(°C)");
  // Serial.print("Intensitas Cahaya : "); Serial.println(intensity);

  Serial.print(voltage); Serial.print("\t"); Serial.println(voltageFinal);

  // Tampilkan ke LCD
  lcd.setCursor(2, 0);
  lcd.print(voltage);
  lcd.setCursor(2, 1);
  lcd.print(current);
  lcd.setCursor(10, 0);
  lcd.print(intensity);
  lcd.setCursor(10, 1);
  lcd.print(temperature);
  lcd.setCursor(0, 0);
  lcd.print("V:");
  lcd.setCursor(0, 1);
  lcd.print("I:");
  lcd.setCursor(8, 0);
  lcd.print("L:");
  lcd.setCursor(8, 1);
  lcd.print("T:");

  // Masukkan nilai sensor ke dalam tensor input
  input->data.f[0] = current;        // Arus
  input->data.f[1] = temperature;   // Suhu
  input->data.f[2] = voltage;       // Tegangan
  input->data.f[3] = intensity;     // Intensitas

  // Jalankan inferensi
  if (interpreter->Invoke() != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "Gagal menjalankan model");
    return;
  }

  float y_pred = output->data.f[0];
  int pwm_value = constrain(static_cast<int>(y_pred), 0, PWM_MAX);

  analogWrite(PWM_PIN, pwm_value);

  delay(1000);
}
