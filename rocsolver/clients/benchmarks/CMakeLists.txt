# ########################################################################
# Copyright 2016 Advanced Micro Devices, Inc.
# ########################################################################

# set( Boost_DEBUG ON )
set( Boost_USE_MULTITHREADED ON )
set( Boost_DETAILED_FAILURE_MSG ON )
set( Boost_ADDITIONAL_VERSIONS 1.65.1 1.65 )
set( Boost_USE_STATIC_LIBS OFF )

if(EXISTS /etc/redhat-release)
    if(CXX_VERSION_STRING MATCHES "clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp=libgomp -pthread -stdlib=libstdc++ -std=c++14")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp=libgomp -pthread")
    endif()
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp -pthread")
endif()
    
find_package( Boost COMPONENTS program_options )

if( NOT Boost_FOUND )
  message( STATUS "Dynamic boost libraries not found. Attempting to find static libraries " )
  set( Boost_USE_STATIC_LIBS ON )
  find_package( Boost COMPONENTS program_options )

  if( NOT Boost_FOUND )
    message( FATAL_ERROR "boost is a required dependency and is not found;  try adding boost path to CMAKE_PREFIX_PATH" )
  endif( )
endif( )

set( THREADS_PREFER_PTHREAD_FLAG ON )
find_package( Threads REQUIRED )

# Linking lapack library requires fortran flags
enable_language( Fortran )
find_package( cblas CONFIG REQUIRED )
if( NOT cblas_FOUND )
  message( FATAL_ERROR "cblas is a required dependency and is not found;  try adding cblas path to CMAKE_PREFIX_PATH" )
endif( )

set( rocsolver_benchmark_common
      ../common/utility.cpp
      ../common/cblas_interface.cpp
      ../common/norm.cpp
      ../common/unit.cpp
      ../common/near.cpp
      ../common/arg_check.cpp
    )

add_executable( rocsolver-bench client.cpp ${rocsolver_benchmark_common} )
target_compile_features( rocsolver-bench PRIVATE cxx_static_assert cxx_nullptr cxx_auto_type )


#if( BUILD_WITH_TENSILE )
#    target_compile_definitions( rocsolver-bench PRIVATE BUILD_WITH_TENSILE=1 )
#else()
#    target_compile_definitions( rocsolver-bench PRIVATE BUILD_WITH_TENSILE=0 )
#endif()

# Internal header includes
target_include_directories( rocsolver-bench
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../library/include>
)

#set( BLIS_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/build/deps/blis/include/blis )
#set( BLIS_LIBRARY ${CMAKE_SOURCE_DIR}/build/deps/blis/lib/libblis.so )

if( OS_ID_rhel OR OS_ID_centos OR OS_ID_sles)
    if( OS_ID_rhel OR OS_ID_centos)
        if( EXISTS "/usr/lib/gcc/x86_64-redhat-linux/8/" )
            set( ROCM_OPENMP_PATH /usr/lib/gcc/x86_64-redhat-linux/8 )
        else()
            set( ROCM_OPENMP_PATH /opt/rh/devtoolset-7/root/usr/lib/gcc/x86_64-redhat-linux/7 )
        endif()

        # defer OpenMP include as search order must come after clang
        set( XXX_OPENMP_INCLUDE_DIR ${ROCM_OPENMP_PATH}/include )
        set( OPENMP_LIBRARY ${ROCM_OPENMP_PATH}/libgomp.so )
        if( CXX_VERSION_STRING MATCHES "clang")
            set( XXX_OPENMP_INCLUDE_DIR ${ROCM_OPENMP_PATH}/include )
        endif()
    else()
    #SLES
        set( OPENMP_INCLUDE_DIR /usr/lib64/gcc/x86_64-suse-linux/7/include/ )
        set( OPENMP_LIBRARY /usr/lib64/gcc/x86_64-suse-linux/7/libgomp.so )
    endif()

    message(STATUS "RocmPath: ${ROCM_PATH}")
    if(EXISTS ${ROCM_PATH}/llvm/lib/clang/11.0.0/include/immintrin.h AND CMAKE_CXX_COMPILER MATCHES ".*/hcc$|.*/hipcc$" )
        set( CLANG_INCLUDE_DIR ${ROCM_PATH}/llvm/lib/clang/11.0.0/include )
    elseif( EXISTS "${ROCM_PATH}/hcc/lib/clang/10.0.0/include/immintrin.h" AND CMAKE_CXX_COMPILER MATCHES ".*/hcc$|.*/hipcc$" )
        set( CLANG_INCLUDE_DIR ${ROCM_PATH}/hcc/lib/clang/10.0.0/include )
    elseif( EXISTS "${ROCM_PATH}/hcc/lib/clang/9.0.0/include/immintrin.h" AND CMAKE_CXX_COMPILER MATCHES ".*/hcc$|.*/hipcc$" )
        set( CLANG_INCLUDE_DIR ${ROCM_PATH}/hcc/lib/clang/9.0.0/include )
    else()
        set( CLANG_INCLUDE_DIR )
    endif()

    # External header includes included as system files
    target_include_directories( rocsolver-bench
      SYSTEM PRIVATE
        $<BUILD_INTERFACE:${CLANG_INCLUDE_DIR}> 
#        $<BUILD_INTERFACE:${BLIS_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${HIP_INCLUDE_DIRS}>
        $<BUILD_INTERFACE:${HCC_INCLUDE_DIRS}>
        $<BUILD_INTERFACE:${Boost_INCLUDE_DIRS}>
        $<BUILD_INTERFACE:${CBLAS_INCLUDE_DIRS}>
        $<BUILD_INTERFACE:${OPENMP_INCLUDE_DIR}> 
        )

    target_link_libraries( rocsolver-bench PRIVATE ${Boost_LIBRARIES} ${OPENMP_LIBRARY} cblas lapack roc::rocsolver )

