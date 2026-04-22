#include <iostream>
#include <omp.h> // Open MP Library
#include <chrono> // For measuring time

void Example1()
{
    //omp_set_num_threads(1000);

#pragma omp parallel
    {
        int id = omp_get_thread_num();

        printf(" Hello(%d)", id);
        printf(" World(%d)\n", id);
    }

#pragma omp parallel num_threads(1000) // Another way to set the number of threads for a specific parallel region
    {

    }
}

//static long numSteps = 10000000;
//double step = 0.0;
//int main()
//{
//    int i = 0;
//    double x = 0.0;
//    double pi = 0.0;
//    double sum = 0.0;
//
//    step = 1.0 / (double)numSteps;
//    for (i = 0; i < numSteps; i++)
//    {
//        x = (i + 0.5) * step;
//        sum = sum + 4.0 / (1.0 + x * x);
//    }
//
//    pi = step * sum;
//
//    //printf("Pi is approximately: %.16f\n", pi);
//    std::cout << "Pi: " << pi << "\n";
//
//    return 0;
//}

// In-Class Work:
#define NUM_THREADS 4
static long numSteps = 10000000;
double step = 0.0;

void Example2()
{
    int numThreads = 0;
    double x = 0.0;
    double pi = 0.0;
    double sum[NUM_THREADS] = { 0.0 };
    step = 1.0 / (double)numSteps;
    omp_set_num_threads(NUM_THREADS); // Sets the number of parallel threads

#pragma omp parallel
    {
        int i = 0;
        int id = omp_get_thread_num(); // Get the thread ID
        int nThreads = omp_get_num_threads(); // Get the total number of threads

        if (id == 0)
        {
            numThreads = nThreads;
        }
        for (i = id; i < numSteps; i += nThreads)
        {
            x = (i + 0.5) * step;
            sum[id] = sum[id] + (4.0 / (1.0 + x * x));
        }
    }
    for (int i = 0; i < numSteps; ++i)
    {
        std::cout << "ID: " << i << " Sum: " << sum[i] << "\n";

        pi += step * sum[i];
    }

    //printf("Pi is approximately: %.16f\n", pi);
    std::cout << "Pi: " << pi << "\n";

    //return 0;
} // Without False Sharing

void Example3()
{
    double pi = 0.0;
    step = 1.0 / (double)numSteps;
    omp_set_num_threads(NUM_THREADS); // Sets the number of parallel threads

    // Current CPU time (in utc)
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

#pragma omp parallel
    {
        double x = 0.0;
        double sum = 0.0;
        int i = 0;
        int id = omp_get_thread_num(); // Get the thread ID
        int nThreads = omp_get_num_threads(); // Get the total number of threads

        for (i = id; i < numSteps; i += nThreads)
        {
            x = (i + 0.5) * step;
            sum = sum + (4.0 / (1.0 + x * x));
        }
        for (i = id; i < numSteps; i += nThreads)
        {
            x = (i + 0.5) * step;
            sum += (4.0 / (1.0 + x * x));
        }
#pragma omp critical // Letting thread know a shared value is being updated
        {
            pi += step * sum;
        }
    }

    //printf("Pi is approximately: %.16f\n", pi);
    std::cout << "Pi: " << pi << "\n";

    std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
    int duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    std::cout << "Execution Time: " << duration << " ms\n";

    //return 0;
}

// Assignment 1: OpenMP PI Computing Assignment
float SerialPI_Integration(int numSteps)
{
    double step = 1.0 / (double)numSteps;
    double sum = 0.0;
    double x = 0.0;

    for (int i = 0; i < numSteps; i++)
    {
        x = (i + 0.5) * step;
        sum = sum + (4.0 / (1.0 + x * x));
    }
    return step * sum;
}

float ParallelPI_Integration(int numSteps)
{
    double step = 1.0 / (double)numSteps;
    double pi = 0.0;
    
    omp_set_num_threads(NUM_THREADS);

#pragma omp parallel
    {
        double x = 0.0;
        double sum = 0.0;
        int i = 0;
        int id = omp_get_thread_num();
        int nThreads = omp_get_num_threads();

        for (i = id; i < numSteps; i += nThreads)
        {
            x = (i + 0.5) * step;
            sum = sum + (4.0 / (1.0 + x * x));
        }

#pragma omp critical
        {
            pi += step * sum;
        }
    }
    return pi;
}

//int main()
//{
//    auto start = std::chrono::steady_clock::now();
//    auto end = std::chrono::steady_clock::now();
//
//    // Serial: n = 10 000
//    start = std::chrono::steady_clock::now();
//    float pi = SerialPI_Integration(10000);
//    end = std::chrono::steady_clock::now();
//    int duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
//    std::cout << "Serial PI (n= 10,000) = " << pi << ", Time: " << duration << " ms\n";
//
//    // Serial: n = 100 000
//    start = std::chrono::steady_clock::now();
//    pi = SerialPI_Integration(100000);
//    end = std::chrono::steady_clock::now();
//    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
//    std::cout << "Serial PI (n=100,000) = " << pi << ", Time: " << duration << " ms\n";
//
//    // Serial: n = 1 000 000
//    start = std::chrono::steady_clock::now();
//    pi = SerialPI_Integration(1000000);
//    end = std::chrono::steady_clock::now();
//    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
//    std::cout << "Serial PI (n=1,000,000) = " << pi << ", Time: " << duration << " ms\n\n";
//
//    // Parallel: n = 10 000
//    start = std::chrono::steady_clock::now();
//    pi = ParallelPI_Integration(10000);
//    end = std::chrono::steady_clock::now();
//    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
//    std::cout << "Parallel PI (n=10,000) = " << pi << ", Time: " << duration << " ms\n";
//
//    // Parallel: n = 100 000
//    start = std::chrono::steady_clock::now();
//    pi = ParallelPI_Integration(100000);
//    end = std::chrono::steady_clock::now();
//    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
//    std::cout << "Parallel PI (n=100,000) = " << pi << ", Time: " << duration << " ms\n";
//
//    // Parallel: n = 1 000 000
//    start = std::chrono::steady_clock::now();
//    pi = ParallelPI_Integration(1000000);
//    end = std::chrono::steady_clock::now();
//    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
//    std::cout << "Parallel PI (n= 1,000,000) = " << pi << ", Time: " << duration << " ms\n";
//
//    return 0;
//}

