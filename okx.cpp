#include <iostream>
#include <iomanip>
#include <cmath>
#include <string>
#include <curl/curl.h>

class CalculationTask
{
private:
   int n;
   double *A, *X, *A_temp, *X_temp, *E;
   void printRow(int i);
   bool allocateMemory(double **array, int size);
   void handleMemoryError();

public:
   CalculationTask(int size);
   ~CalculationTask();
   void printResult();
   void printEquation();
   void mainElement();
   void mainElementTemp();
   int gaussJordan();
   void matrixMultiplication();
   double calculateAccuracy();
};

int main()
{
   int n;
   double variable_for_time;
   std::cout << "Input the size of the array (N x N): ";
   std::cin >> n;
   while (n <= 1)
   {
      std::cout << "The size is invalid! The size must be greater than one.\n";
      std::cout << "Input the size of the array (N x N): ";
      std::cin >> n;
   }

   try
   {
      srand(time(0));
      CalculationTask calculator(n);

      std::cout << "\nMatrix " << n << " by " << n << " filled successfully.\n\n";
      calculator.printEquation();

      variable_for_time = clock();
      calculator.gaussJordan();
      calculator.mainElementTemp();
      calculator.matrixMultiplication();
      variable_for_time = clock() - variable_for_time;

      std::cout << "Matrix A and Matrix X after the gaussJordan function: \n\n";
      calculator.printEquation();

      std::cout << "Result of multiplying A (original) by X (after gaussJordan): \n\n";
      calculator.printResult();

      std::cout << "\nProgram execution time in seconds: " << std::setw(6) << std::setprecision(5) << variable_for_time / CLOCKS_PER_SEC << "\n\n";
      std::cout << std::scientific << std::setprecision(6) << "Norm ||AX - E|| = " << calculator.calculateAccuracy() << '\n';

      calculator.~CalculationTask();
      return 0;
   }
   catch (const std::bad_alloc &e)
   {
      std::cerr << "Memory allocation error: " << e.what() << std::endl;
      return -2; // Exit with an error code
   }
}

CalculationTask::CalculationTask(int size)
{
   n = size;

   if (!allocateMemory(&A, n * n + n * n))
   {
      handleMemoryError();
   }
   if (!allocateMemory(&A_temp, n * n + n * n))
   {
      handleMemoryError();
   }
   if (!allocateMemory(&E, n * n))
   {
      handleMemoryError();
   }

   X = A + n * n;
   X_temp = A_temp + n * n;
   int i;

   for (i = 0; i < n * n; i++)
   {
      A[i] = double(rand() % 10000) / double(1000);
   }

   for (i = 0; i < n * n; i++)
   {
      if (i % (n + 1) == 0)
         X[i] = 1.0;
      else
         X[i] = 0.0;
   }

   for (i = 0; i < n * n; i++)
   {
      A_temp[i] = A[i];
      X_temp[i] = X[i];
   }
}

CalculationTask::~CalculationTask()
{
   delete[] A;
   delete[] X;
   delete[] A_temp;
   delete[] X_temp;
   delete[] E;
}

void CalculationTask::printRow(int i)
{
   double epsilon = 1e-10;

   if (n >= 10)
   {
      std::cout << std::fixed;
      for (int j = 0; j < 5; j++)
         std::cout << std::setw(8) << std::setprecision(3) << (fabs(A[i * n + j]) < epsilon ? 0 : A[i * n + j]);
      std::cout << std::setw(8) << "..." << std::setw(8) << std::setprecision(3) << (fabs(A[i * n + n - 1]) < epsilon ? 0 : A[i * n + n - 1]) << " | ";
      for (int j = 0; j < 5; j++)
         std::cout << std::setw(8) << std::setprecision(3) << (fabs(X[i * n + j]) < epsilon ? 0 : X[i * n + j]);
      std::cout << std::setw(8) << "..." << std::setw(8) << std::setprecision(3) << (fabs(X[i * n + n - 1]) < epsilon ? 0 : X[i * n + n - 1]) << "\n";
   }
   else
   {
      for (int j = 0; j < n; j++)
         std::cout << std::setw(8) << std::setprecision(3) << (fabs(A[i * n + j]) < epsilon ? 0 : A[i * n + j]);
      std::cout << " | ";
      for (int j = 0; j < n; j++)
         std::cout << std::setw(8) << std::setprecision(3) << (fabs(X[i * n + j]) < epsilon ? 0 : X[i * n + j]);
      std::cout << '\n';
   }
}

