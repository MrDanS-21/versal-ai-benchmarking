/*
Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.
SPDX-License-Identifier: MIT
*/

#include <cstdint>
#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cmath>
#include <unistd.h>
#include <chrono>
#include "xrt/xrt_aie.h"
#include "xrt/xrt_kernel.h"
#include "xrt/xrt_bo.h"
#include "matrix_A_data.h"
#include "matrix_B_data.h"
#include "output_data.h"

//#define DEBUG_PRINT

#ifdef DEBUG_PRINT
#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DPRINTF(...) ((void)0)
#endif

// A - MxN, B - NxL, OUT - MxL
#if (M_LARGE == 32 & N_LARGE ==32) 
   #define MATRIX_A_SIZE (M_LARGE*N_LARGE)
   #define MATRIX_B_SIZE (N_LARGE*L_LARGE)
#else 
   #define MATRIX_A_SIZE (M_LARGE*N_LARGE/2)
   #define MATRIX_B_SIZE (N_LARGE*L_LARGE/2)
#endif


#define OUTPUT_SIZE   (M_LARGE*L_LARGE)
unsigned int NUM_COL_URAM = (16*ceil(float(N_LARGE*L_LARGE)/float(262144)));
unsigned int NUM_ROW_URAM = (16*ceil(float(N_LARGE*L_LARGE)/float(262144)));
unsigned int NUM_OP_URAM  = 64; 
//unsigned int NUM_COL_URAM = (8*ceil(float(N_LARGE*L_LARGE)/float(131072)));
//unsigned int NUM_OP_URAM  = (32*ceil(float(M_LARGE*L_LARGE)/float(524288)));
extern uint32_t matrix_A_data[MATRIX_A_SIZE]; 
extern uint32_t matrix_B_data[MATRIX_B_SIZE]; 
extern uint64_t output_data[OUTPUT_SIZE]; 

namespace {
using clock = std::chrono::steady_clock;

inline double ms(clock::duration d) {
  return std::chrono::duration<double, std::milli>(d).count();
}
}

using namespace std;

static std::vector<char>
load_xclbin(xrtDeviceHandle device, const std::string& fnm)
{
   if (fnm.empty())
   throw std::runtime_error("No xclbin speified");

   // load bit stream
   std::ifstream stream(fnm);
   stream.seekg(0,stream.end);
   size_t size = stream.tellg();
   stream.seekg(0,stream.beg);

   std::vector<char> header(size);
   stream.read(header.data(),size);

   auto top = reinterpret_cast<const axlf*>(header.data());
   if (xrtDeviceLoadXclbin(device, top))
   throw std::runtime_error("Bitstream download failed");

   return header;
}

