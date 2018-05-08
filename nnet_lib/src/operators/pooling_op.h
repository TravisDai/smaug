#ifndef _OPERATORS_POOLING_OP_H_
#define _OPERATORS_POOLING_OP_H_

#include "core/operator.h"
#include "core/tensor.h"
#include "core/workspace.h"

namespace smaug {

template <typename Backend>
class PoolingOp : public Operator {
   public:
    PoolingOp(const std::string& name, OpType _opType, Workspace* workspace)
            : Operator(name, _opType, workspace), poolingRowSize(0),
              poolingColSize(0), poolingRowStride(0), poolingColStride(0) {
        inputs.resize(kNumInputs, nullptr);
        outputs.resize(kNumOutputs, nullptr);
    }

    void setPoolingSize(int size) {
        poolingRowSize = size;
        poolingColSize = size;
    }
    void setPoolingStride(int rowStride, int colStride) {
        poolingRowStride = rowStride;
        poolingColStride = colStride;
    }

    virtual void run() = 0;
    virtual bool validate() { assert(false && "Unimplemented!"); }

    virtual DataLayoutSet getInputDataLayouts() const {
        return DataLayoutSet(DataLayout::NCHW);
    }
    virtual DataLayoutSet getOutputDataLayouts() const {
        return DataLayoutSet(DataLayout::NCHW);
    }

    int getNumOfmaps() const {
        Tensor<Backend>* input = workspace->getTensor<Backend>(inputs[0]);
        assert(input && "Unable to find input for pooling layer!");
        bool isNCHW = input->getDataLayout() == DataLayout::NCHW;
        int chanIdx = isNCHW ? 1 : 3;
        return input->dim(chanIdx);
    }

    TensorShape inferOutputShape() const {
        const TensorShape& inputShape = inputs.at(Inputs)->getShape();
        bool isNCHW = inputShape.getLayout() == DataLayout::NCHW;
        int inputRows = isNCHW ? inputShape[2] : inputShape[1];
        int inputCols = isNCHW ? inputShape[3] : inputShape[2];
        int inputChans = isNCHW ? inputShape[1] : inputShape[0];
        int outputRows = calcOutputRows(inputRows);
        int outputCols = calcOutputCols(inputCols);
        if (isNCHW) {
            return TensorShape(
                    { inputShape[0], inputChans, outputRows, outputCols },
                    inputShape.getLayout());
        } else {
            return TensorShape(
                    { inputShape[0], outputRows, outputCols, inputChans },
                    inputShape.getLayout());
        }
    }

    void createOutputTensors() {
        if (outputs.at(Outputs))
            return;
        TensorShape shape = inferOutputShape();
        Tensor<Backend>* output = new Tensor<Backend>(name, shape);
        workspace->addTensor(output);
        outputs.at(Outputs) = output;
    }

    virtual void createAllTensors() {
        createOutputTensors();
    }

   protected:
    int calcOutputRows(int inputRows) const {
        return computeOutputDim(inputRows, poolingRowSize, poolingRowStride);
    }
    int calcOutputCols(int inputCols) const {
        return computeOutputDim(inputCols, poolingColSize, poolingColStride);
    }

    int computeOutputDim(int inputDims, int poolSize, int poolStride) const {
        return (inputDims - poolSize) / poolStride + 1;
    }

    enum { Inputs, kNumInputs };
    enum { Outputs, kNumOutputs };

    int poolingRowSize;
    int poolingColSize;
    int poolingRowStride;
    int poolingColStride;
};

template <typename Backend>
class MaxPoolingOp : public PoolingOp<Backend> {
   protected:
    typedef PoolingOp<Backend> Parent;

   public:
    MaxPoolingOp(const std::string& name, Workspace* workspace)
            : PoolingOp<Backend>(name, OpType::MaxPooling, workspace) {}
    virtual void run(){};
    virtual void printSummary(std::ostream& out) const {
        const TensorShape& outputShape =
                this->outputs.at(Parent::Outputs)->getShape();
        out << this->name << " (MaxPooling)\t\t" << outputShape << "\n";
    }
};

template <typename Backend>
class AvgPoolingOp : public PoolingOp<Backend> {
   protected:
    typedef PoolingOp<Backend> Parent;

   public:
    AvgPoolingOp(const std::string& name, Workspace* workspace)
            : PoolingOp<Backend>(name, OpType::AveragePooling, workspace) {}
    virtual void run(){};
    virtual void printSummary(std::ostream& out) const {
        const TensorShape& outputShape =
                this->outputs.at(Parent::Outputs)->getShape();
        out << this->name << " (AvgPooling)\t\t" << outputShape << "\n";
    }
};

}  // namespace smaug

#endif
