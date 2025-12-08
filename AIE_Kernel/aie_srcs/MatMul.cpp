/*
Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
SPDX-License-Identifier: MIT
*/

#include "aie_kernel.h"
#include <adf.h>
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "aie_api/utils.hpp"

void MatMul( adf::input_buffer<int32> & __restrict in1, adf::input_buffer<int32> & __restrict in2,
             adf::output_buffer<int32> & __restrict out_put)
{
        using MMUL = aie::mmul <MAT_M, MAT_K, MAT_N, int32,int32> ;
        const int32* __restrict  pA=(int32*) in1.data();
        const int32* __restrict pB=(int32*) in2.data();
        int32* __restrict pC =out_put.data();

        aie::vector<int32,MMUL::size_A> A0 = aie::load_v<MMUL::size_A>(pA);
        aie::vector<int32,MMUL::size_B> B0 = aie::load_v<MMUL::size_B>(pB);

        aie::print(A0, true, "A0_matrix: ");
        aie::print(B0, true, "B0_matrix: ");

        //Defining output 
        MMUL C00;
        C00.mul(A0,B0);
        aie::print(C00.template to_vector<int32>(), true, "C00_matrix: ");

        aie::store_v(pC, C00.template to_vector<int32>());
}

