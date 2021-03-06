/* ************************************************************************
 * Copyright 2019-2020 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#include "rocauxiliary_larf.hpp"

template <typename T>
rocblas_status rocsolver_larf_impl(rocblas_handle handle, const rocblas_side side, const rocblas_int m, 
                                   const rocblas_int n, T* x, const rocblas_int incx, const T* alpha,
                                   T* A, const rocblas_int lda)
{
    if(!handle)
        return rocblas_status_invalid_handle;

    // logging is missing ???

    // argument checking
    if (n < 0 || m < 0 || lda < m || !incx)
        return rocblas_status_invalid_size;
    if (!x || !A || !alpha)
        return rocblas_status_invalid_pointer;

    rocblas_stride stridex = 0;
    rocblas_stride stridea = 0;
    rocblas_stride stridep = 0;
    rocblas_int batch_count=1;
    
    // memory managment
    size_t size_1;  //size of constants
    size_t size_2;  //size of workspace
    size_t size_3;  //size of array of pointers to workspace
    rocsolver_larf_getMemorySize<T,false>(side,m,n,batch_count,&size_1,&size_2,&size_3);

    // (TODO) MEMORY SIZE QUERIES AND ALLOCATIONS TO BE DONE WITH ROCBLAS HANDLE    
    void *scalars, *work, *workArr;
    hipMalloc(&scalars,size_1);
    hipMalloc(&work,size_2);
    hipMalloc(&workArr,size_3);
    if (!scalars || (size_2 && !work) || (size_3 && !workArr))
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
           rocsolver_larf_template<T>(handle,side, 
                                      m,n,
                                      x,0,    //vector shifted 0 entries
                                      incx,
                                      stridex,
                                      alpha,
                                      stridep,
                                      A,0,       //matrix shifted 0 entries
                                      lda,
                                      stridea, 
                                      batch_count,
                                      (T*)scalars,
                                      (T*)work,
                                      (T**)workArr);

    hipFree(scalars);
    hipFree(work);
    hipFree(workArr);
    return status;
}


/*
 * ===========================================================================
 *    C wrapper
 * ===========================================================================
 */

extern "C" {

ROCSOLVER_EXPORT rocblas_status rocsolver_slarf(rocblas_handle handle, const rocblas_side side, const rocblas_int m, 
                                                const rocblas_int n, float* x, const rocblas_int incx, const float* alpha,
                                                float* A, const rocblas_int lda)
{
    return rocsolver_larf_impl<float>(handle, side, m, n, x, incx, alpha, A, lda);
}

ROCSOLVER_EXPORT rocblas_status rocsolver_dlarf(rocblas_handle handle, const rocblas_side side, const rocblas_int m, 
                                                const rocblas_int n, double* x, const rocblas_int incx, const double* alpha,
                                                double* A, const rocblas_int lda)
{
    return rocsolver_larf_impl<double>(handle, side, m, n, x, incx, alpha, A, lda);
}

ROCSOLVER_EXPORT rocblas_status rocsolver_clarf(rocblas_handle handle, const rocblas_side side, const rocblas_int m, 
                                                const rocblas_int n, rocblas_float_complex* x, const rocblas_int incx,
                                                const rocblas_float_complex* alpha, rocblas_float_complex* A, const rocblas_int lda)
{
    return rocsolver_larf_impl<rocblas_float_complex>(handle, side, m, n, x, incx, alpha, A, lda);
}

ROCSOLVER_EXPORT rocblas_status rocsolver_zlarf(rocblas_handle handle, const rocblas_side side, const rocblas_int m, 
                                                const rocblas_int n, rocblas_double_complex* x, const rocblas_int incx,
                                                const rocblas_double_complex* alpha, rocblas_double_complex* A, const rocblas_int lda)
{
    return rocsolver_larf_impl<rocblas_double_complex>(handle, side, m, n, x, incx, alpha, A, lda);
}

} //extern C

