#!/usr/bin/env python3
import os
import numpy as np

# directory where THIS script lives
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

def generate_headers(shape, seed=1):
    N, M, L = shape
    np.random.seed(seed)

    folder = os.path.join(BASE_DIR, f"{N}x{M}x{L}")
    os.makedirs(folder, exist_ok=True)

    A = np.random.randint(0, 256, size=(N, M), dtype=np.uint16)
    B = np.random.randint(0, 256, size=(M, L), dtype=np.uint16)
    C = (A.astype(np.uint32) @ B.astype(np.uint32)).astype(np.uint32)

    def write_h(path, var_name, c_type, arr):
        r, c = arr.shape
        with open(path, "w") as f:
            f.write(f"{c_type} {var_name}[{r}][{c}] = {{\n")
            for i in range(r):
                row = ", ".join(str(arr[i, j]) for j in range(c))
                f.write(f"  {{ {row} }},\n")
            f.write("};\n")

    write_h(os.path.join(folder, "matrix_A_data.h"), "matrix_A_data", "uint16_t", A)
    write_h(os.path.join(folder, "matrix_B_data.h"), "matrix_B_data", "uint16_t", B)
    write_h(os.path.join(folder, "output_data.h"),   "output_data",   "uint32_t", C)

    print("Generated headers in", folder)

def generate_multiple_headers(shapes, seed=1):
    for shape in shapes:
        generate_headers(shape, seed=seed)

if __name__ == "__main__":
    generate_multiple_headers([[32, 32, 32], [64, 64, 64], [128, 128, 128], [256, 256, 256], [512, 512, 512], [1024, 1024, 1024]])