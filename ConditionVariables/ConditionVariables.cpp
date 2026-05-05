//#include <iostream>
//#include <string>
//#include <vector>
//#include <mutex>
//#include <thread>
//#include <chrono>
//#include <random>
//#include <future>
//
//class Entity
//{
//public:
//    Entity(const std::string& name, float x, float y, float z)
//        : mName(name)
//        , mX(x)
//        , mY(y)
//        , mZ(z)
//    {
//
//    }
//
//    void Update(float deltaTime)
//    {
//        float moveX = (rand() % 21) - 10; 
//        float moveZ = (rand() % 21) - 10; 
//        float distance = sqrtf(moveX * moveX + moveZ * moveZ);
//        if (std::abs(distance) > 0.01f)
//        {
//            moveX /= distance; 
//            moveZ /= distance;
//        }
//        mX += moveX * deltaTime; 
//        mZ += moveZ * deltaTime;
//        std::cout << "Update: " << mName <<
//            " moved a distance of (" << distance << ")\n";
//    }
//
//    void Render()
//    {
//        std::cout << "Render: " << mName <<
//            " is at position (" << mX << ", " << mY << ", " << mZ << ")\n";
//    }
//
//    const std::string& GetName() const { return mName; }
//
//private:
//    std::string mName = "";
//    float mX = 0.0f, mY = 0.0f, mZ = 0.0f;
//    bool mHasRenderData = false;
//};
//
//class EntityManager
//{
//public:
//    void AddEntity(Entity& entity)
//    {
//        mEntities.push_back(entity);
//    }
//
//    void RemoveEntity(Entity& entity)
//    {
//        auto iter = std::find_if(mEntities.begin(), mEntities.end(), [entity](Entity& e)
//            {
//                return e.GetName() == entity.GetName();
//            });
//        if (iter != mEntities.end())
//        {
//            mEntities.erase(iter);
//        }
//    }
//
//    std::vector<Entity>& GetEntities() { return mEntities; }
//
//private:
//    std::vector<Entity> mEntities;
//};
//
//class Simulation
//{
//public:
//    void Initialize(EntityManager& em, std::mutex& mutex)
//    {
//        mEntityManager = &em;
//        mGameMutex = &mutex;
//        mIsRunning = true;
//    }
//
//    void Update()
//    {
//        std::chrono::milliseconds sleepTime(1000); 
//        const float deltaTime = 1.0f / 60.0f; 
//        while (mIsRunning)
//        {
//            mGameMutex->lock();
//            system("cls"); 
//            std::cout << "Running Update\n";
//            std::vector<Entity>& entities = mEntityManager->GetEntities();
//            for (Entity& e : entities)
//            {
//                e.Update(deltaTime);
//            }
//            std::this_thread::sleep_for(sleepTime);
//            mGameMutex->unlock();
//            std::this_thread::sleep_for(sleepTime);
//        }
//    }
//
//    void Terminate()
//    {
//        mIsRunning = false;
//    }
//
//private:
//    EntityManager* mEntityManager = nullptr;
//    std::mutex* mGameMutex = nullptr;
//    bool mIsRunning = false;
//};
//
//class Render
//{
//public:
//    void Initialize(EntityManager& em, std::mutex& mutex)
//    {
//        mEntityManager = &em;
//        mGameMutex = &mutex;
//        mIsRunning = true;
//    }
//
//    void RenderEntities()
//    {
//        std::chrono::milliseconds sleepTime(1000);
//        while (mIsRunning)
//        {
//            mGameMutex->lock();
//            system("cls"); 
//            std::cout << "Running Render\n";
//            std::vector<Entity>& entities = mEntityManager->GetEntities();
//            for (Entity& e : entities)
//            {
//                e.Render();
//            }
//            std::this_thread::sleep_for(sleepTime);
//            mGameMutex->unlock();
//            std::this_thread::sleep_for(sleepTime);
//        }
//    }
//
//    void Terminate()
//    {
//        mIsRunning = false;
//    }
//
//private:
//    EntityManager* mEntityManager = nullptr;
//    std::mutex* mGameMutex = nullptr;
//    bool mIsRunning = false;
//};
//
//
//float RandFloat()
//{
//    float val = (rand() % 201) - 100;
//    return val;
//}
//
//void GameLoopExample()
//{
//    std::cout << "Game Simulation!\n";
//
//    std::mutex gameMutex;
//    EntityManager entityManager;
//    Simulation simulation;
//    Render render;
//
//    simulation.Initialize(entityManager, gameMutex);
//    render.Initialize(entityManager, gameMutex);
//
//    for (int i = 0; i < 4; ++i) 
//    {
//        std::string name = "Player " + std::to_string(i);
//        Entity newEntity(name, RandFloat(), 0.0f, RandFloat());
//        entityManager.AddEntity(newEntity);
//    }
//
//    for (int i = 0; i < 16; ++i) 
//    {
//        std::string name = "Enemy " + std::to_string(i);
//        Entity newEntity(name, RandFloat(), 0.0f, RandFloat());
//        entityManager.AddEntity(newEntity);
//    }
//
//    std::thread simThread(&Simulation::Update, &simulation);
//    std::thread renderThread(&Render::RenderEntities, &render);
//
//    system("pause"); 
//    simulation.Terminate(); 
//    render.Terminate();
//
//    renderThread.join();
//    simThread.join();
//}
//
//std::atomic_bool done = false;
//
//struct CircularBuffer
//{
//    int* buffer;
//    int capacity;
//
//    int frontIndex;
//    int rearIndex;
//    int count;
//    std::mutex mutex;
//    std::condition_variable notEmpty;
//    std::condition_variable notFull;
//
//    CircularBuffer(int cap)
//        : capacity(cap)
//        , frontIndex(0)
//        , rearIndex(0)
//        , count(0)
//        , buffer(new int[cap])
//    {
//
//    }
//
//    void Push(int num)
//    {
//        std::unique_lock<std::mutex> lk(mutex);
//        notFull.wait(lk, [this]() { return count != capacity; });
//        buffer[rearIndex] = num;
//        rearIndex = (rearIndex + 1) % capacity;
//        ++count;
//
//        lk.unlock();
//        notEmpty.notify_one();
//    }
//    int Pop()
//    {
//        std::unique_lock<std::mutex> lk(mutex);
//        notEmpty.wait(lk, [this]() { return count > 0; });
//        int data = buffer[frontIndex];
//        frontIndex = (frontIndex + 1) % capacity;
//        --count;
//
//        lk.unlock();
//        notFull.notify_one();
//        return data;
//    }
//};
//
//std::mutex bufferMutex;
//int totalMin = INT_MAX;
//int totalMax = INT_MIN;
//void ConsumerFunction(CircularBuffer& buffer, int id)
//{
//    int localMin = INT_MAX;
//    int localMax = INT_MIN;
//    for (int i = 0; i < 950; ++i)
//    {
//        int value = buffer.Pop();
//        localMin = std::min(localMin, value);
//        localMax = std::max(localMax, value);
//    }
//    {
//        std::lock_guard<std::mutex> lk(bufferMutex);
//        std::cout << "Consumer " << id << ": local max [" << localMax << "] local min [" << localMin << "]\n";
//        totalMin = std::min(localMin, totalMin);
//        totalMax = std::max(localMax, totalMax);
//    }
//}
//
//void ProducerFunction(CircularBuffer& buffer, int id)
//{
//    auto sleepTime = std::chrono::milliseconds(10);
//    std::mt19937 randomGenerator((unsigned int)std::chrono::system_clock::now().time_since_epoch().count());
//    for (int i = 0; i < 1000; ++i)
//    {
//        buffer.Push(randomGenerator() % 100000);
//        std::this_thread::sleep_for(sleepTime);
//    }
//}
//
//void Exercise2()
//{
//    CircularBuffer dataBuffer(400);
//    std::thread consumer1(ConsumerFunction, std::ref(dataBuffer), 1);
//    std::thread consumer2(ConsumerFunction, std::ref(dataBuffer), 2);
//    std::thread producer1(ProducerFunction, std::ref(dataBuffer), 1);
//    std::thread producer2(ProducerFunction, std::ref(dataBuffer), 2);
//
//    producer2.join();
//    producer1.join();
//    consumer2.join();
//    consumer1.join();
//
//    std::cout << "Total Max: [" << totalMax << "] Total Min: [" << totalMin << "]\n";
//}
//
//struct Result
//{
//    int minValue = INT_MAX;
//    int maxValue = INT_MIN;
//};
//
//Result ConsumerFunctionAsync(CircularBuffer& buffer, int id)
//{
//    Result result;
//    for (int i = 0; i < 950; ++i)
//    {
//        int value = buffer.Pop();
//        result.minValue = std::min(result.minValue, value);
//        result.maxValue = std::max(result.maxValue, value);
//    }
//
//    return result;
//}
//
//
//int main()
//{
//    CircularBuffer dataBuffer(400);
//    std::future<Result> consumer1Result = std::async(std::launch::async, ConsumerFunctionAsync, std::ref(dataBuffer), 1);
//    std::future<Result> consumer2Result = std::async(std::launch::async, ConsumerFunctionAsync, std::ref(dataBuffer), 2);
//    std::thread producer1(ProducerFunction, std::ref(dataBuffer), 1);
//    std::thread producer2(ProducerFunction, std::ref(dataBuffer), 2);
//
//    Result result1 = consumer1Result.get();
//    Result result2 = consumer2Result.get();
//    producer2.join();
//    producer1.join();
//
//    std::cout << "Result 1 Max: [" << result1.maxValue << "] Min: [" << result1.minValue << "]\n";
//    std::cout << "Result 2 Max: [" << result2.maxValue << "] Min: [" << result2.minValue << "]\n";
//    totalMax = std::max(totalMax, result1.maxValue);
//    totalMax = std::max(totalMax, result2.maxValue);
//    totalMin = std::min(totalMin, result1.minValue);
//    totalMin = std::min(totalMin, result2.minValue);
//
//    std::cout << "Total Max: [" << totalMax << "] Total Min: [" << totalMin << "]\n";
//    return 0;
//}
