#pragma once

// Logical matrix dimensions
constexpr int MAT_M = 2;   // rows of A and C
constexpr int MAT_K = 2;   // cols of A, rows of B
constexpr int MAT_N = 2;   // cols of B and C

// Data type
using mat_elem_t = int32_t;

// Random generation
constexpr unsigned MAT_SEED = 1234;
constexpr int MAT_MIN_VAL = -4;
constexpr int MAT_MAX_VAL = 4;
