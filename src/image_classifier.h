//
// SPDX-FileCopyrightText: Copyright 2024 Arm Limited and/or its affiliates <open-source-office@arm.com>
// SPDX-License-Identifier: MIT
//

#ifndef _IMAGE_CLASSIFIER_H_
#define _IMAGE_CLASSIFIER_H_

namespace tflite {
    class ErrorReporter;
    class MicroInterpreter;
}
struct TfLiteTensor;

class ImageClassifier {
public:
    ImageClassifier(uint8_t* tensorArena, size_t tensorArenaSize);
    virtual ~ImageClassifier();

    int init();

    float* predict(const uint8_t* image, int width, int height);

private:
    uint8_t* _tensorArena;
    size_t _tensorArenaSize;

    tflite::ErrorReporter* _errorReporter;
    tflite::MicroInterpreter* _interpreter;
    TfLiteTensor* _input;
    TfLiteTensor* _output;
};

#endif
