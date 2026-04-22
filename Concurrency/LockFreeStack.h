#pragma once

#include <thread>
#include <atomic>
#include <memory>

/**
 * @brief A thread-safe, lock-free stack implementation.
 * Uses atomic operations to manage synchronization without mutexes.
 */
template<typename T>
class LockFreeStack {
private:
    struct Node {
        explicit Node(T const& val)
            : data(std::make_shared<T>(val)), next(nullptr) {}

        std::shared_ptr<T> data;
        Node* next;
    };

    std::atomic<Node*> mHead{nullptr};
    std::atomic<Node*> mToBeDeleted{nullptr};
    std::atomic<unsigned> mThreadsInPop{0};

public:
    /**
     * @return true if the stack contains no elements.
     */
    auto IsEmpty() const -> bool {
        return mHead.load() == nullptr;
    }

    /**
     * @brief Pushes a new element onto the stack.
     * @param data The value to store.
     */
    void Push(T const& data) {
        Node* const newNode = new Node(data);
        newNode->next = mHead.load();
        
        // Atomic compare-and-swap loop to update the head pointer.
        while (!mHead.compare_exchange_weak(newNode->next, newNode)) {
            // Spin until successful
        }
    }

    /**
     * @brief Pops an element from the stack.
     * @return A shared_ptr to the popped data, or nullptr if empty.
     */
    auto Pop() -> std::shared_ptr<T> {
        ++mThreadsInPop;
        Node* oldHead = mHead.load();

        // Atomic loop for popping the head.
        while (oldHead && !mHead.compare_exchange_weak(oldHead, oldHead->next)) {
            // Spin until successful or stack becomes empty
        }

        std::shared_ptr<T> result;
        if (oldHead) {
            result.swap(oldHead->data);
            TryReclaim(oldHead);
        }

        return result;
    }

private:
    /**
     * @brief Attempts to reclaim memory from a retired node.
     * @param oldNode The node to potentially delete.
     */
    void TryReclaim(Node* oldNode) {
        if (mThreadsInPop.load() == 1) {
            // If this is the only thread in pop(), we can safely reclaim.
            Node* pending = mToBeDeleted.exchange(nullptr);
            
            if (--mThreadsInPop == 0) {
                DeleteNodes(pending);
            } else if (pending) {
                ChainPendingNodes(pending);
            }
            delete oldNode;
        } else {
            // Multiple threads active; queue node for later reclamation.
            ChainPendingANode(oldNode);
            --mThreadsInPop;
        }
    }

    void ChainPendingNodes(Node* nodes) {
        Node* last = nodes;
        while (Node* const next = last->next) {
            last = next;
        }
        EnlistPendingNodes(nodes, last);
    }

    void EnlistPendingNodes(Node* first, Node* last) {
        last->next = mToBeDeleted.load();
        while (!mToBeDeleted.compare_exchange_weak(last->next, first)) {
            // Update next pointer if it changed
        }
    }

    void ChainPendingANode(Node* n) {
        EnlistPendingNodes(n, n);
    }

    static void DeleteNodes(Node* nodes) {
        while (nodes) {
            Node* next = nodes->next;
            delete nodes;
            nodes = next;
        }
    }
};
