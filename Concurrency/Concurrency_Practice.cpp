#include <iostream>
#include <future>
#include <thread>
#include <sstream>
#include <chrono>
#include <random>
#include <vector>
#include <mutex>
#include "LockFreeStack.h"

/**
 * @brief Thread-safe logging utility.
 */
void WriteMessage(const std::string& message) {
    std::cout << message << "\n";
}

/**
 * @brief Demonstrates basic std::async usage.
 */
void Example1() {
    auto f = std::async(WriteMessage, "Hello from background task (std::async)");

    for (int i = 0; i < 50; ++i) {
        WriteMessage("Message from MAIN thread");
    }

    f.wait(); // Block until task completes
}

/**
 * @brief Demonstrates basic std::thread usage.
 */
void Example2() {
    std::thread t(WriteMessage, "Hello from separate thread (std::thread)");
    
    for (int i = 0; i < 50; ++i) {
        WriteMessage("Message from MAIN thread");
    }

    t.join(); // Ensure thread finishes before function exit
}

/**
 * @brief Demonstrates deferred execution with std::async.
 */
void Example3() {
    // std::launch::deferred: task runs when get() or wait() is explicitly called
    auto f = std::async(std::launch::deferred, WriteMessage, "Executing deferred async task");

    WriteMessage("Main thread is running...");

    (void)getchar(); // Pause execution
    f.wait();
}

/**
 * @brief Helper function returning a value.
 */
auto FindTheAnswer() -> int {
    return 42;
}

/**
 * @brief Demonstrates retrieving values from std::async.
 */
void Example4() {
    auto f = std::async(FindTheAnswer);
    std::cout << "The answer is: " << f.get() << "\n";
}

/**
 * @brief Helper for string copying.
 */
auto CopyString(const std::string& str) -> std::string {
    return str;
}

/**
 * @brief Demonstrates parameter passing to async tasks.
 */
void Example5() {
    std::string s = "Initial string";
    
    // Passing by reference using std::ref
    auto f = std::async(std::launch::async, CopyString, std::ref(s));

    s = "Modified string";
    std::cout << "Result: " << f.get() << "\n";
}

/**
 * @brief Sets a value on a promise.
 */
void FindThePromiseAnswer(std::promise<int>* p) {
    p->set_value(42);
}

/**
 * @brief Demonstrates std::promise and std::future interaction.
 */
void Example6() {
    std::promise<int> p;
    auto f = p.get_future();
    
    std::thread t(FindThePromiseAnswer, &p);
    std::cout << "Retrieved from promise: " << f.get() << "\n";

    t.join();
}

/**
 * @brief Demonstrates std::packaged_task usage.
 */
void Example7() {
    std::packaged_task<int()> task(FindTheAnswer);
    auto f = task.get_future();
    
    std::thread t(std::move(task));
    std::cout << "Result from packaged task: " << f.get() << "\n";
    
    t.join();
}

/**
 * @brief Thread function waiting for a shared signal.
 */
void WaitForNotify(int id, std::shared_future<int> sf) {
    std::ostringstream os;
    os << "[Thread " << id << "] Waiting for signal...\n";
    std::cout << os.str();
    
    os.str("");
    os << "[Thread " << id << "] Received value: " << sf.get() << "\n";
    std::cout << os.str();
}

/**
 * @brief Demonstrates std::shared_future for multiple-thread notification.
 */
void Example8() {
    std::promise<int> p;
    auto sf = p.get_future().share();

    std::thread t1(WaitForNotify, 1, sf);
    std::thread t2(WaitForNotify, 2, sf);

    std::cout << "Press Enter to broadcast signal...\n";
    std::cin.get();
    
    p.set_value(42);
    
    t1.join();
    t2.join();
}

std::mutex myMutex;

/**
 * @brief Function demonstrating lock_guard usage.
 */
void MyFunctionThatUsesMutex() {
    std::lock_guard<std::mutex> guard(myMutex);
    std::cout << "Executing critical section with lock_guard\n";
}

/**
 * @brief Demonstrates basic mutex locking.
 */
