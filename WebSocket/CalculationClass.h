#ifndef CALCULATIONCLASS_H
#define CALCULATIONCLASS_H

#include <atomic>
#include <mutex>

class CalculationClass {
private:
    int n;
    double *A, *X, *A_temp, *X_temp, *E;
    void printRow(int i);
    void handleMemoryError();

public:
    CalculationClass(int size);
    void printResult();
    void printEquation();
    void mainElement();
    void mainElementTemp();
    int gaussJordan();
    void matrixMultiplication();
    double calculateAccuracy();
    void run(std::atomic<bool>& flag, std::atomic<int>& heavyTasksCount, std::mutex& mutex);
};

#endif // CALCULATIONCLASS_H
