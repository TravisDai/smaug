// Activation function implementations that consume packed half-precision
// packed values. These are optimized for the CPU, NOT for Aladdin.

#include <float.h>

#include "core/ref/activation_functions.h"
#include "core/ref/lookup_tables_ops.h"
#include "core/smiv/params.h"
#include "utility/compression.h"
#include "utility/utility.h"
#include "nnet_fwd.h"

// Internal to this file only.
#define _VECTOR_WIDTH (4)
#define _VECTOR_BYTES (16)

// The rectified linear activation function
void relu_simd128(float* inputs, size_t size) {
    v4fp_t* vec_inputs = (v4fp_t*)(ASSUME_ALIGNED(inputs, _VECTOR_BYTES));
    v4fp_t zero = (v4fp_t){ 0 };
    for (size_t i = 0; i < size / _VECTOR_WIDTH; i++) {
        v4fp_t a = vec_inputs[i];
        v4sfx_t mask = (a > zero);
        vec_inputs[i] = VEC128_MASK(a, mask);
    }
}

// The leaky rectified linear activation function
void lrelu_simd128(float* inputs, size_t size, float alpha) {
    v4fp_t* vec_inputs = (v4fp_t*)(ASSUME_ALIGNED(inputs, _VECTOR_BYTES));
    v4fp_t zero = (v4fp_t){ 0 };
    v4fp_t alpha_vec = (v4fp_t){ alpha, alpha, alpha, alpha };
    for (size_t i = 0; i < size / _VECTOR_WIDTH; i++) {
        v4fp_t a = vec_inputs[i];
        v4sfx_t neg_mask = a < zero;
        v4sfx_t pos_mask = a >= zero;
        v4fp_t scaled = alpha_vec * a;
        vec_inputs[i] =
                VEC128_MASK(scaled, neg_mask) + VEC128_MASK(a, pos_mask);
    }
}

// The hyperbolic tangent activation function
void tanh_act_simd128(float* inputs, int size, float* results) {
    int i;
    v4fp_t* vec_inputs =
            (v4fp_t*)(__builtin_assume_aligned(inputs, _VECTOR_BYTES));
    v4fp_t* vec_results =
            (v4fp_t*)(__builtin_assume_aligned(results, _VECTOR_BYTES));
    v4fp_t two = { 2, 2, 2, 2 };
    v4fp_t one = { 1, 1, 1, 1 };
    for (i = 0; i < size / _VECTOR_WIDTH; i++) {
        vec_results[i] = vec_inputs[i] * two;
    }
    sigmoid_inplace(results, size);

    for (i = 0; i < size / _VECTOR_WIDTH; i++) {
        vec_results[i] = vec_results[i] * two - one;
    }
}

// Dispatch to the appropriate activation function.
void activation_fun_simd128(packed_fp16* activations,
                            int batch_size,
                            int input_size,
                            activation_type function,
                            packed_fp16* results) {
    uarray_t packed_array;
    packed_array.d = activations;
    packed_array.size = input_size * batch_size;
    farray_t* unpacked_activations = unpack_data_fp16x4(&packed_array, NULL);

    if (function == RELU) {
        relu_simd128(unpacked_activations->d, unpacked_activations->size);
    } else if (function == LRELU) {
        static const float alpha = 0.1;
        lrelu_simd128(
                unpacked_activations->d, unpacked_activations->size, alpha);
    } else if (function == ELU) {
        static const float alpha = 0.1;
        elu(unpacked_activations->d,
            unpacked_activations->size,
            alpha,
            unpacked_activations->d);
    } else if (function == SELU) {
        selu(unpacked_activations->d,
             unpacked_activations->size);
    } else if (function == TANH) {
        tanh_act_simd128(unpacked_activations->d,
                         unpacked_activations->size,
                         unpacked_activations->d);
    } else if (function == SIGMOID) {
        sigmoid_inplace(unpacked_activations->d, unpacked_activations->size);
    } else if (function == SOFTMAX) {
        assert(false && "Softmax SIMD not supported!");
    }
    uarray_t* packed_results = pack_data_fp16(unpacked_activations, results);
    // This frees the malloc'ed pointer to the uarray_t without freeing the
    // buffer itself.
    free(packed_results);
}