void Example9() {
    myMutex.lock();
    std::thread t(MyFunctionThatUsesMutex);
    
    for (int i = 0; i < 5; ++i) {
        std::cout << "Main thread working...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    myMutex.unlock();
    t.join();
}

/**
 * @brief Function demonstrating unique_lock flexible locking.
 */
void FunctionThatUsesUniqueLock(int i) {
    std::unique_lock<std::mutex> guard(myMutex);
    std::cout << "[UniqueLock " << i << "] Critical section start\n";
    
    guard.unlock();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    guard.lock();
    std::cout << "[UniqueLock " << i << "] Critical section resumed\n";
}

/**
 * @brief Demonstrates std::unique_lock flexibility.
 */
void Example10() {
    std::unique_lock<std::mutex> guard(myMutex);
    
    std::thread t1(FunctionThatUsesUniqueLock, 1);
    std::thread t2(FunctionThatUsesUniqueLock, 2);

    std::cout << "Main thread holding lock...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    guard.unlock(); // Release to allow threads to proceed
    
    t1.join();
    t2.join();
}

/**
 * @brief Simple bank account model for deadlock prevention demo.
 */
class Account {
public:
    explicit Account(int balance) : balance(balance) {}

    /**
     * @brief Safe transfer between two accounts using std::lock.
     */
    friend void Transfer(Account& from, Account& to, int amount) {
        // Lock both mutexes simultaneously to prevent deadlocks
        std::lock(from.mutex, to.mutex);
        std::lock_guard<std::mutex> lockFrom(from.mutex, std::adopt_lock);
        std::lock_guard<std::mutex> lockTo(to.mutex, std::adopt_lock);

        from.balance -= amount;
        to.balance += amount;
    }

    auto GetBalance() const -> int { return balance; }

private:
    std::mutex mutex;
    int balance;
};

/**
 * @brief Demonstrates deadlock-safe multi-mutex locking.
 */
void Example11() {
    Account ibrahim(1000);
    Account santi(500);

    Transfer(ibrahim, santi, 100);

    std::cout << "Account Ibrahim: " << ibrahim.GetBalance() << "\n";
    std::cout << "Account Santi: " << santi.GetBalance() << "\n";
}

// Lock-Free Stack Integration Demo
std::mutex gPrintLock;
bool gDone = false;
LockFreeStack<unsigned> gErrorCodes;

/**
 * @brief Consumer thread: processes items from the lock-free stack.
 */
void LoggerFunction() {
    {
        std::lock_guard<std::mutex> lock(gPrintLock);
        std::cout << "[Logger] Starting...\n";
    }

    while (!gDone) {
        if (!gErrorCodes.IsEmpty()) {
            auto code = gErrorCodes.Pop();
            if (code) {
                std::lock_guard<std::mutex> lock(gPrintLock);
                std::cout << "[Logger] Processed error: " << *code << "\n";
            }
        }
    }
}

/**
 * @brief Producer thread: pushes items to the lock-free stack.
 */
void WorkerFunction(int id, std::mt19937& rng) {
    {
        std::lock_guard<std::mutex> lock(gPrintLock);
        std::cout << "[Worker " << id << "] Starting...\n";
    }

    while (!gDone) {
        std::this_thread::sleep_for(std::chrono::seconds(1 + rng() % 5));
        
        unsigned errorCode = id * 100 + (rng() % 50);
        gErrorCodes.Push(errorCode);
    }
}
//
//int main() {
//    auto seed = static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count());
//    std::mt19937 rng(seed);
//    
//    std::vector<std::thread> threads;
//
//    // Launch loggers
//    for (int i = 0; i < 3; ++i) {
//        threads.emplace_back(LoggerFunction);
//    }
//
//    // Launch workers
//    for (int i = 0; i < 3; ++i) {
//        threads.emplace_back(WorkerFunction, i + 1, std::ref(rng));
//    }
//
//    // Run simulation for 10 seconds
//    std::this_thread::sleep_for(std::chrono::seconds(10));
//
//    gDone = true;
//    for (auto& t : threads) {
//        if (t.joinable()) {
//            t.join();
//        }
//    }
//
//    std::cout << "Simulation complete.\n";
//    return 0;
//}
//
