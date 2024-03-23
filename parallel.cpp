#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

class ComputationalTask {
public:
    void run(std::atomic<bool>& flag) {
        // Replace this with your computational task logic
        for (int i = 0; i < 60; ++i) {
            std::cout << "ComputationalTask: Working... (" << i + 1 << "/60)\n";
            std::this_thread::sleep_for(std::chrono::seconds(1)); // Simulate work

            // Check if one minute has passed
            if (flag) {
                std::cout << "ComputationalTask: One minute has passed. Stopping...\n";
                break;
            }
        }
        std::cout << "ComputationalTask: Finished!\n";
    }
};

class SimpleClass {
public:
    void run(std::atomic<bool>& flag) {
        // Replace this with your simple class logic
        int count = 0;
        while (!flag) {
            std::cout << "SimpleClass: Result: " << count << '\n';
            std::this_thread::sleep_for(std::chrono::seconds(1)); // Print every 1 second
            count++;
        }
        std::cout << "SimpleClass: Stopped.\n";
    }
};

int main() {
    std::atomic<bool> flag(false);

    ComputationalTask computationalTask;
    SimpleClass simpleClass;

    // Start threads
    std::thread compTaskThread(&ComputationalTask::run, &computationalTask, std::ref(flag));
    std::thread simpleClassThread(&SimpleClass::run, &simpleClass, std::ref(flag));

    // Wait for one minute
    std::this_thread::sleep_for(std::chrono::minutes(1));

    // Set flag to true to stop threads
    flag = true;

    // Join threads
    compTaskThread.join();
    simpleClassThread.join();

    return 0;
}
