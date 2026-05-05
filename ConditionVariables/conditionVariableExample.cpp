#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <iomanip>
#include <iostream>
#include <limits>
#include <math.h>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

std::atomic_bool done{ false }; // thread-safe flag (kept for compatibility with example)

struct Point {
    float x;    
    float y;
};

static inline int quarterIndex(const Point& p) {
    if (p.x >= 0.f && p.y >= 0.f) return 0; // (+,+)
    if (p.x < 0.f && p.y >= 0.f) return 1; // (-,+)
    if (p.x < 0.f && p.y < 0.f) return 2; // (-,-)
    return 3;                                // (+,-)
}

// Circular buffer that allows one producer and multiple consumers.
// Each consumer has its own read sequence so every consumer sees every produced Point.
class CircularBuffer {
public:
    CircularBuffer(int cap, int consumerCount)
        : capacity(cap),
        buffer(static_cast<size_t>(cap)),
        writeSeq(0),
        readSeqs(static_cast<size_t>(consumerCount), 0),
        prevMinReadSeq(0),
        consumerCount(consumerCount),
        finished(false)
    {
    }

    // Producer push (blocks if buffer is full w.r.t. unread items)
    void push(const Point& p)
    {
        std::unique_lock<std::mutex> lk(m);
        notFull.wait(lk, [this]() {
            return (writeSeq - minReadSeq()) < static_cast<size_t>(capacity) || finished;
            });
        if (finished) return;
        buffer[writeSeq % static_cast<size_t>(capacity)] = p;
        ++writeSeq;
        lk.unlock();
        notEmpty.notify_all();
    }

    // Consumer pop for a given consumerId.
    // Returns true if an item was written to `out`.
    // Returns false when producer finished and there is no more data to read.
    bool popForConsumer(int consumerId, Point& out)
    {
        std::unique_lock<std::mutex> lk(m);
        notEmpty.wait(lk, [this, consumerId]() {
            return readSeqs[static_cast<size_t>(consumerId)] < writeSeq || finished;
            });

        if (readSeqs[static_cast<size_t>(consumerId)] >= writeSeq) {
            // No new data available; if finished, consumer should exit.
            return false;
        }

        out = buffer[readSeqs[static_cast<size_t>(consumerId)] % static_cast<size_t>(capacity)];
        ++readSeqs[static_cast<size_t>(consumerId)];

        // If advancing this consumer increased the minimum read index, notify producer.
        size_t newMin = minReadSeq();
        if (newMin > prevMinReadSeq) {
            prevMinReadSeq = newMin;
            lk.unlock();
            notFull.notify_one();
        }
        else {
            lk.unlock();
        }
        return true;
    }

    // Signal producer done -> wake all waiting consumers/producers
    void setDone()
    {
        {
            std::lock_guard<std::mutex> g(m);
            finished = true;
        }
        notEmpty.notify_all();
        notFull.notify_all();
    }

private:
    size_t minReadSeq() const
    {
        if (readSeqs.empty()) return writeSeq;
        size_t minv = readSeqs[0];
        for (size_t i = 1; i < readSeqs.size(); ++i)
            if (readSeqs[i] < minv) minv = readSeqs[i];
        return minv;
    }

    int capacity;
    std::vector<Point> buffer;
    size_t writeSeq;
    std::vector<size_t> readSeqs;
    size_t prevMinReadSeq;
    int consumerCount;
    bool finished;

    std::mutex m;
    std::condition_variable notEmpty;
    std::condition_variable notFull;
};

// Result returned by each consumer thread
struct QuarterResult {
    int quarterId{ 0 };
    size_t totalPoints{ 0 };
    Point p1{ 0.f, 0.f };
    Point p2{ 0.f, 0.f };
    double minDist{ 0.0 };
    bool hasPair{ false };
};

// Consumer task: receives every produced point, keeps those in its quarter,
// and updates the closest pair incrementally (O(n^2) but simple and allowed for the assignment).
QuarterResult consumerTask(int consumerId, CircularBuffer& buf)
{
    QuarterResult res;
    res.quarterId = consumerId;
    double bestDist2 = std::numeric_limits<double>::infinity();
    std::vector<Point> localPoints;
    Point p;

    while (true) {
        bool ok = buf.popForConsumer(consumerId, p);
        if (!ok) {
            // No more data and producer finished.
            break;
        }

        if (quarterIndex(p) == consumerId) {
            // Compare new point to previous local points
            for (const auto& other : localPoints) {
                double dx = static_cast<double>(p.x) - static_cast<double>(other.x);
                double dy = static_cast<double>(p.y) - static_cast<double>(other.y);
                double d2 = dx * dx + dy * dy;
                if (d2 < bestDist2) {
                    bestDist2 = d2;
                    res.p1 = other;
                    res.p2 = p;
                    res.hasPair = true;
                }
            }
            localPoints.push_back(p);
            ++res.totalPoints;
        }
    }

    if (res.hasPair) res.minDist = std::sqrt(bestDist2);
    return res;
}

int main()
{
    const size_t PRODUCE_COUNT = 10000;
    const int CONSUMERS = 4;
    const int BUFFER_CAPACITY = 400; // similar to the example

    CircularBuffer dataBuf(BUFFER_CAPACITY, CONSUMERS);

    // Launch 4 consumer threads using async
    std::vector<std::future<QuarterResult>> futures;
    for (int i = 0; i < CONSUMERS; ++i) {
        futures.emplace_back(std::async(std::launch::async, consumerTask, i, std::ref(dataBuf)));
    }

    // Random generator for points in [-1000, 1000]
    std::mt19937 generator((unsigned int)std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<float> dist(-1000.f, 1000.f);

    // Producer: generate points with 30 ms pause
    for (size_t i = 0; i < PRODUCE_COUNT; ++i) {
        Point p{ dist(generator), dist(generator) };
        dataBuf.push(p);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    // Signal done and wake consumers
    dataBuf.setDone();

    // Collect results
    std::vector<QuarterResult> results;
    for (auto& f : futures) results.push_back(f.get());

    // Print final results
    std::cout << std::fixed << std::setprecision(1);
    for (const auto& r : results) {
        int qnum = r.quarterId + 1;
        if (!r.hasPair || r.totalPoints < 2) {
            std::cout << "quarter " << qnum
                << ": not enough points to determine a closest pair. Total number of point in this quarter is "
                << r.totalPoints << ".\n";
        }
        else {
            std::cout << "quarter " << qnum << ": closest points are ("
                << r.p1.x << ", " << r.p1.y << ") and ("
                << r.p2.x << ", " << r.p2.y << ") and their distance is "
                << r.minDist << ". Total number of point in this quarter is "
                << r.totalPoints << ".\n";
        }
    }

    return 0;
}