// Header file model.h
#include <Arduino.h>

#ifndef _MODEL_H_
#define _MODEL_H_

// Fungsi Keanggotaan Gaussian
float gaussian_mf(float x, float mean, float sigma) {
    return exp(-((x - mean) * (x - mean)) / (2 * sigma * sigma));
}

// Fungsi Inferensi ANFIS
float anfis_inference(float inputs[], float mf_params[][2], float rule_weights[]) {
    float mf_values[4];
    // Menghitung nilai keanggotaan untuk setiap input
    for (int i = 0; i < 4; i++) {
        mf_values[i] = gaussian_mf(inputs[i], mf_params[i][0], mf_params[i][1]);
    }

    // Agregasi hasil keanggotaan berdasarkan aturan
    float weighted_sum = 0;
    float weight_total = 0;
    for (int i = 0; i < 4; i++) {
        weighted_sum += mf_values[i] * rule_weights[i];
        weight_total += rule_weights[i];
    }

    return weighted_sum / weight_total;
}

#endif
