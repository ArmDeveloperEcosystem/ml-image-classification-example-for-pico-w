//
// SPDX-FileCopyrightText: Copyright 2024 Arm Limited and/or its affiliates <open-source-office@arm.com>
// SPDX-License-Identifier: MIT
//

#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/version.h>

#include "tflite_model.h"

#include "image_classifier.h"

ImageClassifier::ImageClassifier(uint8_t* tensorArena, size_t tensorArenaSize) :
    _tensorArena(tensorArena),
    _tensorArenaSize(tensorArenaSize)
{
}

ImageClassifier::~ImageClassifier()
{
}

int ImageClassifier::init()
{
    static tflite::MicroErrorReporter micro_error_reporter;
    _errorReporter = &micro_error_reporter;

    const tflite::Model* model = tflite::GetModel(tflite_model);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        TF_LITE_REPORT_ERROR(_errorReporter,
            "Model provided is schema version %d not equal "
            "to supported version %d.",
            model->version(), TFLITE_SCHEMA_VERSION);
        return 0;
    }

    static tflite::MicroMutableOpResolver<9> micro_op_resolver(_errorReporter);
    if (micro_op_resolver.AddConv2D() != kTfLiteOk) {
        return 0;
    }
    if (micro_op_resolver.AddDepthwiseConv2D() != kTfLiteOk) {
        return 0;
    }
    if (micro_op_resolver.AddPad() != kTfLiteOk) {
        return 0;
    }
    if (micro_op_resolver.AddMean() != kTfLiteOk) {
        return 0;
    }
    if (micro_op_resolver.AddSoftmax() != kTfLiteOk) {
        return 0;
    } 
    if (micro_op_resolver.AddReshape() != kTfLiteOk) {
        return 0;
    }
    if (micro_op_resolver.AddDequantize() != kTfLiteOk) {
        return 0;
    }
    if (micro_op_resolver.AddQuantize() != kTfLiteOk) {
        return 0;
    }
    if (micro_op_resolver.AddConcatenation() != kTfLiteOk) {
        return 0;
    }

    // Build an interpreter to run the model with.
    static tflite::MicroInterpreter static_interpreter(
        model,
        micro_op_resolver,
        _tensorArena,
        _tensorArenaSize,
        _errorReporter
    );
    _interpreter = &static_interpreter;

    // Allocate memory from the tensor_arena for the model's tensors.
    TfLiteStatus allocate_status = _interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        TF_LITE_REPORT_ERROR(_errorReporter, "AllocateTensors() failed");
        return 0;
    }

    _input = _interpreter->input(0);
    _output = _interpreter->output(0);

    return 1;
}

float* ImageClassifier::predict(const uint8_t* image, int width, int height)
{
    int input_height = _input->dims->data[1];
    int input_width = _input->dims->data[2];

    int y_offset = (height - input_height) / 2;
    int x_offset = (width - input_width) / 2;

    int8_t* dst = _input->data.int8;
    const uint8_t* src = image;

    src += y_offset * width;

    for (int y = 0; y < input_height; y++) {
        src += x_offset;

        for (int x = 0; x < input_width; x++) {
            *dst++ = *src++ - 128;
        }

        src += x_offset;
    }

    if (kTfLiteOk != _interpreter->Invoke()) {
        TF_LITE_REPORT_ERROR(_errorReporter, "Invoke failed.");
    }

    return _output->data.f;
}
