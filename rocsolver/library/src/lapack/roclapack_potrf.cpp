/* ************************************************************************
 * Copyright 2019-2020 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#include "roclapack_potrf.hpp"

template <typename S, typename T, typename U>
rocblas_status rocsolver_potrf_impl(rocblas_handle handle, const rocblas_fill uplo,    
                                    const rocblas_int n, U A, const rocblas_int lda, rocblas_int* info) 
{ 
    if(!handle)
        return rocblas_status_invalid_handle;
    
    //logging is missing ???    
    
    // argument checking
    if (!A || !info)
        return rocblas_status_invalid_pointer;
    if (n < 0 || lda < n)
        return rocblas_status_invalid_size;

    rocblas_stride strideA = 0;
    rocblas_int batch_count = 1;

    // memory managment
    size_t size_1;  //size of constants
    size_t size_2;  //size of workspace
    size_t size_3;  
    size_t size_4;  
    rocsolver_potrf_getMemorySize<T>(n,batch_count,&size_1,&size_2,&size_3,&size_4);

    // (TODO) MEMORY SIZE QUERIES AND ALLOCATIONS TO BE DONE WITH ROCBLAS HANDLE
    void *scalars, *work, *pivotGPU, *iinfo;
    hipMalloc(&scalars,size_1);
    hipMalloc(&work,size_2);
    hipMalloc(&pivotGPU,size_3);
    hipMalloc(&iinfo,size_4);
    if (!scalars || (size_2 && !work) || (size_3 && !pivotGPU) || (size_4 && !iinfo))
        return rocblas_status_memory_error;

    // scalars constants for rocblas functions calls
    // (to standarize and enable re-use, size_1 always equals 3)
    std::vector<T> sca(size_1);
    sca[0] = -1;
    sca[1] = 0;
    sca[2] = 1;
    RETURN_IF_HIP_ERROR(hipMemcpy(scalars, sca.data(), sizeof(T)*size_1, hipMemcpyHostToDevice));

    // execution
    rocblas_status status =
           rocsolver_potrf_template<S,T>(handle,uplo,n,
                                         A,0,    //the matrix is shifted 0 entries (will work on the entire matrix)
                                         lda,strideA,
                                         info,batch_count,
                                         (T*)scalars,
                                         (T*)work,
                                         (T*)pivotGPU,
                                         (rocblas_int*)iinfo);

    hipFree(scalars);
    hipFree(work);
    hipFree(pivotGPU);
    hipFree(iinfo);
    return status;
}




/*
 * ===========================================================================
 *    C wrapper
 * ===========================================================================
 */

extern "C" {

ROCSOLVER_EXPORT rocblas_status rocsolver_spotrf(rocblas_handle handle, const rocblas_fill uplo, const rocblas_int n,
                 float *A, const rocblas_int lda, rocblas_int* info)
{
    return rocsolver_potrf_impl<float,float>(handle, uplo, n, A, lda, info);
}

ROCSOLVER_EXPORT rocblas_status rocsolver_dpotrf(rocblas_handle handle, const rocblas_fill uplo, const rocblas_int n,
                 double *A, const rocblas_int lda, rocblas_int* info)
{
    return rocsolver_potrf_impl<double,double>(handle, uplo, n, A, lda, info);
}

ROCSOLVER_EXPORT rocblas_status rocsolver_cpotrf(rocblas_handle handle, const rocblas_fill uplo, const rocblas_int n,
                 rocblas_float_complex *A, const rocblas_int lda, rocblas_int* info)
{
    return rocsolver_potrf_impl<float,rocblas_float_complex>(handle, uplo, n, A, lda, info);
}

ROCSOLVER_EXPORT rocblas_status rocsolver_zpotrf(rocblas_handle handle, const rocblas_fill uplo, const rocblas_int n,
                 rocblas_double_complex *A, const rocblas_int lda, rocblas_int* info)
{
    return rocsolver_potrf_impl<double,rocblas_double_complex>(handle, uplo, n, A, lda, info);
}

}