else()
    # External header includes included as system files
    target_include_directories( rocsolver-bench
      SYSTEM PRIVATE
        $<BUILD_INTERFACE:${HIP_INCLUDE_DIRS}>
        $<BUILD_INTERFACE:${HCC_INCLUDE_DIRS}>
        $<BUILD_INTERFACE:${Boost_INCLUDE_DIRS}>
        $<BUILD_INTERFACE:${CBLAS_INCLUDE_DIRS}>
#        $<BUILD_INTERFACE:${BLIS_INCLUDE_DIR}>
        )

    target_link_libraries( rocsolver-bench PRIVATE ${Boost_LIBRARIES} cblas lapack roc::rocsolver )
endif()

target_link_libraries( rocsolver-bench PRIVATE roc::rocblas ) #${ROCBLAS_LIBRARY})

if( CUDA_FOUND )
  target_include_directories( rocsolver-bench
    PRIVATE
      $<BUILD_INTERFACE:${CUDA_INCLUDE_DIRS}>
      $<BUILD_INTERFACE:${hip_INCLUDE_DIRS}>
    )
  target_compile_definitions( rocsolver-bench PRIVATE __HIP_PLATFORM_NVCC__ )
  target_link_libraries( rocsolver-bench PRIVATE ${CUDA_LIBRARIES} )
else( )
  target_compile_definitions( rocsolver-bench PRIVATE __HIP_PLATFORM_HCC__ )
  target_link_libraries( rocsolver-bench PRIVATE hip::device )
endif( )

if( CMAKE_CXX_COMPILER MATCHES ".*/hcc$" )
  # Remove following when hcc is fixed; hcc emits following spurious warning ROCm v1.6.1
  # "clang-5.0: warning: argument unused during compilation: '-isystem ${ROCM_PATH}/include'"
  target_compile_options( rocsolver-bench PRIVATE -Wno-unused-command-line-argument -mf16c )
  target_include_directories( rocsolver-bench PRIVATE ${ROCM_PATH}/hsa/include)
elseif( CMAKE_COMPILER_IS_GNUCXX OR CXX_VERSION_STRING MATCHES "clang")
  # GCC or hip-clang needs specific flags to turn on f16c intrinsics
  target_compile_options( rocsolver-bench PRIVATE -mf16c )
endif( )

if( CXX_VERSION_STRING MATCHES "clang" )
  target_link_libraries( rocsolver-bench PRIVATE -lpthread -lstdc++ -lgcc_s -lm -rtlib=compiler-rt )
  if(OS_ID_rhel OR OS_ID_centos)
    set(CMAKE_CXX_FLAGS "-isystem ${CLANG_INCLUDE_DIR} -isystem ${XXX_OPENMP_INCLUDE_DIR} ${CMAKE_CXX_FLAGS}")
  endif( )
else( )
  if(OS_ID_rhel OR OS_ID_centos)
    set(CMAKE_CXX_FLAGS "-isystem ${CLANG_INCLUDE_DIR} -isystem ${XXX_OPENMP_INCLUDE_DIR} ${CMAKE_CXX_FLAGS}")
  endif( )
  set(CMAKE_CXX_FLAGS "-isystem ${ROCM_PATH}/include ${CMAKE_CXX_FLAGS}")
endif( )

set_target_properties( rocsolver-bench PROPERTIES CXX_EXTENSIONS NO )
set_target_properties( rocsolver-bench PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/../../clients/staging" )
