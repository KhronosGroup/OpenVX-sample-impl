# OpenVX-sample-impl Conformance CI

This document describes the OpenVX 1.3 conformance coverage of the
`KhronosGroup/OpenVX-sample-impl` CI workflow (`.github/workflows/ci.yml`).
It maps every enabled feature set / KHR extension to the upstream
[OpenVX-cts](https://github.com/KhronosGroup/OpenVX-cts) test band that
exercises it.

## Workflows

| Workflow | Purpose |
|:---|:---|
| `.github/workflows/ci.yml` | Full OpenVX 1.3 + KHR extension conformance matrix, code coverage, automated PR-vs-base perf gate, and optional same-runner benchmark comparison against rustVX. |

## Build feature flags

The sample implementation is built with every conformance feature enabled:

| Feature / extension | `Build.py` flag | CTS CMake flag |
|:---|:---|:---|
| Vision conformance | `--conf_vision` | `OPENVX_CONFORMANCE_VISION=ON` |
| Enhanced Vision | `--enh_vision` | `OPENVX_USE_ENHANCED_VISION=ON` |
| Neural Networks | `--conf_nn --nn` | `OPENVX_CONFORMANCE_NEURAL_NETWORKS=ON`, `OPENVX_USE_NN=ON` |
| NN 16-bit | `--nn` | `OPENVX_USE_NN_16=ON` |
| Import/Export KHR | `--ix` | `OPENVX_USE_IX=ON` |
| Pipelining KHR | `--pipelining` | `OPENVX_USE_PIPELINING=ON` |
| Streaming KHR | `--streaming` | `OPENVX_USE_STREAMING=ON` |
| User Data Object KHR | `--userdataobj` | `OPENVX_USE_USER_DATA_OBJECT=ON` |

`--conf_nnef` / `OPENVX_CONFORMANCE_NNEF_IMPORT=ON` is not enabled by default
because the NNEF-Tools parser submodule is required; it can be added once the
parser dependency is wired into the sample-impl build.

## Test matrix

| CI job | CTS filter | Feature set / extension | Notes |
|:---|:---|:---|:---|
| `build-debug` | — | All | Debug build with coverage instrumentation. |
| `build-release` | — | All | Release build for benchmarking. |
| `build-cts` | — | All | Builds `vx_test_conformance` against the Debug sample impl. |
| `cts-baseline` | `GraphBase.*:SmokeTestBase.*:SmokeTest.*:TargetBase.*:Target.*:Logging.*` | Base / core | Smoke + baseline + code-coverage collection. |
| `cts-vision-kernels` | All core 2D vision kernels | Vision | Box, Gaussian, Sobel, magnitude, phase, color, arithmetic, geometry, features, statistics, pyramids, optical flow. |
| `cts-enhanced-vision` | `Graph/Conv*`, `HOG*`, `LBP`, `BilateralFilter`, `ControlFlow`, `TensorOp`, `Tensor`, `TensorEnhanced`, etc. | Enhanced Vision | Tensors, HOG, LBP, bilateral filter, control flow, advanced filters, feature extraction, post-processing. |
| `cts-neural-networks` | `TensorNetworks.*:-AlexNetTestNetwork:*NN*:*NNAndNNEF*` | Neural Networks NN/16 | AlexNet test is excluded because ImageNet weights are not shipped in the public CTS. Marked `continue-on-error` for NN/16 stability. |
| `cts-ix` | `Graph/ExportImport*:*IX*` | Import/Export KHR | Object serialization tests. |
| `cts-graph-features` | `GraphDelay.*:GraphROI.*:GraphCallback.*:GraphPipeline.*:GraphStreaming.*:GraphPipe*` | Graph + Pipelining + Streaming | Marked `continue-on-error` because the C model target pipelining is incomplete upstream. |
| `cts-data-objects` | `Array.*:Image.*:Scalar.*:Matrix.*:Distribution.*:LUT.*:Remap.*:Tensor.*:ObjectArray.*:UserDataObject.*` | Data objects + User Data Object KHR | |
| `cts-user-kernels` | `UserNode.*:UserKernel.*` | User-defined kernels/nodes | |

## Performance gates

### PR vs base ref (`perf-gate`)

On every pull request, the workflow builds the sample implementation Release
from both the PR head and the merge target (`${{ github.base_ref }}`), builds
`openvx-mark` against each on the **same runner VM**, and runs the same FHD
benchmark workload. The results are fed to `.github/scripts/perf_gate.py`,
which enforces:

| Threshold | Default | Meaning |
|:---|---:|---|
| Geomean floor | `0.97x` | Aggregate PR throughput may not regress more than 3%. |
| Per-kernel floor | `0.90x` | No single benchmark may regress more than 10%. |
| Warn floor | `0.95x` | Kernels between 5% and 10% slower produce an advisory. |
| Max CV% | `5.0%` | Noisy kernels are skipped rather than false-failing. |

The gate retries up to three times if a single attempt fails, to tolerate
within-runner noise; if it still fails, the regression is treated as real.

### Benchmark vs rustVX (`benchmark-vs-rustvx`)

The workflow also downloads the latest rustVX Release artifact (built by the
`kiritigowda/rustVX` `conformance.yml` workflow on `main`) and benchmarks it
on the same runner that benchmarks the Khronos sample. This produces an
informational, same-hardware speedup comparison and is marked
`continue-on-error` because rustVX artifact availability is outside this repo's
control.

## Local reproduction

```bash
# 1. Build the sample implementation (Debug + all extensions)
python3 Build.py \
  --os=Linux --arch=64 --conf=Debug --build=true \
  --conf_vision --enh_vision --conf_nn --nn --ix \
  --pipelining --streaming --userdataobj

# 2. Build the CTS against it
cd OpenVX-cts
mkdir -p build && cd build
cmake .. \
  -DOPENVX_INCLUDES="$PWD/../../install/Linux/x64/Debug/include" \
  -DOPENVX_LIBRARIES="$PWD/../../install/Linux/x64/Debug/lib/libopenvx.so;$PWD/../../install/Linux/x64/Debug/lib/libvxu.so;pthread;dl;m;rt" \
  -DOPENVX_CONFORMANCE_VISION=ON \
  -DOPENVX_USE_ENHANCED_VISION=ON \
  -DOPENVX_CONFORMANCE_NEURAL_NETWORKS=ON \
  -DOPENVX_USE_NN=ON \
  -DOPENVX_USE_NN_16=ON \
  -DOPENVX_USE_IX=ON \
  -DOPENVX_USE_PIPELINING=ON \
  -DOPENVX_USE_STREAMING=ON \
  -DOPENVX_USE_USER_DATA_OBJECT=ON
make -j$(nproc)

# 3. Run a single band
export LD_LIBRARY_PATH=$PWD/../../install/Linux/x64/Debug/lib
export VX_TEST_DATA_PATH=$PWD/../test_data/
./bin/vx_test_conformance --filter="GraphBase.*:SmokeTest.*" --verbose
```

## Future work

1. Enable `OPENVX_CONFORMANCE_NNEF_IMPORT=ON` once the NNEF-Tools parser is
   integrated into the sample-impl build and CTS CMake path.
2. Promote `cts-graph-features` and `cts-neural-networks` to required once
   the underlying C model implementation gaps are resolved upstream.
3. Add `lcov` / `gcov` coverage thresholds to `cts-baseline` so PRs cannot
   silently drop coverage.
4. Extend `benchmark-vs-rustvx` to also compare against other OpenVX
   implementations on the same runner.
