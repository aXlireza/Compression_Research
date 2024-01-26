#undef abs

#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "Adafruit_TFLite_Micro_Speech/src/tensorflow/lite/experimental/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "Adafruit_TFLite_Micro_Speech/src/tensorflow/lite/version.h"

#include "sine_model.h"
#include "tensorflow/lite/micro/testing/micro_test.h"


void setup() {
  tflite::MicroErrorReporter micro_error_reporter;
  tflite::ErrorReporter* error_reporter = &micro_error_reporter;
  Serial.begin(9600);

  const tflite::Model* model = ::tflite::GetModel(sine_model);
  Serial.println(model->version());
  Serial.println(TFLITE_SCHEMA_VERSION);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
      "Model provided is schema version %d not equal "
      "to supported version %d.\n",
      model->version(), TFLITE_SCHEMA_VERSION
    );
  }
}


void loop() {

}