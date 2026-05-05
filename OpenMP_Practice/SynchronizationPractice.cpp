#include <iostream>
#include <omp.h> // Open MP Library
#include <chrono> // For measuring time
#include <algorithm>
#include <random>
#include <iomanip> // for Setprecision
#include <assert.h>

#define NUM_THREADS 4
static long numSteps = 10000000;


double RandRange(double min, double max)
{
    double r = static_cast<double>(rand()) / RAND_MAX;
    return min + (max - min) * r;
}

void MonteCarlo()
{
    // Monte Carlo Pi Calculation
    long numTrials = 10000;
    long i = 0;
    long nCirc = 0; // Number of points that fall within the circle
    double pi = 0.0;
    double x = 0.0;
    double y = 0.0;
    double r = 1.0; // Radius of the circle. Side of square is 2*r
    std::srand(time(0));

#pragma omp parallel for private(x, y) reduction(+:nCirc) // Make a copy of it for each thread and merge it all togehter
    for (i = 0; i < numTrials; ++i)
    {
        x = RandRange(-r, r);
        y = RandRange(-r, r);
        // DistSq for 2D point
        if ((x * x + y * y) <= (r * r))
        {
            ++nCirc;
        }
    }
    pi = 4.0 * (static_cast<double>(nCirc) / static_cast<double>(numTrials));
    std::cout << std::setprecision(20) << "Pi: " << pi << "NumHits: " << nCirc << "\n";
}

void Example4_Sections()
{
    omp_set_num_threads(4);

#pragma omp parallel
    {
#pragma omp sections // adding 'nowait' here will allow the threads to continue without waiting for all sections to finish
        {
#pragma omp section
            {
                int id = omp_get_thread_num();
                for (int i = 0; i < 10; ++i)
                {
                    std::cout << "Section 1 for ID: " << id << "Index: " << i << "\n";
                }
            }
#pragma omp section
            {
                int id = omp_get_thread_num();
                for (int i = 0; i < 10; ++i)
                {
                    std::cout << "Section 2 for ID: " << id << "Index: " << i << "\n";
                }
            }
#pragma omp section
            {
                int id = omp_get_thread_num();
                for (int i = 0; i < 10; ++i)
                {
                    std::cout << "Section 3 for ID: " << id << "Index: " << i << "\n";
                }
            }
#pragma omp section
            {
                int id = omp_get_thread_num();
                for (int i = 0; i < 10; ++i)
                {
                    std::cout << "Section 4 for ID: " << id << "Index: " << i << "\n";
                }
            }
#pragma omp section
            {
                int id = omp_get_thread_num();
                for (int i = 0; i < 10; ++i)
                {
                    std::cout << "Section 5 for ID: " << id << "Index: " << i << "\n";
                }
            }
        }
    }

    std::cout << "All threads are done!\n";
}

struct MatrixOld
{
    float m[16];
    MatrixOld()
    {
        for (int i = 0; i < 16; ++i)
        {
            m[i] = 0;
            if (i == 0 || i == 5 || i == 10 || i == 15)
            {
                m[i] = 1;
            }
        }
    }
};
void MatrixMul(MatrixOld& a, MatrixOld& b, MatrixOld& result) // Old way of doing the matrix mult 
{
    // 4x4 Matrix
    int m = 4; // Number of rows in A and C
    int n = 4; // Number of columns in B and C
#pragma omp parallel for
    for (int i = 0; i < n; ++i)
    {
#pragma omp critical
        std::cout << "Thread: " << omp_get_thread_num() << "Index: " << i << "\n";
        for (int j = 0; j < m; ++j)
        {
            float cell = 0.0f;
            for (int k = 0; k < m; ++k)
            {
                int aIndex = k + (i * m); // Row-major order
                int bIndex = j + (k * n); // Row-major order
                cell += a.m[aIndex] * b.m[bIndex];
            }
            int resIndex = j + (i * m);
            result.m[resIndex] = cell;
        }
    }
}

struct Matrix
{
    int nRows, nCols;
    long long** data;

    Matrix() = default;
    Matrix(int r, int c) : nRows{ r }, nCols{ c }
    {
        data = new long long* [nRows];
        for (int i = 0; i < nRows; ++i)
        {
            data[i] = new long long[nCols];
        }
    }
    ~Matrix()
    {
        if (nRows == 0)
            return;
        for (int i = 0; i < nRows; ++i)
            delete[] data[i];
        delete[] data;
    }
    // initialize the matrix by random numbers
    void init()
    {
        std::srand(std::time(0));
        for (int i = 0; i < nRows; ++i)
            for (int j = 0; j < nCols; ++j)
                data[i][j] = std::rand() % 20 - 10;
    }

    void print()
    {
        std::cout << "Matrix:\n";
        for (int i = 0; i < nRows; ++i)
        {
            for (int j = 0; j < nCols; ++j)
                std::cout << data[i][j] << ", ";
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    static void Mult(Matrix const& A, Matrix const& B, Matrix* result)
    {
        //checking if A's num of Col is equal to B's num of Row
        assert(A.nCols == B.nRows && "Matrices num of Cols and Rows do not match!");
        if (A.nCols != B.nRows)
        {
            std::cout << "A and B dimensions do not match!" <<
                " A.nCols=" << A.nCols << ", B.nRows=" << B.nRows << std::endl;
            return;
        }

        // do the multiplication
        for (int i = 0; i < A.nRows; ++i) // i <= A.nRows
        {
            for (int j = 0; j < B.nCols; ++j)
            {
                long long res = 0;
                for (int k = 0; k < A.nCols; ++k)
                    res += A.data[i][k] * B.data[k][j];
                result->data[i][j] = res;
            }
        }
    }
};

void SerialMatrixMult(Matrix const& A, Matrix const& B, Matrix* result)
{
    // Serial version of matrix multiplication
    for (int i = 0; i < A.nRows; ++i)
    {
        for (int j = 0; j < B.nCols; ++j)
        {
            long long res = 0;
            for (int k = 0; k < A.nCols; ++k)
            {
                res += A.data[i][k] * B.data[k][j];
            }
            result->data[i][j] = res;
        }
    }
}

void ParallelMatrixMult(Matrix const& A, Matrix const& B, Matrix* result)
{
    // Parallel version where; each thread computes a portion of rows
    omp_set_num_threads(NUM_THREADS);

#pragma omp parallel for
    for (int i = 0; i < A.nRows; ++i)
    {
        for (int j = 0; j < B.nCols; ++j)
        {
            long long res = 0;
            for (int k = 0; k < A.nCols; ++k)
            {
                res += A.data[i][k] * B.data[k][j];
            }
            result->data[i][j] = res;
        }
    }
}

void Print5x5(Matrix const& M, const char* name)
{
    // Print first 5 rows and 5 columns
    std::cout << name << " first 5x5:\n";
    int rows = (M.nRows < 5) ? M.nRows : 5;
    int cols = (M.nCols < 5) ? M.nCols : 5;

    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            std::cout << M.data[i][j] << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}
