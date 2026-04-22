#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <algorithm>


class Philosopher {
public:
    Philosopher(int id, std::mutex& leftFork, std::mutex& rightFork)
        : mId(id), mLeftFork(leftFork), mRightFork(rightFork) {}


     // @brief Simulates eating. Takes 1 second.
     // Prevents deadlock by locking the lower-indexed fork first (Resource Hierarchy).
     
    void Eat() {
        int leftId = mId;
        int rightId = (mId + 1) % 5;

        // Resource hierarchy: always pick up the fork with the lower ID first.
        int firstId = std::min(leftId, rightId);
        int secondId = std::max(leftId, rightId);

        std::mutex& firstFork = (firstId == leftId) ? mLeftFork : mRightFork;
        std::mutex& secondFork = (secondId == leftId) ? mLeftFork : mRightFork;

        // lock_guard or unique_lock must be used for mutex management.
        std::unique_lock<std::mutex> lock1(firstFork);
        std::unique_lock<std::mutex> lock2(secondFork);

        std::cout << "Philosopher " << mId << " is eating using forks " << firstId << " and " << secondId << std::endl;
        
        // Requirement: Each eating takes 1 second.
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    /**
     * @brief Main loop for the philosopher thread.
     */
    void Live() {
        auto startTime = std::chrono::steady_clock::now();
        
        // Requirement: All eat for total of 1 minute (60 seconds).
        while (std::chrono::steady_clock::now() - startTime < std::chrono::seconds(60)) {
            Eat();
            
            // Requirement: Pause for 4 seconds before attempting to eat again.
            // This time is spent "debating".
            std::this_thread::sleep_for(std::chrono::seconds(4));
        }
    }

private:
    int mId;
    std::mutex& mLeftFork;
    std::mutex& mRightFork;
};

int main() {
    std::cout << "Initializing Dining Philosophers simulation..." << std::endl;
    std::cout << "Simulation duration: 60 seconds." << std::endl;

    // 5 forks equally spaced around the table.
    std::mutex forks[5];

    std::vector<std::thread> threads;
    std::vector<Philosopher*> philosophers;

    // Create 5 philosophers and assign their threads.
    for (int i = 0; i < 5; ++i) {
        // Philosopher i is between fork i and fork (i+1)%5.
        Philosopher* p = new Philosopher(i, forks[i], forks[(i + 1) % 5]);
        philosophers.push_back(p);
        
        // Requirement: Use std::thread to implement the solution.
        threads.emplace_back(&Philosopher::Live, p);
    }

    // Join all threads after they complete their 1-minute lifecycle.
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Cleanup memory.
    for (auto p : philosophers) {
        delete p;
    }

    std::cout << "Simulation complete. All philosophers have finished eating." << std::endl;

    return 0;
}
