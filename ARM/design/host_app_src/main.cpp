#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <iostream>

// Matricies
#include "matrix_A_data.h"
#include "matrix_B_data.h"
#include "output_data.h"

static void* aligned_alloc_or_die(size_t alignment, size_t bytes) {
  void* p = nullptr;
  if (posix_memalign(&p, alignment, bytes) != 0 || !p) std::abort();
  return p;
}

static inline uint64_t us_now() {
  using clock = std::chrono::steady_clock;
  return (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(
             clock::now().time_since_epoch())
      .count();
}

static inline double us_to_ms(uint64_t us) {
  return (double)us / 1000.0;
}

// Treat A, B as NxN uint16; C as NxN uint32
static void gemm_u16_u32(const uint16_t* __restrict A,
                         const uint16_t* __restrict B,
                         uint32_t* __restrict C,
                         int N) {
  std::memset(C, 0, sizeof(uint32_t) * (size_t)N * (size_t)N);

  // Loop order i-k-j generally better for cache than i-j-k
  for (int i = 0; i < N; ++i) {
    const uint16_t* Ai = A + (size_t)i * N;
    uint32_t* Ci = C + (size_t)i * N;

    for (int k = 0; k < N; ++k) {
      const uint32_t a = (uint32_t)Ai[k];
      const uint16_t* Bk = B + (size_t)k * N;

      for (int j = 0; j < N; ++j) {
        Ci[j] += a * (uint32_t)Bk[j];
      }
    }
  }
}

// Error check
static uint32_t mismatch_count_u32(const uint32_t* C,
                                  const uint32_t* C_gold,
                                  size_t elems) {
  uint32_t mism = 0;
  for (size_t i = 0; i < elems; ++i) {
    if (C[i] != C_gold[i]) ++mism;
  }
  return mism;
}

int main(int argc, char** argv) {
  const int N = GEMM_SIZE;
  int iters = 1;
  if (argc >= 2) iters = std::atoi(argv[1]);
  if (iters < 1) iters = 1;

  // Working buffers (flattened)
  const size_t elems = (size_t)N * (size_t)N;
  const size_t bytesA = sizeof(uint16_t) * elems;
  const size_t bytesB = sizeof(uint16_t) * elems;
  const size_t bytesC = sizeof(uint32_t) * elems;

  uint16_t* A = (uint16_t*)aligned_alloc_or_die(64, bytesA);
  uint16_t* B = (uint16_t*)aligned_alloc_or_die(64, bytesB);
  uint32_t* C = (uint32_t*)aligned_alloc_or_die(64, bytesC);

  // Source pointers to the *contiguous* 2D arrays from your headers
  const uint16_t* A_src = &matrix_A_data[0][0];
  const uint16_t* B_src = &matrix_B_data[0][0];
  const uint32_t* C_gold = &output_data[0][0];

  // Warm-up (not timed)
  std::memcpy(A, A_src, bytesA);
  std::memcpy(B, B_src, bytesB);
  gemm_u16_u32(A, B, C, N);

  // ---- Timers (accumulate over iters) ----
  uint64_t arm_hw_us_sum = 0;        // GEMM-only
  uint64_t linux_compute_us_sum = 0; // memcpy(A,B) + GEMM
  uint64_t linux_e2e_us = 0;         // whole loop region

  auto t_linux_e2e_start = us_now();

  for (int r = 0; r < iters; ++r) {
    // linux_compute: memcpy in + compute
    auto t_linux_compute_start = us_now();
    std::memcpy(A, A_src, bytesA);
    std::memcpy(B, B_src, bytesB);

    // arm_hw: GEMM-only
    auto t_hw_compute_start = us_now();
    gemm_u16_u32(A, B, C, N);
    auto t_compute_end = us_now();

    arm_hw_us_sum += (t_compute_end - t_hw_compute_start);
    linux_compute_us_sum += (t_compute_end - t_linux_compute_start);
  }

  auto t_linux_e2e_end = us_now();
  linux_e2e_us = (t_linux_e2e_end - t_linux_e2e_start);

  // Correctness (outside timing, like your other builds)
  uint32_t mism = mismatch_count_u32(C, C_gold, elems);
  std::cout << "TEST " << (mism ? "FAILED" : "PASSED") << std::endl;

  // Report
  const double arm_hw_ms        = us_to_ms(arm_hw_us_sum);
  const double linux_compute_ms = us_to_ms(linux_compute_us_sum);
  const double linux_e2e_ms     = us_to_ms(linux_e2e_us);

  // Throughput based on GEMM-only time
  const double ops_per = 2.0 * (double)N * (double)N * (double)N;
  const double total_ops = ops_per * (double)iters;
  const double gops_s_hw =
      (total_ops / 1e9) / (arm_hw_ms / 1e3);

  printf("\n================ ARM GEMM TIMING (Linux POV) ================\n");
  printf("arm_hw_ms        (gemm-only sum)      : %.3f ms\n", arm_hw_ms);
  printf("linux_compute_ms (memcpy+gemm sum)    : %.3f ms\n", linux_compute_ms);
  printf("linux_e2e_ms     (loop e2e)           : %.3f ms\n", linux_e2e_ms);
  printf("hw_throughput    (gemm-only)          : %.3f GOPS\n", gops_s_hw);
  printf("mismatch_count   (u32 exact)          : %u\n", (unsigned)mism);
  printf("=============================================================\n\n");


  // touch output so compiler can’t get clever
  volatile uint32_t sink = C[(size_t)(N/2) * (size_t)N + (size_t)(N/2)];
  (void)sink;

  std::free(A);
  std::free(B);
  std::free(C);
  return 0;
}