class gemm 
{

public:
xrtKernelHandle gemm_top_khdl;
xrtRunHandle gemm_top_rhdl;
uint32_t instance_errCnt;
//uint64_t GEMM_BASE_ADDR = 0xA4090000; 
//uint64_t GEMM_BASE_ADDR = 0xA4050000; 

//void init(xrtDeviceHandle dhdl, const axlf *top, char insts)
void init(xrtDeviceHandle dhdl, const axlf *top)
{

std::string gemm_top_obj_str = "gemm_large_ocm:{gemm_large_ocm_0}";
const char *gemm_top_obj = gemm_top_obj_str.c_str();

// Open kernel handle exclusively to read the ap_return register later for reporting error...
gemm_top_khdl = xrtPLKernelOpenExclusive(dhdl, top->m_header.uuid, gemm_top_obj);
gemm_top_rhdl = xrtRunOpen(gemm_top_khdl);

DPRINTF ("Initialized gemm_top...\n");

}

void gemm_soft_reset_pulse(void) 
{
    DPRINTF("Writing soft_reset\n");
    xrtKernelWriteRegister(gemm_top_khdl, 0x34,  0x1);
    //usleep(100);
    xrtKernelWriteRegister(gemm_top_khdl, 0x34,  0x0);
}
void gemm_bring_up(void) 
{

unsigned int i, j;
uint32_t uram_data;
unsigned int waddr;
    DPRINTF("Writing into registers\n");
    // 1. Write to Control register with Address autoincrement bit set to 1
    //    Write to address 0x10 data = 0x2
    xrtKernelWriteRegister(gemm_top_khdl, 0x10,  0x2);
    // 2. Write to Indirect address register value of 0
    //    Write to address 0x1C, data = 0
    xrtKernelWriteRegister(gemm_top_khdl, 0x1C,  0x0);

    // 3. Write to indirect address control register, Valid bit = 1, R/W# = 0
    //    Write to address 0x18 data = 0x1
    xrtKernelWriteRegister(gemm_top_khdl, 0x18,  0x1);

    // Write 16 32x32 A Matrices into Row URAMs at adress 0x20
    // Size of each Matrix is 2KB, total size = 32KB
    // Data is arrangde in 32-bit wide entry (4Byte)
    // So total lines = 8K
     
    DPRINTF("Writing Matrix A\n");
    waddr = 0;
    //for (i=0; i<64; i=i+1) {  // 64 Row URAMs
    //   for (j=0; j<(MATRIX_A_SIZE/64); j=j+1) { // 128 32-bit locations 
    //      uram_data = matrix_A_data[MATRIX_A_SIZE/64*i+j]; 
    for (i=0; i<NUM_ROW_URAM; i=i+1) {   // Only 8 URAMs are populated
       for (j=0; j<(MATRIX_A_SIZE/NUM_ROW_URAM); j=j+1) { // 1024 locations written to 8 URAMs
          uram_data = matrix_A_data[MATRIX_A_SIZE/NUM_COL_URAM*i+j]; 
          xrtKernelWriteRegister (gemm_top_khdl, 0x20, uram_data);
       }
       // Increment the address
       waddr += 0x8000;
       xrtKernelWriteRegister (gemm_top_khdl, 0x1c, waddr);
    }
    
    waddr = 0x200000;
    xrtKernelWriteRegister (gemm_top_khdl, 0x1c, waddr);
    DPRINTF("Writing Matrix B\n");
    for (i=0; i<NUM_COL_URAM; i=i+1) {   // Only 8 URAMs are populated
       for (j=0; j<(MATRIX_B_SIZE/NUM_COL_URAM); j=j+1) { // 1024 locations written to 8 URAMs
          uram_data = matrix_B_data[MATRIX_B_SIZE/NUM_COL_URAM*i+j]; 
          xrtKernelWriteRegister (gemm_top_khdl, 0x20, uram_data);
       }
       // Increment the address
       waddr += 0x8000;
       xrtKernelWriteRegister (gemm_top_khdl, 0x1c, waddr);
    }
    
    // Set DUT Enable bit
    // Write to address 0x10, data = 0x3
    // xrtKernelWriteRegister (gemm_top_khdl, 0x10, 0x3); 

}

void read_perf ()
{

uint32_t perf_counter;

// Read perf counter at address offset 0x30
xrtKernelReadRegister(gemm_top_khdl, 0x30, &perf_counter);
DPRINTF ("Perf Counter : %d\n", perf_counter);

}

uint32_t get_cycle ()
{

uint32_t cycle;
xrtKernelReadRegister(gemm_top_khdl, 0x30, &cycle);
return cycle;

}

void check_done ()
{

unsigned int Done = 0;
uint32_t  read_data;

    // Poll for Done bit from DUT
    DPRINTF("entered golden_check");
    while (Done == 0) {
        // Read address 4
        xrtKernelReadRegister(gemm_top_khdl, 0x14, &read_data);
        Done = read_data & 0x1;
    }
}



void golden_check(uint32_t &mismatch_count) 
{

uint16_t golden_data_masked_lower;
uint16_t golden_data_masked_upper;
uint16_t read_data_masked_lower;
uint16_t read_data_masked_upper;
uint32_t read_data;
uint32_t read_addr;
unsigned int i, j, Done;
uint32_t match_count;
unsigned int error_count;
unsigned int raddr;

    // Read 1 large 1Kx1K outpu Matrix from Output URAMs from address 0x24
    // Total data is 1M. 4K depth from 64 URAMs 
    match_count = 0;
    mismatch_count = 0;
    error_count = 0;
    raddr = 0;
    for (i=0; i<NUM_OP_URAM; i=i+1) {  // 32 Output URAMs
      for (j=0; j<(OUTPUT_SIZE/NUM_OP_URAM); j=j+2) {
         golden_data_masked_lower = output_data [(OUTPUT_SIZE/NUM_OP_URAM)*i+j] & 0xFFFF;
         golden_data_masked_upper = output_data [(OUTPUT_SIZE/NUM_OP_URAM)*i+j+1] & 0xFFFF;
         xrtKernelReadRegister (gemm_top_khdl, 0x24, &read_data);
         read_data_masked_lower = read_data & 0xFFFF;
         read_data_masked_upper = read_data >> 16;
         if (golden_data_masked_lower != read_data_masked_lower) {
            DPRINTF ("Data mismatch Addr : 0x%x, Golden Data : 0x%x, Read Data : 0x%x\n", (OUTPUT_SIZE/NUM_OP_URAM)*i+j, golden_data_masked_lower, read_data_masked_lower);
            mismatch_count++;
            error_count++;
         } else {
            //DPRINTF ("Data match Addr : 0x%x, Golden Data : 0x%x, Read Data : 0x%x\n", (OUTPUT_SIZE/NUM_OP_URAM)*i+j, golden_data_masked_lower, read_data_masked_lower);
            match_count++;
         }
         if (golden_data_masked_upper != read_data_masked_upper) {
            DPRINTF ("Data mismatch Addr : 0x%x, Golden Data : 0x%x, Read Data : 0x%x\n", (OUTPUT_SIZE/NUM_OP_URAM)*i+j+1, golden_data_masked_upper, read_data_masked_upper);
            mismatch_count++;
            error_count++;
         } else {
            //DPRINTF ("Data match Addr : 0x%x, Golden Data : 0x%x, Read Data : 0x%x\n", (OUTPUT_SIZE/NUM_OP_URAM)*i+j+1, golden_data_masked_upper, read_data_masked_upper);
            match_count++;
         }
      }
      // Change the address
      raddr += 0x8000;
      xrtKernelWriteRegister (gemm_top_khdl, 0x28, raddr);
      if (error_count > 10) {
         break;
      }
    }  
    DPRINTF ("Match Count : %u, Mismatch Count : %u\n", match_count, mismatch_count);
}

int test_gemm() 
{
    int result = 1;    
    usleep(10);
    gemm_bring_up();
    return result;
}

void start_gemm()
{
  xrtKernelWriteRegister(gemm_top_khdl, 0x10, 0x3);
}

};

