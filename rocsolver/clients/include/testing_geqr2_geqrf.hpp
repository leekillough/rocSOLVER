/* ************************************************************************
 * Copyright 2018 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#include <cmath> // std::abs
#include <fstream>
#include <iostream>
#include <limits> // std::numeric_limits<T>::epsilon();
#include <stdlib.h>
#include <string>
#include <vector>

#include "arg_check.h"
#include "cblas_interface.h"
#include "norm.h"
#include "rocblas_test_unique_ptr.hpp"
#include "rocsolver.hpp"
#include "unit.h"
#include "utility.h"
#ifdef GOOGLE_TEST
#include <gtest/gtest.h>
#endif

// this is max error PER element
#define ERROR_EPS_MULTIPLIER 3000
// AS IN THE ORIGINAL ROCSOLVER TEST UNITS, WE CURRENTLY USE A HIGH TOLERANCE 
// AND THE MAX NORM TO EVALUATE THE ERROR. THIS IS NOT "NUMERICALLY SOUND"; 
// A MAJOR REFACTORING OF ALL UNIT TESTS WILL BE REQUIRED.  

using namespace std;

template <typename T, typename U, int geqrf> 
rocblas_status testing_geqr2_geqrf(Arguments argus) {
    rocblas_int M = argus.M;
    rocblas_int N = argus.N;
    rocblas_int lda = argus.lda;
    int hot_calls = argus.iters;

    std::unique_ptr<rocblas_test::handle_struct> unique_ptr_handle(new rocblas_test::handle_struct);
    rocblas_handle handle = unique_ptr_handle->handle;

    // check invalid size and quick return
    if (M < 1 || N < 1 || lda < M) {
        auto dA_managed = rocblas_unique_ptr{rocblas_test::device_malloc(sizeof(T)), rocblas_test::device_free};
        T *dA = (T *)dA_managed.get();

        auto dIpiv_managed = rocblas_unique_ptr{rocblas_test::device_malloc(sizeof(T)), rocblas_test::device_free};
        T *dIpiv = (T *)dIpiv_managed.get();

        if (!dA || !dIpiv) {
            PRINT_IF_HIP_ERROR(hipErrorOutOfMemory);
            return rocblas_status_memory_error;
        }
        
        if(geqrf) {
            return rocsolver_geqrf<T>(handle, M, N, dA, lda, dIpiv);
        }
        else { 
            return rocsolver_geqr2<T>(handle, M, N, dA, lda, dIpiv);
        }
    }

    rocblas_int size_A = lda * N;
    rocblas_int size_piv = min(M, N);    

    // Naming: dK is in GPU (device) memory. hK is in CPU (host) memory
    vector<T> hA(size_A);
    vector<T> hAr(size_A);
    vector<T> hw(N);
    vector<T> hIpiv(size_piv);
    vector<T> hIpivr(size_piv);

    auto dA_managed = rocblas_unique_ptr{rocblas_test::device_malloc(sizeof(T) * size_A), rocblas_test::device_free};
    T *dA = (T *)dA_managed.get();
    auto dIpiv_managed = rocblas_unique_ptr{rocblas_test::device_malloc(sizeof(T) * size_piv), rocblas_test::device_free};
    T *dIpiv = (T *)dIpiv_managed.get();
  
    if ((size_A > 0 && !dA) || (size_piv > 0 && !dIpiv)) {
        PRINT_IF_HIP_ERROR(hipErrorOutOfMemory);
        return rocblas_status_memory_error;
    }

    //initialize full random matrix hA with all entries in [1, 10]
    rocblas_init<T>(hA.data(), M, N, lda);
    for (rocblas_int i = 0; i < M; ++i) {
        for (rocblas_int j = 0; j < N; ++j) {
            if (i == j)
                hA[i+j*lda] += 400;
            else
                hA[i+j*lda] -= 4;
        }
    }


    // copy data from CPU to device
    CHECK_HIP_ERROR(hipMemcpy(dA, hA.data(), sizeof(T) * size_A, hipMemcpyHostToDevice));

    double gpu_time_used, cpu_time_used;
    double error_eps_multiplier = ERROR_EPS_MULTIPLIER;
    double eps = std::numeric_limits<U>::epsilon();
    double max_err_1 = 0.0, max_val = 0.0;
    double diff;

/* =====================================================================
           ROCSOLVER
    =================================================================== */  
    if (argus.unit_check || argus.norm_check) {
        //GPU lapack
        if(geqrf) {
            CHECK_ROCBLAS_ERROR(rocsolver_geqrf<T>(handle, M, N, dA, lda, dIpiv));
        }
        else {
            CHECK_ROCBLAS_ERROR(rocsolver_geqr2<T>(handle, M, N, dA, lda, dIpiv));
        }   
        //copy output from device to cpu
        CHECK_HIP_ERROR(hipMemcpy(hAr.data(), dA, sizeof(T) * size_A, hipMemcpyDeviceToHost));
        CHECK_HIP_ERROR(hipMemcpy(hIpivr.data(), dIpiv, sizeof(T) * size_piv, hipMemcpyDeviceToHost));

        //CPU lapack
        cpu_time_used = get_time_us();
        if(geqrf) {
            cblas_geqrf<T>(M, N, hA.data(), lda, hIpiv.data(), hw.data(), N);
        }
        else {
            cblas_geqr2<T>(M, N, hA.data(), lda, hIpiv.data(), hw.data());
        }
        cpu_time_used = get_time_us() - cpu_time_used;

        // +++++++++ Error Check +++++++++++++
        // check if the pivoting returned is identical
        for (int j = 0; j < size_piv; j++) {
            diff = abs(hIpiv[j]);
            max_val = max_val > diff ? max_val : diff;
            diff = abs(hIpivr[j] - hIpiv[j]);
            max_err_1 = max_err_1 > diff ? max_err_1 : diff;
        }
        // hAr contains calculated decomposition, so error is hA - hAr
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                diff = abs(hA[i + j * lda]);
                max_val = max_val > diff ? max_val : diff;
                diff = abs(hAr[i + j * lda] - hA[i + j * lda]);
                max_err_1 = max_err_1 > diff ? max_err_1 : diff;
            }
        }
        max_err_1 = max_err_1 / max_val;

        if(argus.unit_check)
            getf2_err_res_check<U>(max_err_1, M, N, error_eps_multiplier, eps);
    }
 
    if (argus.timing) {
        // GPU rocBLAS
        int cold_calls = 2;

        if(geqrf) {
            for(int iter = 0; iter < cold_calls; iter++)
                rocsolver_geqrf<T>(handle, M, N, dA, lda, dIpiv);
            gpu_time_used = get_time_us();
            for(int iter = 0; iter < hot_calls; iter++)
                rocsolver_geqrf<T>(handle, M, N, dA, lda, dIpiv);
            gpu_time_used = (get_time_us() - gpu_time_used) / hot_calls;       
        }
        else {
            for(int iter = 0; iter < cold_calls; iter++)
                rocsolver_geqr2<T>(handle, M, N, dA, lda, dIpiv);
            gpu_time_used = get_time_us();
            for(int iter = 0; iter < hot_calls; iter++)
                rocsolver_geqr2<T>(handle, M, N, dA, lda, dIpiv);
            gpu_time_used = (get_time_us() - gpu_time_used) / hot_calls;       
        }

        // only norm_check return an norm error, unit check won't return anything
        cout << "M,N,lda,gpu_time(us),cpu_time(us)";

        if (argus.norm_check)
            cout << ",norm_error_host_ptr";

        cout << endl;
        cout << M << "," << N << "," << lda << "," << gpu_time_used << ","<< cpu_time_used;

        if (argus.norm_check)
            cout << "," << max_err_1;

        cout << endl;
    }
  
    return rocblas_status_success;
}

#undef ERROR_EPS_MULTIPLIER
