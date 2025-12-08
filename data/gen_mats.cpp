#include <cstdint>
#include <vector>
#include <random>
#include <fstream>
#include <string>
#include <iostream>
#include "../include/matmul_config.h"

// Fallback defaults
#ifndef MAT_SEED
#define MAT_SEED 1234
#endif

#ifndef MAT_MIN_VAL
#define MAT_MIN_VAL -4
#endif

#ifndef MAT_MAX_VAL
#define MAT_MAX_VAL 4
#endif

// Helper: write a matrix to a .txt file, one element per line:
template <typename T>
void write_txt(const std::string &filename, const std::vector<T> &data) {
    std::ofstream ofs(filename);
    if (!ofs) {
        std::cerr << "ERROR: cannot open " << filename << " for writing\n";
        return;
    }

    for (std::size_t i = 0; i < data.size(); ++i) {
        ofs << data[i] << "\n";
    }
}

// Helper: write a C header:
template <typename T>
void write_header(const std::string &filename,
                  const std::string &array_name,
                  const std::string &ctype,
                  const std::vector<T> &data)
{
    std::ofstream ofs(filename);
    if (!ofs) {
        std::cerr << "ERROR: cannot open " << filename << " for writing\n";
        return;
    }

    ofs << "#pragma once\n";
    ofs << "#include <cstdint>\n\n";
    ofs << ctype << " " << array_name << "[" << data.size() << "] = {\n";

    for (std::size_t i = 0; i < data.size(); ++i) {
        ofs << "\t" << static_cast<long long>(data[i]); // print as number
        if (i + 1 < data.size()) {
            ofs << ",";
        }
        ofs << "\n";
    }
    ofs << "};\n";
}

int main() {
    // Dimensions from matmul_config.h:
    // A: MAT_M x MAT_K
    // B: MAT_K x MAT_N
    // C: MAT_M x MAT_N
    const std::size_t sizeA = static_cast<std::size_t>(MAT_M) * MAT_K;
    const std::size_t sizeB = static_cast<std::size_t>(MAT_K) * MAT_N;
    const std::size_t sizeC = static_cast<std::size_t>(MAT_M) * MAT_N;

    // Use 32-bit ints for inputs, 64-bit for accumulation/golden
    using in_t  = int32_t;
    using acc_t = int64_t;

    std::vector<in_t>  A(sizeA);
    std::vector<in_t>  B(sizeB);
    std::vector<acc_t> C(sizeC);

    // Pseudo-random generation
    std::mt19937 rng(MAT_SEED);
    std::uniform_int_distribution<int> dist(MAT_MIN_VAL, MAT_MAX_VAL);

    for (auto &x : A) x = static_cast<in_t>(dist(rng));
    for (auto &x : B) x = static_cast<in_t>(dist(rng));

    // Golden C = A * B (row-major)
    for (int m = 0; m < MAT_M; ++m) {
        for (int n = 0; n < MAT_N; ++n) {
            acc_t acc = 0;
            for (int k = 0; k < MAT_K; ++k) {
                acc += static_cast<acc_t>(
                           A[m * MAT_K + k]
                       ) * static_cast<acc_t>(
                           B[k * MAT_N + n]
                       );
            }
            C[m * MAT_N + n] = acc;
        }
    }

    // ---- Write .txt files (canonical) ----
    write_txt("MatrixA.txt", A);
    write_txt("MatrixB.txt", B);
    write_txt("GoldenC.txt", C);

    // ---- Write .h files derived from the .txt data ----
    write_header("inputA_data.h", "inputA_data", "int32_t", A);
    write_header("inputB_data.h", "inputB_data", "int32_t", B);
    write_header("goldenC_data.h", "goldenC_data", "int64_t", C);

    std::cout << "Generated matrices and headers for "
              << "MAT_M=" << MAT_M
              << ", MAT_K=" << MAT_K
              << ", MAT_N=" << MAT_N << "\n";

    return 0;
}
