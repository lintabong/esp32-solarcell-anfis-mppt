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

#define LDR_PIN 34
#define ONE_WIRE_BUS 4
#define PWM_PIN 5
#define PWM_MAX 255

Adafruit_INA219 ina219;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

namespace {
  tflite::ErrorReporter* error_reporter = nullptr;
  const tflite::Model* model = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* input = nullptr;
  TfLiteTensor* output = nullptr;

  constexpr int kTensorArenaSize = 4096;
  uint8_t tensor_arena[kTensorArenaSize];
}

void setup() {
  Serial.begin(115200);
  pinMode(PWM_PIN, OUTPUT);

  if (!ina219.begin()) {
    Serial.println("Gagal menginisialisasi INA219!");
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
}

void loop() {
  float voltage = ina219.getBusVoltage_V();  // Tegangan dari INA219
  float current = ina219.getCurrent_mA();    // Arus dari INA219
  sensors.requestTemperatures();            // Minta data suhu dari DS18B20
  float temperature = sensors.getTempCByIndex(0);  // Suhu dalam derajat Celsius
  int intensity = analogRead(LDR_PIN);      // Nilai intensitas cahaya dari LDR

  // Tampilkan nilai sensor untuk debugging
  Serial.println("Nilai Sensor:");
  Serial.print("Tegangan (V): ");
  Serial.println(voltage);
  Serial.print("Arus (mA): ");
  Serial.println(current);
  Serial.print("Suhu (°C): ");
  Serial.println(temperature);
  Serial.print("Intensitas Cahaya: ");
  Serial.println(intensity);

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

  // Ambil output dan konversi ke integer
  float y_pred = output->data.f[0];
  int pwm_value = constrain(static_cast<int>(y_pred), 0, PWM_MAX);

  Serial.print("Prediksi (float): ");
  Serial.println(y_pred);
  Serial.print("PWM (integer): ");
  Serial.println(pwm_value);

  analogWrite(PWM_PIN, pwm_value);

  delay(1000);
}
