#ifndef MODEL_H
#define MODEL_H

// Membership function parameters (mean, sigma)
float mf_params[4][2] = {
    {0.6, 0.1}, // current
    {100, 10}, // rawIntensity
    {30, 5}, // temperature
    {1.5, 0.2}, // voltage
};

// Rule weights
float rule_weights[4] = {1.0, 1.0, 1.0, 1.0};

#endif // MODEL_H
