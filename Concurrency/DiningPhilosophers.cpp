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
     
    void Eat() {
        int leftId = mId;
        int rightId = (mId + 1) % 5;

        int firstId = std::min(leftId, rightId);
        int secondId = std::max(leftId, rightId);

        std::mutex& firstFork = (firstId == leftId) ? mLeftFork : mRightFork;
        std::mutex& secondFork = (secondId == leftId) ? mLeftFork : mRightFork;

        std::unique_lock<std::mutex> lock1(firstFork);
        std::unique_lock<std::mutex> lock2(secondFork);

        std::cout << "Philosopher " << mId << " is eating using forks " << firstId << " and " << secondId << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    void Live() {
        auto startTime = std::chrono::steady_clock::now();
        
        while (std::chrono::steady_clock::now() - startTime < std::chrono::seconds(60)) {
            Eat();
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

    std::mutex forks[5];

    std::vector<std::thread> threads;
    std::vector<Philosopher*> philosophers;

    for (int i = 0; i < 5; ++i) {

        Philosopher* p = new Philosopher(i, forks[i], forks[(i + 1) % 5]);
        philosophers.push_back(p);
        
        threads.emplace_back(&Philosopher::Live, p);
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    for (auto p : philosophers) {
        delete p;
    }

    std::cout << "Simulation complete. All philosophers have finished eating." << std::endl;

    return 0;
}