void CalculationTask::printResult()
{
   double epsilon = 1e-10;
   if (n >= 10)
   {
      std::cout << std::fixed;
      for (int i = 0; i < 5; i++)
      {
         for (int j = 0; j < 5; j++)
            std::cout << std::setw(8) << std::setprecision(3) << (fabs(E[i * n + j]) < epsilon ? 0 : E[i * n + j]);
         std::cout << std::setw(8) << "..." << std::setw(8) << std::setprecision(3) << (fabs(E[i * n + n - 1]) < epsilon ? 0 : E[i * n + n - 1]) << '\n';
      }
   }
   else
   {
      for (int i = 0; i < n; i++)
      {
         for (int j = 0; j < n; j++)
            std::cout << std::setw(8) << std::setprecision(2) << (fabs(E[i * n + j]) < epsilon ? 0 : E[i * n + j]) << ' ';
         std::cout << '\n';
      }
   }
}

void CalculationTask::printEquation()
{
   if (n >= 10)
   {
      for (int i = 0; i < 5; i++)
         printRow(i);
      std::cout << '\n';
      for (int i = 0; i < 7; i++)
         std::cout << std::setw(8) << "...";
      std::cout << " | ";
      for (int i = 0; i < 7; i++)
         std::cout << std::setw(8) << "...";
      std::cout << '\n'
                << '\n';
      printRow(n - 1);
      std::cout << '\n';
   }
   else
      for (int i = 0; i < n; i++)
         printRow(i);
   std::cout << '\n';
}

void CalculationTask::mainElement()
{
   int i, j, k, max_row;
   double temp, max_element;

   for (i = 0; i < n; i++)
   {
      max_element = A[i * n + i];
      max_row = i;
      for (j = i + 1; j < n; j++)
      {
         if (fabs(max_element) < fabs(A[j * n + i]))
         {
            max_element = fabs(A[j * n + i]);
            max_row = j;
         }
      }

      if (fabs(max_element) < 1.e-20)
      {
         std::cout << "The matrix cannot be inverted!\n";
         return;
      }

      for (j = i; j < n; j++)
      {
         temp = A[i * n + j];
         A[i * n + j] = A[max_row * n + j];
         A[max_row * n + j] = temp;
      }
   }
}

void CalculationTask::mainElementTemp()
{
   int i, j, k, max_row;
   double temp, max_element;

   for (i = 0; i < n; i++)
   {
      max_element = A_temp[i * n + i];
      max_row = i;
      for (j = i + 1; j < n; j++)
      {
         if (fabs(max_element) < fabs(A_temp[j * n + i]))
         {
            max_element = fabs(A_temp[j * n + i]);
            max_row = j;
         }
      }

      if (fabs(max_element) < 1.e-20)
      {
         std::cout << "The matrix cannot be inverted!\n";
         return;
      }

      for (j = i; j < n; j++)
      {
         temp = A_temp[i * n + j];
         A_temp[i * n + j] = A_temp[max_row * n + j];
         A_temp[max_row * n + j] = temp;
      }
   }
}

int CalculationTask::gaussJordan()
{
   int i, j, k, max_row;
   double temp, max_element;

   mainElement();

   for (i = 0; i < n; i++)
   {
      temp = 1. / A[i * n + i];

      for (j = i; j < n; j++)
         A[i * n + j] *= temp;

      for (j = 0; j < n; j++)
         X[i * n + j] *= temp;

      for (j = i + 1; j < n; j++)
      {
         temp = A[j * n + i];
         for (k = i; k < n; k++)
            A[j * n + k] -= temp * A[i * n + k];

         for (k = 0; k < n; k++)
            X[j * n + k] -= temp * X[i * n + k];
      }
   }
   for (i = n - 1; i >= 0; i--)
   {
      for (j = i - 1; j >= 0; j--)
      {
         temp = A[j * n + i];
         for (k = 0; k < n; k++)
         {
            A[j * n + k] -= temp * A[i * n + k];
            X[j * n + k] -= temp * X[i * n + k];
         }
      }
   }
   return 0;
}

void CalculationTask::matrixMultiplication()
{
   int i, j, k;
   for (i = 0; i < n * n; i++)
      E[i] = 0.0;
   for (i = 0; i < n; i++)
      for (j = 0; j < n; j++)
         for (k = 0; k < n; k++)
            E[i * n + j] += A_temp[i * n + k] * X[k * n + j];
}

double CalculationTask::calculateAccuracy()
{
   double norma = 0.;
   int i, j, k;
   for (i = 0; i < n; i++)
   {
      for (j = 0; j < n; j++)
      {
         norma += E[i * n + j] * E[i * n + j];
      }
   }
   norma = sqrt(norma);
   return fabs(sqrt(n) - norma);
}

bool CalculationTask::allocateMemory(double **array, int size)
{
   *array = (double *)malloc(size * sizeof(double));
   if (!(*array))
   {
      std::cerr << "Error allocating memory for matrix!\n";
      return false;
   }
   return true;
}

void CalculationTask::handleMemoryError()
{
   std::cerr << "Error allocating memory. Program terminating.\n";
   exit(1); // Exit with an error code (optional)
}

// #include <iostream>
// using namespace std;

// int main()
// {
//    cout << "Hello World";
// } 