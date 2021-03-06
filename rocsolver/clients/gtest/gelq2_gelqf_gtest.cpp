/* ************************************************************************
 * Copyright 2018 Advanced Micro Devices, Inc.
 *
 * ************************************************************************ */

#include "testing_gelq2_gelqf.hpp"
#include "utility.h"
#include <gtest/gtest.h>
#include <math.h>
#include <stdexcept>
#include <vector>

using ::testing::Combine;
using ::testing::TestWithParam;
using ::testing::Values;
using ::testing::ValuesIn;
using namespace std;


typedef std::tuple<vector<int>, int> gelq_tuple;

// vector of vector, each vector is a {M, lda};
const vector<vector<int>> matrix_size_range = {
    {0, 1}, {-1, 1}, {20, 5}, {50, 50}, {70, 100}, {130, 130}, {150, 200}
};

// each is a N
const vector<int> n_size_range = {
    -1, 0, 16, 20, 130, 150
};

const vector<vector<int>> large_matrix_size_range = {
    {152, 152}, {640, 640}, {1000, 1024}, 
};

const vector<int> large_n_size_range = {
    64, 98, 130, 220, 400
};


Arguments setup_arguments_lq(gelq_tuple tup) 
{
    vector<int> matrix_size = std::get<0>(tup);
    int n_size = std::get<1>(tup);

    Arguments arg;

    arg.M = matrix_size[0];
    arg.N = n_size;
    arg.lda = matrix_size[1];

    arg.timing = 0;

    return arg;
}

class LQfact : public ::TestWithParam<gelq_tuple> {
protected:
    LQfact() {}
    virtual ~LQfact() {}
    virtual void SetUp() {}
    virtual void TearDown() {}
};

TEST_P(LQfact, gelq2_float) {
    Arguments arg = setup_arguments_lq(GetParam());

    rocblas_status status = testing_gelq2_gelqf<float,float,0>(arg);

    // if not success, then the input argument is problematic, so detect the error
    // message
    if (status != rocblas_status_success) {
        if (arg.M < 0 || arg.N < 0) {
            EXPECT_EQ(rocblas_status_invalid_size, status);
        } else if (arg.lda < arg.M) {
            EXPECT_EQ(rocblas_status_invalid_size, status);
        }
    }
}

TEST_P(LQfact, gelq2_double) {
    Arguments arg = setup_arguments_lq(GetParam());

    rocblas_status status = testing_gelq2_gelqf<double,double,0>(arg);

    // if not success, then the input argument is problematic, so detect the error
    // message
    if (status != rocblas_status_success) {
        if (arg.M < 0 || arg.N < 0) {
            EXPECT_EQ(rocblas_status_invalid_size, status);
        } else if (arg.lda < arg.M) {
            EXPECT_EQ(rocblas_status_invalid_size, status);
        }
    }
}

TEST_P(LQfact, gelq2_float_complex) {
    Arguments arg = setup_arguments_lq(GetParam());

    rocblas_status status = testing_gelq2_gelqf<rocblas_float_complex,float,0>(arg);

    // if not success, then the input argument is problematic, so detect the error
    // message
    if (status != rocblas_status_success) {
        if (arg.M < 0 || arg.N < 0) {
            EXPECT_EQ(rocblas_status_invalid_size, status);
        } else if (arg.lda < arg.M) {
            EXPECT_EQ(rocblas_status_invalid_size, status);
        }
    }
}

TEST_P(LQfact, gelq2_double_complex) {
    Arguments arg = setup_arguments_lq(GetParam());

    rocblas_status status = testing_gelq2_gelqf<rocblas_double_complex,double,0>(arg);

    // if not success, then the input argument is problematic, so detect the error
    // message
    if (status != rocblas_status_success) {
        if (arg.M < 0 || arg.N < 0) {
            EXPECT_EQ(rocblas_status_invalid_size, status);
        } else if (arg.lda < arg.M) {
            EXPECT_EQ(rocblas_status_invalid_size, status);
        }
    }
}

TEST_P(LQfact, gelqf_float) {
    Arguments arg = setup_arguments_lq(GetParam());

    rocblas_status status = testing_gelq2_gelqf<float,float,1>(arg);

    // if not success, then the input argument is problematic, so detect the error
    // message
    if (status != rocblas_status_success) {
        if (arg.M < 0 || arg.N < 0) {
            EXPECT_EQ(rocblas_status_invalid_size, status);
        } else if (arg.lda < arg.M) {
            EXPECT_EQ(rocblas_status_invalid_size, status);
        }
    }
}

TEST_P(LQfact, gelqf_double) {
    Arguments arg = setup_arguments_lq(GetParam());

    rocblas_status status = testing_gelq2_gelqf<double,double,1>(arg);

    // if not success, then the input argument is problematic, so detect the error
    // message
    if (status != rocblas_status_success) {
        if (arg.M < 0 || arg.N < 0) {
            EXPECT_EQ(rocblas_status_invalid_size, status);
        } else if (arg.lda < arg.M) {
            EXPECT_EQ(rocblas_status_invalid_size, status);
        }
    }
}

TEST_P(LQfact, gelqf_float_complex) {
    Arguments arg = setup_arguments_lq(GetParam());

    rocblas_status status = testing_gelq2_gelqf<rocblas_float_complex,float,1>(arg);

    // if not success, then the input argument is problematic, so detect the error
    // message
    if (status != rocblas_status_success) {
        if (arg.M < 0 || arg.N < 0) {
            EXPECT_EQ(rocblas_status_invalid_size, status);
        } else if (arg.lda < arg.M) {
            EXPECT_EQ(rocblas_status_invalid_size, status);
        }
    }
}

TEST_P(LQfact, gelqf_double_complex) {
    Arguments arg = setup_arguments_lq(GetParam());

    rocblas_status status = testing_gelq2_gelqf<rocblas_double_complex,double,1>(arg);

    // if not success, then the input argument is problematic, so detect the error
    // message
    if (status != rocblas_status_success) {
        if (arg.M < 0 || arg.N < 0) {
            EXPECT_EQ(rocblas_status_invalid_size, status);
        } else if (arg.lda < arg.M) {
            EXPECT_EQ(rocblas_status_invalid_size, status);
        }
    }
}


INSTANTIATE_TEST_CASE_P(daily_lapack, LQfact,
                        Combine(ValuesIn(large_matrix_size_range),
                                ValuesIn(large_n_size_range)));

INSTANTIATE_TEST_CASE_P(checkin_lapack, LQfact,
                        Combine(ValuesIn(matrix_size_range),
                                ValuesIn(n_size_range)));
