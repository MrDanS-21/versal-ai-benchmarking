# versal-ai-benchmarking

Benchmarking AI workloads such as matrix multiplication (GeMM) on the Versal VCK190. Evaluates performance across the Processing System (PS), Programmable Logic (PL), and AI Engine (AIE), and analyses AXI/DMA interface throughput and bottlenecks.

The AIE and DSP designs are based on AMD's [Versal GeMM Implementation Using Vitis Acceleration Library and DSP58 tutorial](https://github.com/Xilinx/Vitis-Tutorials/tree/2025.2/AI_Engine_Development/AIE/Design_Tutorials) (Vitis 2025.2). The ARM baseline and the timing instrumentation in each host application were added on top of it.

## Implementations

| Directory | Compute target | Description |
|-----------|----------------|-------------|
| [ARM/](ARM/) | PS (Cortex-A72) | Software GeMM baseline (`uint16` inputs, `uint32` output), compiled at a selectable optimisation level. |
| [DSP/](DSP/) | PL (DSP58) | RTL systolic array of 1024 DSP58s (32 cascade chains of 32), clocked at 700 MHz by default. |
| [AIE/](AIE/) | AI Engine | 24-core GeMM overlay built with the Vitis DSP Library `matrix_mult` graph, fed by an HLS data mover in the PL. |

Each design supports square matrix sizes of 32, 64, 128, 256, 512 and 1024. The AIE and DSP subdirectories keep AMD's original READMEs, which describe the hardware and software design in detail.

## Directory Structure

```
versal-ai-benchmarking
|__ARM
|    |Makefile.....................build the A72 application and SD card image
|    |design
|         |host_app_src...........main.cpp (GeMM + timing)
|         |data...................gen_gemm.py and generated matrix headers per size
|         |exec_scripts...........run_script.sh run on the board
|__DSP
|    |Makefile
|    |design
|         |pl_src.................RTL GeMM, constraints and memory init files
|         |host_app_src...........XRT host application and matrix headers
|         |system_configs.........v++ link config
|         |exec_scripts...........run_script.sh run on the board
|__AIE
|    |Makefile
|    |design
|         |aie_src................ADF graph and aiesimulator data
|         |pl_src.................HLS data mover (dma_hls)
|         |host_app_src...........XRT host application
|         |system_configs.........v++ link config
|         |profiling_configs......xrt.ini
|         |exec_scripts...........run_script.sh run on the board
|__ _ide.........................Vitis Unified IDE workspace files
```

## Setup

Requirements: Vitis 2025.2, the `xilinx_vck190_base_202520_1` platform, the `xilinx-versal-common-v2025.2` common image and, for the AIE design, the [Vitis Libraries](https://github.com/Xilinx/Vitis_Libraries) (`DSPLIB_VITIS`).

In each implementation directory, edit `sample_env_setup.sh` with your install paths, then source it:

```bash
source sample_env_setup.sh
```

## Building

Builds are driven by each directory's Makefile. Run `make help` for the full list of targets and options.

```bash
# ARM baseline (regenerate matrix headers first with: make gen_gemm)
cd ARM && make sd_card GEMM_SIZE=256 OPT=3

# DSP58 design
cd DSP && make sd_card TARGET=hw GEMM_SIZE=256

# AI Engine design
cd AIE && make sd_card TARGET=hw GEMM_SIZE=256 EN_TRACE=1
```

Common options:

- `GEMM_SIZE`: 32 (default), 64, 128, 256, 512, 1024
- `TARGET`: `hw` or `hw_emu` (ARM supports `hw` only)
- `OPT`: compiler optimisation level, ARM only (0, 1, 2, 3, s)
- `PL_FREQ`: PL clock in MHz (DSP default 700, AIE default 312.5)

Output goes to `build/gemm_<N>x<N>x<N>/...` (`build_<PL_FREQ>/` for DSP). The SD card image is in the `package/` subfolder of the target build directory.

> **Note:** The ARM Makefile packages against the DSP design's XSA through a hard-coded `SHELL_XSA` path. Build the DSP design first, or update `SHELL_XSA` to point to a valid `.xsa`.

## Running on the VCK190

Write `package/sd_card.img` to an SD card, boot the board, then run:

```bash
cd /mnt/sd-mmcblk0p1
./run_script.sh
```

Each application checks its output against the golden data, prints `TEST PASSED` or `TEST FAILED`, and reports timing from the Linux side:

| Metric | Meaning |
|--------|---------|
| `*_hw_ms` | Compute-only time: GeMM loop (ARM), hardware cycle counter (DSP). For AIE, read it from `xrt.run_summary` in `vitis_analyzer`. |
| `linux_compute_ms` | Time from kernel start to done, as measured by the host (includes buffer copies for ARM). |
| `linux_e2e_ms` | End-to-end time including data staging. |

The ARM application also reports throughput in GOPS (2·N³ operations per GeMM).

## References

- [Original AMD Vitis Tutorials repository](https://github.com/Xilinx/Vitis-Tutorials)
- [Vitis Unified Software Platform Documentation (UG1416)](https://docs.amd.com/v/u/en-US/ug1416-vitis-documentation)
- [AI Engine Architecture Manual (AM009)](https://docs.amd.com/r/en-US/am009-versal-ai-engine/Revision-History)
- [Vitis DSP Library documentation](https://docs.amd.com/r/en-US/Vitis_Libraries/dsp/index.html)
- [XRT documentation](https://xilinx.github.io/XRT/master/html/index.html)
