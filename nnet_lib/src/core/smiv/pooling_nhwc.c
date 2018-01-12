#include <stdio.h>

#include "core/nnet_fwd_defs.h"
#include "core/smiv/params.h"
#include "core/smiv/impls.h"

// A max-pooling operation on SMIV.
//
// This requires a blocked channel data format (GNHWC), where G = channels/8,
// and the last dimension = chans = 8. The last dimension MUST be 8.
// This supports arbitrary pooling sizes and strides.
//
// Args:
//   inputs: A pointer to the input buffer.
//   curr_layer: A description of this pooling layer. Note that the
//     input/dimensions are still described logically as NCHW (e.g.
//     layer.input.rows = actual number of rows). The number of channels need
//     not be a multiple of 8; prior to calling this function the data shoud
//     have been converted into NHWC format, and that conversion will take care
//     of the required alignment.
//   results: A pointer to the output buffer.
void maxpooling_nhwc_smiv_fxp(float* inputs, layer_t curr_layer, float* results) {
    const int a_rows = curr_layer.inputs.rows;
    const int a_cols = curr_layer.inputs.cols;
    const int a_chan_groups = FRAC_CEIL(curr_layer.inputs.height, VECTOR_SIZE);
    const int result_rows = curr_layer.outputs.rows;
    const int result_cols = curr_layer.outputs.cols;

    const int pool_size = curr_layer.weights.cols;
    const int stride = curr_layer.field_stride;

    const int end_row = a_rows - pool_size + 1;
    const int end_col = a_cols - pool_size + 1;

    ARRAY_4D(float, _a, inputs, a_rows, a_cols, VECTOR_SIZE);
    ARRAY_4D(float, _results, results, result_rows, result_cols, VECTOR_SIZE);

    maxpool_chan_grp:
    for (int chan_grp = 0; chan_grp < a_chan_groups; chan_grp++) {
        int out_row = 0;
        maxpool_chan_input_row:
        for (int row = 0; row < end_row; row+=stride) {
            int out_col = 0;
            maxpool_chan_input_col:
            for (int col = 0; col < end_col; col+=stride) {
                float curr_results[VECTOR_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0};
                float next_pixels[VECTOR_SIZE];
                maxpool_pool_row:
                for (int pool_i = 0; pool_i < pool_size; pool_i++) {
                    maxpool_pool_col:
                    for (int pool_j = 0; pool_j < pool_size; pool_j++) {
                        maxpool_load_chan_px:
                        for (int px = 0; px < VECTOR_SIZE; px++) {
                            next_pixels[px] = _a[chan_grp][row+pool_i][col+pool_j][px];
                        }
                        maxpool_compare:
                        for (int px = 0; px < VECTOR_SIZE; px++) {
                           if (curr_results[px] < next_pixels[px])
                              curr_results[px] = next_pixels[px];
                        }
                    }
                }
                // Commit.
                maxpool_commit:
                for (int commit_i = 0; commit_i < VECTOR_SIZE; commit_i++) {
                    _results[chan_grp][out_row][out_col][commit_i] =
                        curr_results[commit_i];
                }
                out_col++;
            }
            out_row++;
        }
    }
}
