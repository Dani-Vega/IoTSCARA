#include "Filter.h"

Filter::Filter() {
    for (int i = 0; i < MAX_NUM_SIZE; i++) {
        _x[i] = 0.0; 
    }
    for (int i = 0; i < MAX_DEN_SIZE; i++) {
        _y[i] = 0.0; 
    }
}

Filter::~Filter() {
}

void Filter::setup(float b[], float a[], unsigned char num_coeff_b, unsigned char num_coeff_a, float x_init, float y_init) {
    _b = b;  // Set numerator coefficients
    _a = a;  // Set denominator coefficients
    _num_coeff[0] = num_coeff_b;  // Number of numerator coefficients
    _num_coeff[1] = num_coeff_a;  // Number of denominator coefficients

    // Initialize filter state with provided values or zero
    for (int i = 0; i < _num_coeff[0]; i++) {
        _x[i] = x_init;  
    }
    for (int i = 0; i < _num_coeff[1]; i++) {
        _y[i] = y_init;  
    }
}

// Apply function - Apply the difference equation to the input data to get the filtered output
float Filter::apply(float input_data) {
    float output = 0.0;

    // Shift past input values (x[n], x[n-1], etc.)
    for (int i = _num_coeff[0] - 1; i > 0; i--) {
        _x[i] = _x[i - 1];  // Shift past inputs
    }
    _x[0] = input_data;  // Set current input as the newest x[0]

    // Shift past output values (y[n-1], y[n-2], etc.)
    for (int i = _num_coeff[1] - 1; i > 0; i--) {
        _y[i] = _y[i - 1];  // Shift past outputs
    }

    for (int i = 0; i < _num_coeff[0]; i++) {
        output += _b[i] * _x[i];
    }

    // Subtract the denominator terms (a coefficients * past outputs, excluding a[0] which is 1 by convention)
    for (int i = 1; i < _num_coeff[1]; i++) {
        output -= _a[i] * _y[i];
    }

    _y[0] = output;  
    return output;  
}