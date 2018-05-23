#include <cmath>

#include "core/backend.h"
#include "operators/common.h"
#include "operators/softmax_op.h"
#include "operators/ref/ref_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Softmax function.
//
// The softmax function exponentiates each element and then normalizes each row
// to sum to 1.
//
// Args:
//   inputs: Matrix of size input_num x input_size, stored rowmajor. This
//      contains both inputs and the outputs.
//   input_num: batch size.
//   input_size: number of activations per input.
//   input_pad: alignment padding.
//   results: Output array.
//
// To improve numerical stability, we use the max trick: all elements are first
// subtracted by the maximum value in each input before being exponentiated.
void ref_softmax_f32_nc(float* inputs,
                        float* results,
                        int input_num,
                        int input_size,
                        int input_pad) {
    ARRAY_2D(float, _inputs, inputs, input_size + input_pad);
    ARRAY_2D(float, _results, results, input_size + input_pad);

    // Compute the maximum of the elements in groups of 8 and the remainder one
    // by one.
    int max8_remainder = input_size - ((input_size >> 3) << 3);

    softmax_batch:
    for (int i = 0; i < input_num; i++) {
        // Find the maximum of each input.
        float max_elem = -FLT_MAX;
        softmax_max_loop0:
        for (int j = 0; j < input_size - max8_remainder; j += 8) {
            max_elem = max9(max_elem,
                            _inputs[i][j],
                            _inputs[i][j + 1],
                            _inputs[i][j + 2],
                            _inputs[i][j + 3],
                            _inputs[i][j + 4],
                            _inputs[i][j + 5],
                            _inputs[i][j + 6],
                            _inputs[i][j + 7]);
        }
        // Do the remainder.
        softmax_max_loop1:
        for (int j = input_size - max8_remainder - 1; j < input_size; j++) {
            max_elem = max2(max_elem, _inputs[i][j]);
        }

        // Subtract the max from each activation.
        softmax_max_sub:
        for (int j = 0; j < input_size; j++) {
            _results[i][j] = _inputs[i][j] - max_elem;
        }

        // Now exponentiate.
        softmax_exp:
        for (int j =0; j < input_size; j++) {
            _results[i][j] = exp(_results[i][j]);
        }

        // Compute the normalization factor, separately from the exponentiation,
        // making it easier for Aladdin to turn this into an adder tree.
        float normaliz = 0.0;
        softmax_inner0:
        for (int j = 0; j < input_size; j++) {
            normaliz += _results[i][j];
        }
        // Precompute the division so that later we can just do a multiplication.
        normaliz = 1.0 / (normaliz + 1e-6);  // epsilon for numerical stability.

        softmax_inner1:
        for (int j = 0; j < input_size; j++) {
            _results[i][j] *= normaliz;
        }
    }
}

#ifdef __cplusplus
}
#endif

namespace smaug {

template <>
void SoftmaxOp<ReferenceBackend>::run() {
    auto inputs = getInput<ReferenceBackend>(Inputs);
    auto outputs = getOutput<ReferenceBackend>(Outputs);
    const TensorShape& inputShape = inputs->getShape();
    assert(inputShape == outputs->getShape());
    ref_softmax_f32_nc(inputs->data<float>(), outputs->data<float>(),
                       inputShape[0], inputShape[1], 0);
}

}  // namespace smaug