int main(int argc, char ** argv)
{
    uint32_t mismatch_count = 0;
    int i = 0;

    //////////////////////////////////////////
    //    Open xclbin
    ////////////////////////////////////////////
    
    if(argc != 2) {
        std::cout << "Usage: " << argv[0] <<" <xclbin>" << std::endl;
        return EXIT_FAILURE;
    }

    //Init timing
    uint32_t dsp_hw_cycles_delta = 0;
    double   dsp_hw_compute_ms   = -1.0;
    double   linux_e2e_ms        = -1.0;
    double   linux_compute_ms    = -1.0;
    
    char* xclbinFilename = argv[1];
    
    auto dhdl = xrtDeviceOpen(0);
    auto xclbin = load_xclbin(dhdl, xclbinFilename);
    auto top = reinterpret_cast<const axlf*>(xclbin.data());

    // Instantiate gemm class
    gemm  gemm_obj;
    
    DPRINTF("BEGIN TEST\n");
    DPRINTF("MATRIX_A_SIZE %d\n",MATRIX_A_SIZE);
    DPRINTF("MATRIX_B_SIZE %d\n",MATRIX_B_SIZE);
    DPRINTF("OUTPUT_SIZE %d\n",OUTPUT_SIZE);
    DPRINTF("NUM_OP_URAM %d\n",NUM_OP_URAM);
    DPRINTF("NUM_COL_URAM %d\n",NUM_COL_URAM);
    
    gemm_obj.init(dhdl,top);
    gemm_obj.gemm_soft_reset_pulse();
    auto t_linux_e2e_start = clock::now();
    gemm_obj.test_gemm();
    auto t_linux_compute_start = clock::now();
    auto dsp_hw_cycles_start = gemm_obj.get_cycle();
    gemm_obj.start_gemm();

    while ( i < 1 ) {
    //while (1) {
    gemm_obj.check_done();
    auto dsp_hw_cycles_end = gemm_obj.get_cycle();
    auto t_linux_end = clock::now();
    DPRINTF ("Read Perf Counter\n");
    gemm_obj.read_perf();
    DPRINTF("Started DI check \n");
    gemm_obj.golden_check(mismatch_count);

    dsp_hw_cycles_delta = dsp_hw_cycles_end - dsp_hw_cycles_start;
    linux_compute_ms = ms(t_linux_end - t_linux_compute_start);
    linux_e2e_ms = ms(t_linux_end - t_linux_e2e_start);

    std::cout << "TEST " << (mismatch_count ? "FAILED" : "PASSED") << std::endl;
    gemm_obj.gemm_soft_reset_pulse();
    //xrtKernelWriteRegister(gemm_top_khdl, 0x34,  0x1);
    i = i + 1;
    }

    printf("END TEST\n");

    // CLOCK_FREQ assumed to be DSP clock in Hz
    const double dsp_freq_hz      = (double)CLOCK_FREQ;
    const double logic_freq_hz    = dsp_freq_hz / 2.0;
    const unsigned logic_freq_mhz = (unsigned)(logic_freq_hz / 1e6 + 0.5);
    const double dsp_hw_ms        = ((double)dsp_hw_cycles_delta / logic_freq_hz) * 1000.0;
   
    printf("\n================ DSP GEMM TIMING (Linux POV) ================\n");
    printf("dsp_hw_cycles    (counter)             : %u cycles\n", (unsigned)dsp_hw_cycles_delta);
    printf("dsp_hw_ms        (calculated @ %u MHz): %.3f ms\n", (unsigned)logic_freq_mhz, dsp_hw_ms);
    printf("linux_compute_ms (enable->done)        : %.3f ms\n", linux_compute_ms);
    printf("linux_e2e_ms     (staging->done)       : %.3f ms\n", linux_e2e_ms);
    printf("=============================================================\n\n");
   
    return (mismatch_count ? EXIT_FAILURE :  EXIT_SUCCESS);

}
