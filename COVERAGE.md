# OpenVX 1.3.2 Sample Implementation -- API & CTS Coverage Report

This document provides a comprehensive audit of the OpenVX 1.3.2 sample implementation, covering API implementation status and Conformance Test Suite (CTS) test coverage for every declared function.

## Contents

* [Summary](#summary)
* [Core API Coverage](#core-api-coverage)
  * [Context Functions](#context-functions)
  * [Image Functions](#image-functions)
  * [Kernel Functions](#kernel-functions)
  * [Graph Functions](#graph-functions)
  * [Node Functions](#node-functions)
  * [Parameter Functions](#parameter-functions)
  * [Scalar Functions](#scalar-functions)
  * [Reference Functions](#reference-functions)
  * [Delay Functions](#delay-functions)
  * [Logging Functions](#logging-functions)
  * [LUT Functions](#lut-functions)
  * [Distribution Functions](#distribution-functions)
  * [Threshold Functions](#threshold-functions)
  * [Matrix Functions](#matrix-functions)
  * [Convolution Functions](#convolution-functions)
  * [Pyramid Functions](#pyramid-functions)
  * [Remap Functions](#remap-functions)
  * [Array Functions](#array-functions)
  * [Object Array Functions](#object-array-functions)
  * [Tensor Functions](#tensor-functions)
  * [Meta Format Functions](#meta-format-functions)
* [Node Creation Functions](#node-creation-functions)
* [Immediate Mode Functions](#immediate-mode-functions)
* [Vision Kernel Implementations](#vision-kernel-implementations)
* [Extension API Coverage](#extension-api-coverage)
  * [Import/Export (IX)](#importexport-ix)
  * [Neural Networks (NN)](#neural-networks-nn)
  * [User Data Object](#user-data-object)
  * [NNEF Import Kernel](#nnef-import-kernel)
  * [Pipelining](#pipelining)
  * [Streaming](#streaming)
  * [Event Queue](#event-queue)
  * [Compatibility/Deprecated](#compatibilitydeprecated)
  * [Unimplemented Extensions](#unimplemented-extensions)
* [Build Options](#build-options)

## Summary

| Category | Total Functions | Implemented | CTS Covered | Coverage |
|----------|----------------|-------------|-------------|----------|
| Core API (`vx_api.h`) | 155 | 155 | 155 | 100% |
| Node Creation (`vx_nodes.h`) | 61 | 61 | 61 | 100% |
| Immediate Mode (`vxu.h`) | 59 | 59 | 59 | 100% |
| **Core Total** | **275** | **275** | **275** | **100%** |
| Import/Export (IX) | 5 | 5 | 5 | 100% |
| Neural Networks (NN) | 8 | 8 | 8 | 100% |
| User Data Object | 8 | 6 | 6 | 75% |
| NNEF Import Kernel | 1 | 1 | 1 | 100% |
| Pipelining | 7 | 4 (stub) | 4 | 57% |
| Streaming | 3 | 3 (stub) | 3 | 100% |
| Event Queue | 10 | 5 (stub) | 5 | 50% |

> **Note:** 7 node/vxu functions (`vxAbsDiffNode`, `vxXorNode`, `vxuAbsDiff`, `vxuAnd`, `vxuOr`, `vxuXor`, `vxAndNode`/`vxOrNode`) are tested through C preprocessor token-pasting macros (e.g., `FUNC_ARG(AbsDiff)` expands to `vxAbsDiffNode`), not direct string references.

---

## Core API Coverage

All 155 functions declared in `vx_api.h` are fully implemented and have CTS test coverage.

### Context Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxCreateContext` | Yes | test_export_import_extension.c, test_logging.c, test_target.c |
| `vxReleaseContext` | Yes | test_export_import_extension.c, test_target.c |
| `vxGetContext` | Yes | test_graph_pipeline.c, test_graph.c, test_smoke.c |
| `vxQueryContext` | Yes | test_bilateralfilter.c, test_convolve.c, test_graph.c, test_smoke.c, test_target.c, test_tensor_nn.c |
| `vxSetContextAttribute` | Yes | test_bilateralfilter.c, test_box3x3.c, test_canny.c, test_convolve.c, test_dilate3x3.c, test_erode3x3.c, test_gaussian3x3.c, test_target.c |
| `vxDirective` | Yes | test_graph.c, test_logging.c |
| `vxGetStatus` | Yes | test_export_import_extension.c, test_graph.c, test_smoke.c, test_vximage.c |
| `vxRegisterUserStruct` | Yes | test_array.c, test_graph.c, test_smoke.c |
| `vxRegisterUserStructWithName` | Yes | test_array.c, test_graph.c, test_smoke.c |
| `vxGetUserStructEnumByName` | Yes | test_graph.c |
| `vxGetUserStructNameByEnum` | Yes | test_graph.c |
| `vxAllocateUserKernelId` | Yes | test_graph_pipeline.c, test_graph.c |
| `vxAllocateUserKernelLibraryId` | Yes | test_graph.c |
| `vxSetImmediateModeTarget` | Yes | test_target.c |
| `vxHint` | Yes | test_smoke.c |
| `vxAddLogEntry` | Yes | test_logging.c |
| `vxRegisterLogCallback` | Yes | test_logging.c, test_tensor_networks.c |

### Image Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxCreateImage` | Yes | test_addsub.c, test_box3x3.c, test_canny.c, test_graph.c, test_smoke.c, test_vximage.c (56+ files) |
| `vxCreateImageFromROI` | Yes | test_graph_roi.c, test_graph.c, test_vximage.c |
| `vxCreateUniformImage` | Yes | test_addsub.c, test_graph_pipeline.c, test_graph.c, test_vximage.c |
| `vxCreateVirtualImage` | Yes | test_addsub.c, test_graph_pipeline.c, test_graph.c, test_vximage.c |
| `vxCreateImageFromHandle` | Yes | test_vximage.c |
| `vxCreateImageFromChannel` | Yes | test_vximage.c |
| `vxCreateImageObjectArrayFromTensor` | Yes | test_vxtensor.c |
| `vxSwapImageHandle` | Yes | test_vximage.c |
| `vxQueryImage` | Yes | test_addsub.c, test_graph_delay.c, test_object_array.c, test_vximage.c |
| `vxSetImageAttribute` | Yes | test_convertcolor.c |
| `vxSetImagePixelValues` | Yes | test_vximage.c |
| `vxReleaseImage` | Yes | 56+ test files |
| `vxFormatImagePatchAddress1d` | Yes | test_copy.c, test_vximage.c |
| `vxFormatImagePatchAddress2d` | Yes | test_addsub.c, test_graph.c, test_vximage.c |
| `vxGetValidRegionImage` | Yes | test_copy.c, test_dilate3x3.c, test_erode3x3.c, test_vximage.c |
| `vxCopyImagePatch` | Yes | test_vximage.c |
| `vxMapImagePatch` | Yes | test_addsub.c, test_copy.c, test_graph_pipeline.c, test_graph.c, test_vximage.c |
| `vxUnmapImagePatch` | Yes | test_addsub.c, test_copy.c, test_graph_pipeline.c, test_graph.c, test_vximage.c |
| `vxSetImageValidRectangle` | Yes | test_dilate3x3.c, test_erode3x3.c, test_meanstddev.c, test_threshold.c, test_warpaffine.c |

### Kernel Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxLoadKernels` | Yes | test_smoke.c |
| `vxUnloadKernels` | Yes | test_smoke.c |
| `vxGetKernelByName` | Yes | test_graph_pipeline.c, test_graph.c, test_smoke.c, test_usernode.c |
| `vxGetKernelByEnum` | Yes | test_graph.c, test_smoke.c, test_usernode.c |
| `vxGetKernelParameterByIndex` | Yes | test_graph.c, test_nnef_import.c, test_smoke.c, test_usernode.c |
| `vxQueryKernel` | Yes | test_graph.c, test_nnef_import.c, test_smoke.c |
| `vxReleaseKernel` | Yes | test_graph_pipeline.c, test_graph.c, test_nnef_import.c, test_smoke.c, test_usernode.c |
| `vxAddUserKernel` | Yes | test_graph_pipeline.c, test_graph.c, test_smoke.c, test_usernode.c |
| `vxFinalizeKernel` | Yes | test_graph_pipeline.c, test_graph.c, test_smoke.c, test_usernode.c |
| `vxAddParameterToKernel` | Yes | test_graph_pipeline.c, test_graph.c, test_smoke.c, test_usernode.c |
| `vxRemoveKernel` | Yes | test_graph_pipeline.c, test_graph.c, test_smoke.c, test_usernode.c |
| `vxSetKernelAttribute` | Yes | test_graph.c, test_usernode.c |
| `vxRegisterKernelLibrary` | Yes | test_smoke.c |

### Graph Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxCreateGraph` | Yes | 69 test files |
| `vxReleaseGraph` | Yes | 69 test files |
| `vxVerifyGraph` | Yes | 62 test files |
| `vxProcessGraph` | Yes | 64 test files |
| `vxScheduleGraph` | Yes | test_addsub.c, test_graph_pipeline.c, test_graph.c |
| `vxWaitGraph` | Yes | test_addsub.c, test_graph_pipeline.c, test_graph.c |
| `vxIsGraphVerified` | Yes | test_graph.c, test_threshold.c |
| `vxQueryGraph` | Yes | test_graph.c |
| `vxSetGraphAttribute` | Yes | test_graph.c |
| `vxAddParameterToGraph` | Yes | test_graph_pipeline.c, test_graph.c |
| `vxSetGraphParameterByIndex` | Yes | test_graph_pipeline.c, test_graph.c |
| `vxGetGraphParameterByIndex` | Yes | test_graph.c |

### Node Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxCreateGenericNode` | Yes | test_graph_pipeline.c, test_graph.c, test_nnef_import.c, test_smoke.c, test_usernode.c |
| `vxQueryNode` | Yes | test_graph.c, test_smoke.c, test_usernode.c |
| `vxSetNodeAttribute` | Yes | test_bilateralfilter.c, test_box3x3.c, test_canny.c, test_graph.c, test_usernode.c |
| `vxReleaseNode` | Yes | 60 test files |
| `vxRemoveNode` | Yes | test_graph.c |
| `vxAssignNodeCallback` | Yes | test_graph_callbacks.c, test_graph_roi.c, test_graph.c |
| `vxRetrieveNodeCallback` | Yes | test_graph_callbacks.c |
| `vxSetNodeTarget` | Yes | test_target.c |
| `vxReplicateNode` | Yes | test_graph_pipeline.c, test_graph.c |

### Parameter Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxGetParameterByIndex` | Yes | test_graph_delay.c, test_graph_pipeline.c, test_graph.c, test_smoke.c |
| `vxReleaseParameter` | Yes | test_graph_delay.c, test_graph_pipeline.c, test_graph.c, test_smoke.c, test_usernode.c |
| `vxSetParameterByIndex` | Yes | test_graph_pipeline.c, test_graph.c, test_nnef_import.c, test_smoke.c, test_usernode.c |
| `vxSetParameterByReference` | Yes | test_graph.c, test_smoke.c |
| `vxQueryParameter` | Yes | test_graph_delay.c, test_graph.c, test_nnef_import.c, test_smoke.c, test_usernode.c |

### Scalar Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxCreateScalar` | Yes | test_controlflow.c, test_copy.c, test_graph_pipeline.c, test_graph.c, test_scalar.c, test_smoke.c |
| `vxCreateScalarWithSize` | Yes | test_scalar.c |
| `vxCreateVirtualScalar` | Yes | test_scalar.c |
| `vxReleaseScalar` | Yes | test_controlflow.c, test_graph_pipeline.c, test_graph.c, test_scalar.c, test_smoke.c |
| `vxCopyScalar` | Yes | test_controlflow.c, test_copy.c, test_graph_pipeline.c, test_graph.c, test_scalar.c |
| `vxCopyScalarWithSize` | Yes | test_scalar.c |
| `vxQueryScalar` | Yes | test_graph_delay.c, test_graph_pipeline.c, test_object_array.c, test_scalar.c, test_usernode.c |

### Reference Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxQueryReference` | Yes | test_copy.c, test_graph_delay.c, test_graph.c, test_smoke.c, test_user_data_object.c |
| `vxReleaseReference` | Yes | test_controlflow.c, test_copy.c, test_graph_delay.c, test_graph.c, test_smoke.c |
| `vxRetainReference` | Yes | test_graph.c, test_smoke.c |
| `vxSetReferenceName` | Yes | test_export_import_extension.c, test_smoke.c |

### Delay Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxCreateDelay` | Yes | test_export_import_extension.c, test_graph_delay.c, test_graph_pipeline.c, test_smoke.c |
| `vxReleaseDelay` | Yes | test_export_import_extension.c, test_graph_delay.c, test_graph_pipeline.c, test_smoke.c |
| `vxQueryDelay` | Yes | test_graph_delay.c |
| `vxGetReferenceFromDelay` | Yes | test_graph_delay.c, test_graph_pipeline.c |
| `vxAgeDelay` | Yes | test_graph_delay.c |
| `vxRegisterAutoAging` | Yes | test_graph_delay.c, test_graph_pipeline.c |

### Logging Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxAddLogEntry` | Yes | test_logging.c |
| `vxRegisterLogCallback` | Yes | test_logging.c, test_tensor_networks.c |

### LUT Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxCreateLUT` | Yes | test_controlflow.c, test_copy.c, test_graph.c, test_lut.c, test_smoke.c, test_tensor_op.c |
| `vxCreateVirtualLUT` | Yes | test_lut.c |
| `vxReleaseLUT` | Yes | test_export_import_extension.c, test_graph.c, test_lut.c, test_smoke.c |
| `vxQueryLUT` | Yes | test_graph_delay.c, test_lut.c, test_object_array.c, test_usernode.c |
| `vxCopyLUT` | Yes | test_copy.c, test_lut.c, test_tensor_op.c, test_usernode.c |
| `vxMapLUT` | Yes | test_copy.c, test_graph.c, test_lut.c |
| `vxUnmapLUT` | Yes | test_copy.c, test_graph.c, test_lut.c |

### Distribution Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxCreateDistribution` | Yes | test_controlflow.c, test_copy.c, test_histogram.c, test_smoke.c |
| `vxCreateVirtualDistribution` | Yes | test_distribution.c |
| `vxReleaseDistribution` | Yes | test_distribution.c, test_histogram.c, test_smoke.c |
| `vxQueryDistribution` | Yes | test_distribution.c, test_histogram.c, test_object_array.c, test_usernode.c |
| `vxCopyDistribution` | Yes | test_histogram.c, test_usernode.c |
| `vxMapDistribution` | Yes | test_histogram.c |
| `vxUnmapDistribution` | Yes | test_histogram.c |

### Threshold Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxCreateThresholdForImage` | Yes | test_canny.c, test_controlflow.c, test_copy.c, test_graph.c, test_threshold.c |
| `vxCreateVirtualThresholdForImage` | Yes | test_threshold.c |
| `vxReleaseThreshold` | Yes | test_canny.c, test_graph.c, test_threshold.c |
| `vxQueryThreshold` | Yes | test_canny.c, test_graph_delay.c, test_threshold.c, test_usernode.c |
| `vxSetThresholdAttribute` | Yes | test_threshold.c |
| `vxCopyThresholdValue` | Yes | test_copy.c, test_threshold.c, test_usernode.c |
| `vxCopyThresholdRange` | Yes | test_canny.c, test_graph.c, test_threshold.c |
| `vxCopyThresholdOutput` | Yes | test_threshold.c |

### Matrix Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxCreateMatrix` | Yes | test_controlflow.c, test_copy.c, test_matrix.c, test_nonlinearfilter.c, test_warpaffine.c |
| `vxCreateMatrixFromPattern` | Yes | test_matrix.c, test_nonlinearfilter.c |
| `vxCreateMatrixFromPatternAndOrigin` | Yes | test_matrix.c, test_nonlinearfilter.c |
| `vxCreateVirtualMatrix` | Yes | test_matrix.c |
| `vxReleaseMatrix` | Yes | test_matrix.c, test_nonlinearfilter.c, test_warpaffine.c, test_warpperspective.c |
| `vxQueryMatrix` | Yes | test_graph_delay.c, test_matrix.c, test_object_array.c, test_usernode.c |
| `vxCopyMatrix` | Yes | test_copy.c, test_matrix.c, test_usernode.c, test_warpaffine.c, test_warpperspective.c |

### Convolution Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxCreateConvolution` | Yes | test_controlflow.c, test_convolution.c, test_convolve.c, test_copy.c, test_laplacianpyramid.c |
| `vxCreateVirtualConvolution` | Yes | test_convolution.c |
| `vxReleaseConvolution` | Yes | test_convolution.c, test_convolve.c, test_laplacianpyramid.c |
| `vxQueryConvolution` | Yes | test_convolution.c, test_convolve.c, test_laplacianpyramid.c |
| `vxSetConvolutionAttribute` | Yes | test_convolution.c, test_convolve.c, test_laplacianpyramid.c |
| `vxCopyConvolutionCoefficients` | Yes | test_convolution.c, test_convolve.c, test_copy.c, test_laplacianpyramid.c |

### Pyramid Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxCreatePyramid` | Yes | test_controlflow.c, test_gaussianpyramid.c, test_graph.c, test_laplacianpyramid.c, test_optflowpyrlk.c |
| `vxCreateVirtualPyramid` | Yes | test_graph.c, test_optflowpyrlk.c |
| `vxReleasePyramid` | Yes | test_gaussianpyramid.c, test_graph.c, test_laplacianpyramid.c, test_optflowpyrlk.c |
| `vxQueryPyramid` | Yes | test_gaussianpyramid.c, test_graph.c, test_laplacianpyramid.c, test_object_array.c |
| `vxGetPyramidLevel` | Yes | test_copy.c, test_gaussianpyramid.c, test_graph.c, test_laplacianpyramid.c |

### Remap Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxCreateRemap` | Yes | test_controlflow.c, test_copy.c, test_remap.c, test_smoke.c |
| `vxCreateVirtualRemap` | Yes | test_remap.c |
| `vxReleaseRemap` | Yes | test_remap.c, test_smoke.c |
| `vxQueryRemap` | Yes | test_graph_delay.c, test_object_array.c, test_remap.c, test_usernode.c |
| `vxCopyRemapPatch` | Yes | test_controlflow.c, test_copy.c, test_remap.c, test_usernode.c |
| `vxMapRemapPatch` | Yes | test_copy.c, test_remap.c |
| `vxUnmapRemapPatch` | Yes | test_copy.c, test_remap.c |

### Array Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxCreateArray` | Yes | test_array.c, test_controlflow.c, test_fast.c, test_graph.c, test_harriscorners.c, test_smoke.c |
| `vxCreateVirtualArray` | Yes | test_graph.c |
| `vxReleaseArray` | Yes | test_array.c, test_fast.c, test_graph.c, test_harriscorners.c, test_smoke.c |
| `vxQueryArray` | Yes | test_array.c, test_graph.c, test_harriscorners.c, test_object_array.c, test_smoke.c |
| `vxAddArrayItems` | Yes | test_array.c, test_copy.c, test_graph.c, test_optflowpyrlk.c |
| `vxTruncateArray` | Yes | test_graph.c |
| `vxCopyArrayRange` | Yes | test_array.c |
| `vxMapArrayRange` | Yes | test_array.c, test_copy.c, test_graph.c, test_harriscorners.c |
| `vxUnmapArrayRange` | Yes | test_array.c, test_copy.c, test_graph.c, test_harriscorners.c |

### Object Array Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxCreateObjectArray` | Yes | test_controlflow.c, test_copy.c, test_graph_pipeline.c, test_graph.c, test_object_array.c |
| `vxCreateVirtualObjectArray` | Yes | test_object_array.c |
| `vxReleaseObjectArray` | Yes | test_graph_delay.c, test_graph_pipeline.c, test_graph.c, test_object_array.c |
| `vxGetObjectArrayItem` | Yes | test_copy.c, test_graph_delay.c, test_graph_pipeline.c, test_graph.c, test_object_array.c |
| `vxQueryObjectArray` | Yes | test_graph_delay.c, test_graph.c, test_object_array.c, test_usernode.c |

### Tensor Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxCreateTensor` | Yes | test_bilateralfilter.c, test_copy.c, test_graph.c, test_hog.c, test_tensor_nn.c, test_tensor_op.c, test_vxtensor.c |
| `vxCreateTensorFromHandle` | Yes | test_vxtensor.c |
| `vxCreateTensorFromView` | Yes | test_vxtensor.c |
| `vxCreateVirtualTensor` | Yes | test_vxtensor.c |
| `vxSwapTensorHandle` | Yes | test_vxtensor.c |
| `vxReleaseTensor` | Yes | test_bilateralfilter.c, test_graph.c, test_hog.c, test_tensor_nn.c, test_tensor_op.c, test_vxtensor.c |
| `vxQueryTensor` | Yes | test_graph_delay.c, test_graph.c, test_vxtensor.c |
| `vxCopyTensorPatch` | Yes | test_bilateralfilter.c, test_copy.c, test_hog.c, test_tensor_nn.c, test_tensor_op.c, test_vxtensor.c |
| `vxMapTensorPatch` | Yes | test_vxtensor.c |
| `vxUnmapTensorPatch` | Yes | test_vxtensor.c |

### Meta Format Functions

| Function | Implemented | CTS Test Files |
|----------|:-----------:|----------------|
| `vxSetMetaFormatAttribute` | Yes | test_graph_pipeline.c, test_graph.c, test_user_data_object.c, test_usernode.c |
| `vxSetMetaFormatFromReference` | Yes | test_graph.c, test_user_data_object.c, test_usernode.c |
| `vxQueryMetaFormatAttribute` | Yes | test_graph.c, test_nnef_import.c |

---

## Node Creation Functions

All 61 node creation functions from `vx_nodes.h` are implemented and have CTS coverage.

| Function | CTS Test Files |
|----------|----------------|
| `vxColorConvertNode` | test_convertcolor.c |
| `vxChannelExtractNode` | test_channelextract.c |
| `vxChannelCombineNode` | test_channelcombine.c |
| `vxSobel3x3Node` | test_sobel3x3.c |
| `vxMagnitudeNode` | test_magnitude.c |
| `vxPhaseNode` | test_phase.c |
| `vxScaleImageNode` | test_scale.c |
| `vxTableLookupNode` | test_graph.c, test_lut.c |
| `vxHistogramNode` | test_histogram.c |
| `vxEqualizeHistNode` | test_eqhist.c |
| `vxAbsDiffNode` | test_binop8u.c, test_binop16s.c (via macro) |
| `vxMeanStdDevNode` | test_graph_pipeline.c, test_graph.c, test_meanstddev.c |
| `vxThresholdNode` | test_threshold.c |
| `vxNonMaxSuppressionNode` | test_nonmaxsuppression.c |
| `vxIntegralImageNode` | test_graph.c, test_integral.c, test_graph_callbacks.c |
| `vxErode3x3Node` | test_erode3x3.c |
| `vxDilate3x3Node` | test_dilate3x3.c |
| `vxMedian3x3Node` | test_graph.c, test_median3x3.c |
| `vxBox3x3Node` | test_graph.c, test_box3x3.c, test_graph_callbacks.c |
| `vxGaussian3x3Node` | test_gaussian3x3.c, test_export_import_extension.c |
| `vxNonLinearFilterNode` | test_nonlinearfilter.c |
| `vxConvolveNode` | test_convolve.c |
| `vxGaussianPyramidNode` | test_graph.c, test_gaussianpyramid.c, test_optflowpyrlk.c |
| `vxLaplacianPyramidNode` | test_graph.c, test_laplacianpyramid.c |
| `vxLaplacianReconstructNode` | test_graph.c, test_laplacianpyramid.c |
| `vxWeightedAverageNode` | test_graph.c, test_weighted_average.c |
| `vxMinMaxLocNode` | test_minmaxloc.c, test_matchtemplate.c |
| `vxMinNode` | test_min.c |
| `vxMaxNode` | test_max.c |
| `vxAndNode` | test_graph_pipeline.c, test_binop8u.c (via macro) |
| `vxOrNode` | test_graph_pipeline.c, test_binop8u.c (via macro) |
| `vxXorNode` | test_binop8u.c, test_binop1u.c (via macro) |
| `vxNotNode` | test_graph_pipeline.c, test_graph.c, test_not.c |
| `vxScalarOperationNode` | test_controlflow.c |
| `vxSelectNode` | test_controlflow.c |
| `vxMultiplyNode` | test_graph.c, test_multiply.c |
| `vxAddNode` | test_addsub.c, test_graph_pipeline.c, test_graph.c, test_smoke.c, test_target.c |
| `vxSubtractNode` | test_graph.c |
| `vxConvertDepthNode` | test_convertdepth.c, test_not.c |
| `vxCannyEdgeDetectorNode` | test_canny.c, test_graph.c |
| `vxWarpAffineNode` | test_warpaffine.c |
| `vxWarpPerspectiveNode` | test_warpperspective.c |
| `vxHarrisCornersNode` | test_graph.c, test_harriscorners.c |
| `vxFastCornersNode` | test_graph.c, test_fast.c |
| `vxOpticalFlowPyrLKNode` | test_graph.c, test_optflowpyrlk.c |
| `vxRemapNode` | test_remap.c |
| `vxHalfScaleGaussianNode` | test_graph.c, test_halfscalegaussian.c |
| `vxMatchTemplateNode` | test_matchtemplate.c |
| `vxLBPNode` | test_lbp.c |
| `vxHOGCellsNode` | test_hog.c |
| `vxHOGFeaturesNode` | test_hog.c |
| `vxHoughLinesPNode` | test_houghlinesp.c |
| `vxBilateralFilterNode` | test_bilateralfilter.c |
| `vxCopyNode` | test_copy.c |
| `vxAccumulateImageNode` | test_graph.c |
| `vxAccumulateWeightedImageNode` | test_graph.c |
| `vxAccumulateSquareImageNode` | test_graph.c |
| `vxTensorMultiplyNode` | test_tensor_op.c |
| `vxTensorAddNode` | test_tensor_op.c |
| `vxTensorSubtractNode` | test_tensor_op.c |
| `vxTensorTableLookupNode` | test_tensor_op.c |
| `vxTensorTransposeNode` | test_tensor_op.c |
| `vxTensorConvertDepthNode` | test_tensor_op.c |
| `vxTensorMatrixMultiplyNode` | test_tensor_op.c |

---

## Immediate Mode Functions

All 59 immediate-mode functions from `vxu.h` are implemented and have CTS coverage.

| Function | CTS Test Files |
|----------|----------------|
| `vxuColorConvert` | test_convertcolor.c |
| `vxuChannelExtract` | test_channelextract.c, test_vximage.c |
| `vxuChannelCombine` | test_channelcombine.c |
| `vxuSobel3x3` | test_sobel3x3.c |
| `vxuMagnitude` | test_magnitude.c |
| `vxuPhase` | test_phase.c |
| `vxuScaleImage` | test_scale.c |
| `vxuTableLookup` | test_graph.c, test_lut.c |
| `vxuHistogram` | test_histogram.c |
| `vxuEqualizeHist` | test_eqhist.c |
| `vxuAbsDiff` | test_binop8u.c, test_binop16s.c (via macro) |
| `vxuMeanStdDev` | test_meanstddev.c |
| `vxuThreshold` | test_threshold.c |
| `vxuIntegralImage` | test_integral.c |
| `vxuErode3x3` | test_erode3x3.c |
| `vxuDilate3x3` | test_dilate3x3.c |
| `vxuMedian3x3` | test_median3x3.c |
| `vxuBox3x3` | test_box3x3.c |
| `vxuGaussian3x3` | test_gaussian3x3.c |
| `vxuNonLinearFilter` | test_nonlinearfilter.c |
| `vxuConvolve` | test_convolve.c |
| `vxuGaussianPyramid` | test_gaussianpyramid.c, test_laplacianpyramid.c, test_optflowpyrlk.c |
| `vxuLaplacianPyramid` | test_laplacianpyramid.c |
| `vxuLaplacianReconstruct` | test_laplacianpyramid.c |
| `vxuWeightedAverage` | test_weighted_average.c |
| `vxuMin` | test_min.c, test_matchtemplate.c |
| `vxuMax` | test_max.c |
| `vxuMinMaxLoc` | test_minmaxloc.c, test_matchtemplate.c |
| `vxuAnd` | test_binop8u.c, test_binop1u.c (via macro) |
| `vxuOr` | test_binop8u.c, test_binop1u.c (via macro) |
| `vxuXor` | test_binop8u.c, test_binop1u.c (via macro) |
| `vxuNot` | test_canny.c, test_not.c |
| `vxuMultiply` | test_graph.c, test_multiply.c |
| `vxuAdd` | test_addsub.c, test_laplacianpyramid.c, test_target.c |
| `vxuSubtract` | test_graph.c, test_laplacianpyramid.c |
| `vxuConvertDepth` | test_convertdepth.c |
| `vxuCannyEdgeDetector` | test_canny.c |
| `vxuWarpAffine` | test_warpaffine.c |
| `vxuWarpPerspective` | test_warpperspective.c |
| `vxuHarrisCorners` | test_harriscorners.c |
| `vxuFastCorners` | test_fast.c |
| `vxuOpticalFlowPyrLK` | test_optflowpyrlk.c |
| `vxuRemap` | test_remap.c |
| `vxuHalfScaleGaussian` | test_halfscalegaussian.c |
| `vxuMatchTemplate` | test_matchtemplate.c |
| `vxuLBP` | test_lbp.c |
| `vxuBilateralFilter` | test_bilateralfilter.c |
| `vxuHOGCells` | test_hog.c |
| `vxuHOGFeatures` | test_hog.c |
| `vxuHoughLinesP` | test_houghlinesp.c |
| `vxuNonMaxSuppression` | test_nonmaxsuppression.c |
| `vxuCopy` | test_copy.c |
| `vxuTensorMultiply` | test_tensor_op.c |
| `vxuTensorAdd` | test_tensor_op.c |
| `vxuTensorSubtract` | test_tensor_op.c |
| `vxuTensorTableLookup` | test_tensor_op.c |
| `vxuTensorTranspose` | test_tensor_op.c |
| `vxuTensorConvertDepth` | test_tensor_op.c |
| `vxuTensorMatrixMultiply` | test_tensor_op.c |

---

## Vision Kernel Implementations

All vision and neural network kernels are implemented in the **c_model** (reference C) backend.

| Kernel | Source File | Enum |
|--------|------------|------|
| Color Convert | `c_convertcolor.c` | `VX_KERNEL_COLOR_CONVERT` |
| Channel Extract/Combine | `c_channel.c` | `VX_KERNEL_CHANNEL_EXTRACT/COMBINE` |
| Sobel 3x3 | `c_sobel3x3.c` | `VX_KERNEL_SOBEL_3x3` |
| Magnitude | `c_magnitude.c` | `VX_KERNEL_MAGNITUDE` |
| Phase | `c_phase.c` | `VX_KERNEL_PHASE` |
| Scale Image | `c_scale.c` | `VX_KERNEL_SCALE_IMAGE` |
| Table Lookup | `c_lut.c` | `VX_KERNEL_TABLE_LOOKUP` |
| Histogram / Equalize Histogram | `c_histogram.c` | `VX_KERNEL_HISTOGRAM/EQUALIZE_HISTOGRAM` |
| Absolute Difference | `c_absdiff.c` | `VX_KERNEL_ABSDIFF` |
| Mean/StdDev | `c_statistics.c` | `VX_KERNEL_MEAN_STDDEV` |
| Threshold | `c_threshold.c` | `VX_KERNEL_THRESHOLD` |
| Integral Image | `c_integralimage.c` | `VX_KERNEL_INTEGRAL_IMAGE` |
| Erode 3x3 / Dilate 3x3 | `c_morphology.c` | `VX_KERNEL_ERODE_3x3/DILATE_3x3` |
| Median 3x3 / Box 3x3 / Gaussian 3x3 | `c_filter.c` | `VX_KERNEL_MEDIAN_3x3/BOX_3x3/GAUSSIAN_3x3` |
| Custom Convolution | `c_convolve.c` | `VX_KERNEL_CUSTOM_CONVOLUTION` |
| Gaussian Pyramid | `c_filter.c` | `VX_KERNEL_GAUSSIAN_PYRAMID` |
| Laplacian Pyramid/Reconstruct | composite | `VX_KERNEL_LAPLACIAN_PYRAMID/RECONSTRUCT` |
| Min/Max/MinMaxLoc | `c_minmax.c` | `VX_KERNEL_MIN/MAX/MINMAXLOC` |
| Convert Depth | `c_convertdepth.c` | `VX_KERNEL_CONVERTDEPTH` |
| Canny Edge Detector | composite | `VX_KERNEL_CANNY_EDGE_DETECTOR` |
| Bitwise And/Or/Xor/Not | `c_bitwise.c` | `VX_KERNEL_AND/OR/XOR/NOT` |
| Multiply | `c_multiply.c` | `VX_KERNEL_MULTIPLY` |
| Add/Subtract | `c_addsub.c` | `VX_KERNEL_ADD/SUBTRACT` |
| Warp Affine/Perspective | `c_warp.c` | `VX_KERNEL_WARP_AFFINE/PERSPECTIVE` |
| Harris Corners | composite | `VX_KERNEL_HARRIS_CORNERS` |
| FAST Corners | `c_fast9.c` | `VX_KERNEL_FAST_CORNERS` |
| Optical Flow (LK) | `c_optpyrlk.c` | `VX_KERNEL_OPTICAL_FLOW_PYR_LK` |
| Remap | `c_scale.c` | `VX_KERNEL_REMAP` |
| Half-Scale Gaussian | `c_filter.c` | `VX_KERNEL_HALFSCALE_GAUSSIAN` |
| Non-Linear Filter | `c_nonlinearfilter.c` | `VX_KERNEL_NON_LINEAR_FILTER` |
| Match Template | `c_matchtemplate.c` | `VX_KERNEL_MATCH_TEMPLATE` |
| LBP | `c_lbp.c` | `VX_KERNEL_LBP` |
| Hough Lines P | `c_houghlinesp.c` | `VX_KERNEL_HOUGH_LINES_P` |
| HOG Cells/Features | `c_hog.c` | `VX_KERNEL_HOG_CELLS/FEATURES` |
| Bilateral Filter | `c_bilateral_filter.c` | `VX_KERNEL_BILATERAL_FILTER` |
| Non-Max Suppression | `c_nonmaxsuppression.c` | `VX_KERNEL_NON_MAX_SUPPRESSION` |
| Copy | `c_copy.c` | `VX_KERNEL_COPY` |
| Select / Scalar Operation | `c_controlflow.c` | `VX_KERNEL_SELECT/SCALAR_OPERATION` |
| Weighted Average | `c_weighted_average.c` | `VX_KERNEL_WEIGHTED_AVERAGE` |
| Accumulate/Weighted/Square | `c_accumulate.c` | compatibility kernels |
| Tensor Add/Sub/Multiply | `c_tensor_op.c` | `VX_KERNEL_TENSOR_ADD/SUBTRACT/MULTIPLY` |
| Tensor Table Lookup | `c_tensor_lut.c` | `VX_KERNEL_TENSOR_TABLE_LOOKUP` |
| Tensor Transpose | `c_tensor_transpose.c` | `VX_KERNEL_TENSOR_TRANSPOSE` |
| Tensor Convert Depth | `c_tensor_op.c` | `VX_KERNEL_TENSOR_CONVERT_DEPTH` |
| Tensor Matrix Multiply | `c_tensor_multiply_matrix.c` | `VX_KERNEL_TENSOR_MATRIX_MULTIPLY` |

**Additional kernel backends:**
- **venum** (`kernels/venum/`): NEON-optimized for ARM (Raspberry Pi 3B+)
- **tiling** (`kernels/tiling/`): Tiling-based implementations
- **opencl** (`kernels/opencl/`): OpenCL GPU implementations

### Neural Network Kernels

| NN Kernel | Source File | CTS Test |
|-----------|------------|----------|
| Convolution Layer | `c_khr_nn.c` | test_tensor_nn.c |
| Fully Connected Layer | `c_khr_nn.c` | test_tensor_nn.c |
| Pooling Layer | `c_khr_nn.c` | test_tensor_nn.c |
| Softmax Layer | `c_khr_nn.c` | test_tensor_nn.c |
| Local Response Normalization | `c_khr_nn.c` | Networks/src/graph_alexnet.c |
| Activation Layer | `c_khr_nn.c` | test_tensor_nn.c |
| ROI Pooling | `c_khr_nn.c` | test_tensor_nn.c |
| Deconvolution Layer | `c_khr_nn.c` | test_tensor_nn.c |

---

## Extension API Coverage

### Import/Export (IX)

**Status: Fully Implemented | CTS: Full Coverage**

| Function | Implemented | CTS Coverage |
|----------|:-----------:|:------------:|
| `vxExportObjectsToMemory` | Yes | test_export_import_extension.c |
| `vxReleaseExportedMemory` | Yes | test_export_import_extension.c |
| `vxImportObjectsFromMemory` | Yes | test_export_import_extension.c |
| `vxReleaseImport` | Yes | test_export_import_extension.c |
| `vxGetImportReferenceByName` | Yes | test_export_import_extension.c |

### Neural Networks (NN)

**Status: Fully Implemented | CTS: Full Coverage**

| Function | Implemented | CTS Coverage |
|----------|:-----------:|:------------:|
| `vxConvolutionLayer` | Yes | test_tensor_nn.c |
| `vxFullyConnectedLayer` | Yes | test_tensor_nn.c |
| `vxPoolingLayer` | Yes | test_tensor_nn.c |
| `vxSoftmaxLayer` | Yes | test_tensor_nn.c |
| `vxLocalResponseNormalizationLayer` | Yes | Networks/src/graph_alexnet.c |
| `vxActivationLayer` | Yes | test_tensor_nn.c |
| `vxROIPoolingLayer` | Yes | test_tensor_nn.c |
| `vxDeconvolutionLayer` | Yes | test_tensor_nn.c |

### User Data Object

**Status: Partially Implemented | CTS: Covers All Implemented Functions**

| Function | Implemented | CTS Coverage |
|----------|:-----------:|:------------:|
| `vxCreateUserDataObject` | Yes | test_user_data_object.c |
| `vxCreateVirtualUserDataObject` | No | -- |
| `vxReleaseUserDataObject` | Yes | test_user_data_object.c |
| `vxQueryUserDataObject` | Yes | test_user_data_object.c |
| `vxSetUserDataObjectAttribute` | No | -- |
| `vxCopyUserDataObject` | Yes | test_user_data_object.c |
| `vxMapUserDataObject` | Yes | test_user_data_object.c |
| `vxUnmapUserDataObject` | Yes | test_user_data_object.c |

### NNEF Import Kernel

**Status: Fully Implemented | CTS: Full Coverage**

| Function | Implemented | CTS Coverage |
|----------|:-----------:|:------------:|
| `vxImportKernelFromURL` | Yes | test_nnef_import.c |

### Pipelining

**Status: Stub (VX_ERROR_NOT_IMPLEMENTED) | CTS: Partial Coverage**

| Function | Implemented | CTS Coverage |
|----------|:-----------:|:------------:|
| `vxSetGraphScheduleConfig` | Stub | test_graph_pipeline.c |
| `vxGraphParameterEnqueueReadyRef` | Stub | test_graph_pipeline.c |
| `vxGraphParameterDequeueDoneRef` | Stub | test_graph_pipeline.c |
| `vxGraphParameterCheckDoneRef` | Stub | test_graph_pipeline.c |
| `vxGetGraphParameterRefsList` | No | -- |
| `vxAddReferencesToGraphParameterList` | No | -- |
| `vxGetKernelParameterConfig` | No | -- |
| `vxGetGraphParameterConfig` | No | -- |

### Streaming

**Status: Stub (VX_ERROR_NOT_IMPLEMENTED) | CTS: Full Coverage**

| Function | Implemented | CTS Coverage |
|----------|:-----------:|:------------:|
| `vxEnableGraphStreaming` | Stub | test_graph_streaming.c |
| `vxStartGraphStreaming` | Stub | test_graph_streaming.c |
| `vxStopGraphStreaming` | Stub | test_graph_streaming.c |

### Event Queue

**Status: Stub (VX_ERROR_NOT_IMPLEMENTED) | CTS: Partial Coverage**

| Function | Implemented | CTS Coverage |
|----------|:-----------:|:------------:|
| `vxEnableEvents` | Stub | test_graph_pipeline.c |
| `vxDisableEvents` | Stub | test_graph_pipeline.c |
| `vxSendUserEvent` | Stub | test_graph_pipeline.c |
| `vxWaitEvent` | Stub | test_graph_pipeline.c |
| `vxRegisterEvent` | Stub | test_graph_pipeline.c |
| `vxRegisterGraphEvent` | No | -- |
| `vxWaitGraphEvent` | No | -- |
| `vxEnableGraphEvents` | No | -- |
| `vxDisableGraphEvents` | No | -- |
| `vxSendUserGraphEvent` | No | -- |

### Compatibility/Deprecated

**Status: Implemented | CTS: Partial Coverage**

| Function | CTS Coverage |
|----------|:------------:|
| `vxAccessImagePatch` | test_vximage.c |
| `vxCommitImagePatch` | test_vximage.c |
| `vxAccessArrayRange` | test_optflowpyrlk.c |
| `vxCommitArrayRange` | test_optflowpyrlk.c |
| `vxAddKernel` | -- |
| `vxComputeImagePatchSize` | -- |
| `vxAccessDistribution` | -- |
| `vxCommitDistribution` | -- |
| `vxAccessLUT` | -- |
| `vxCommitLUT` | -- |
| `vxReadMatrix` | -- |
| `vxWriteMatrix` | -- |
| `vxReadConvolutionCoefficients` | -- |
| `vxWriteConvolutionCoefficients` | -- |
| `vxReadScalarValue` | -- |
| `vxWriteScalarValue` | -- |
| `vxSetRemapPoint` | -- |
| `vxGetRemapPoint` | -- |
| `vxCreateThreshold` (old signature) | -- |

> Deprecated functions are superseded by their modern equivalents (e.g., `vxCopyImagePatch` replaces `vxAccessImagePatch`/`vxCommitImagePatch`). The modern equivalents all have full CTS coverage.

### Unimplemented Extensions

The following extension headers are present in `api-docs/include/VX/` but have **no implementation** in the sample. They are optional KHR extensions not required for conformance.

| Extension | Header | Functions | Status |
|-----------|--------|-----------|--------|
| Node Send Command | `vx_khr_node_send_command.h` | 2 | No implementation, no CTS |
| Bidirectional Parameters | `vx_khr_bidirectional_parameters.h` | 2 | No implementation, no CTS |
| Buffer Aliasing | `vx_khr_buffer_aliasing.h` | 2 | No implementation, no CTS |
| Swap/Move | `vx_khr_swap_move.h` | 4 | No implementation, no CTS |
| OpenCL Interop | `vx_khr_opencl_interop.h` | 1 | Partial (context creation only), no CTS |
| Raw Image | `vx_khr_raw_image.h` | 3 | No implementation, no CTS |
| Safe Casts | `vx_khr_safe_casts.h` | macros | Header-only (no implementation needed), no CTS |
| ICD Loader | `vx_khr_icd.h` | 3 | Minimal (context alias only), no CTS |
| Sub-Image Object Array | `vx_khr_sub_image_object_array.h` | 2 | No implementation, no CTS |
| Supplementary Data | `vx_khr_supplementary_data.h` | 3 | No implementation, no CTS |
| Target Kernel | `vx_khr_target_kernel.h` | 8 | No implementation, no CTS |
| Tensor From Image | `vx_khr_tensor_from_image.h` | 5 | No implementation, no CTS |
| Tiling | `vx_khr_tiling.h` | 1 | No implementation, no CTS |
| XML | `vx_khr_xml.h` | 4 | Implemented (off by default), no CTS |
| Classifier | `vx_khr_class.h` | 3 | No implementation, no CTS |

---

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `OPENVX_CONFORMANCE_VISION` | ON | Vision conformance feature set |
| `OPENVX_CONFORMANCE_NEURAL_NETWORKS` | ON | Neural Networks conformance |
| `OPENVX_CONFORMANCE_NNEF_IMPORT` | ON | NNEF Import conformance |
| `OPENVX_USE_ENHANCED_VISION` | ON | Enhanced Vision features |
| `OPENVX_USE_IX` | ON | Import/Export extension |
| `OPENVX_USE_NN` | ON | Neural Network extension |
| `OPENVX_USE_USER_DATA_OBJECT` | ON | User Data Object extension |
| `OPENVX_USE_U1` | ON | Binary (U1) image support |
| `OPENVX_USE_PIPELINING` | OFF | Pipelining extension (stubs) |
| `OPENVX_USE_STREAMING` | OFF | Streaming extension (stubs) |
| `OPENVX_USE_OPENCL_INTEROP` | OFF | OpenCL interop |
| `OPENVX_USE_S16` | OFF | Extended S16 support |
| `OPENVX_USE_TILING` | OFF | Tiling extension |
| `OPENVX_USE_XML` | OFF | XML import/export |
| `EXPERIMENTAL_USE_VENUM` | OFF | ARM NEON kernels |
| `EXPERIMENTAL_USE_OPENCL` | OFF | OpenCL kernel backend |
| `EXPERIMENTAL_PLATFORM_SUPPORTS_16_FLOAT` | OFF | FP16 support |
