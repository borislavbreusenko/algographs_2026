#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <random>
#include <utility>
#include <vector>

inline volatile std::sig_atomic_t stop_signal = 0;

struct StopRequested {};
struct TimeLimitReached {};

struct TimeLimit {
    using Clock = std::chrono::steady_clock;

    Clock::time_point (*now)();
    Clock::time_point deadline;
    mutable int checks_left = 1;

    explicit TimeLimit(Clock::duration limit = std::chrono::seconds(570),
                       Clock::time_point (*get_time)() = Clock::now)
        : now(get_time), deadline(now() + limit) {
    }

    bool Expired() const {
        return now() >= deadline;
    }
};

inline const TimeLimit* active_timer = nullptr;

inline void RequestStop(int signal) {
    stop_signal = signal;
}

inline void CheckSignal() {
    if (stop_signal != 0) {
        throw StopRequested{};
    }
    if (active_timer != nullptr && --active_timer->checks_left == 0) {
        active_timer->checks_left = 1024;
        if (active_timer->Expired()) {
            throw TimeLimitReached{};
        }
    }
}

struct BestSolution {
    std::vector<bool> removed;
    size_t size = 0;

    void Reset(int n) {
        removed.assign(n, true);
        size = n;
    }

    void Save(const std::vector<int>& answer) {
        if (answer.size() > size) {
            return;
        }
        std::fill(removed.begin(), removed.end(), false);
        for (auto node: answer) {
            removed[node] = true;
        }
        size = answer.size();
    }

    void Restore(int node) {
        if (removed[node]) {
            removed[node] = false;
            --size;
        }
    }

    void Exchange(int restored, int deleted) {
        removed[restored] = false;
        removed[deleted] = true;
    }

    std::vector<int> GetAnswer() const {
        std::vector<int> answer;
        answer.reserve(size);
        for (int node = 0; node < static_cast<int>(removed.size()); ++node) {
            if (removed[node]) {
                answer.push_back(node);
            }
        }
        return answer;
    }
};

inline BestSolution* saved_solution = nullptr;

inline void SaveAnswer(const std::vector<int>& answer) {
    if (saved_solution != nullptr) {
        saved_solution->Save(answer);
    }
}

struct Graph {
    int n;
    std::vector<std::vector<int>> adj;
    std::vector<std::vector<int>> reversed_adj;

    explicit Graph(int n_nodes) : n(n_nodes), adj(n_nodes), reversed_adj(n_nodes) {
    }
};

struct DFSState {
    int node;
    size_t next_index;
};

struct ScoredNode {
    int64_t score;
    int node;

    bool operator<(const ScoredNode& other) const {
        if (score != other.score) {
            return score < other.score;
        }
        return node > other.node;
    }
};

struct GreedyState {
    int active_count = 0;
    std::vector<int> in_degree;
    std::vector<int> out_degree;
    std::vector<bool> active;
    std::queue<int> peeling;
    std::priority_queue<ScoredNode> heap;

    explicit GreedyState(int n_nodes)
        : in_degree(n_nodes), out_degree(n_nodes), active(n_nodes, false) {
    }
};

struct TreapNode {
    int left = -1;
    int right = -1;
    int parent = -1;
    int size = 1;
    uint32_t priority = 0;
};

class ImplicitTreap {
private:
    std::vector<TreapNode> nodes_;
    int root_ = -1;

    int GetSize(int node) const {
        return node == -1 ? 0 : nodes_[node].size;
    }

    void Update(int node) {
        auto& current = nodes_[node];
        current.size = 1 + GetSize(current.left) + GetSize(current.right);
        if (current.left != -1) {
            nodes_[current.left].parent = node;
        }
        if (current.right != -1) {
            nodes_[current.right].parent = node;
        }
    }

    std::pair<int, int> Split(int node, int left_size) {
        if (node == -1) {
            return {-1, -1};
        }
        int size = GetSize(nodes_[node].left);
        if (left_size <= size) {
            auto parts = Split(nodes_[node].left, left_size);
            nodes_[node].left = parts.second;
            Update(node);
            nodes_[node].parent = -1;
            return {parts.first, node};
        }
        auto parts = Split(nodes_[node].right, left_size - size - 1);
        nodes_[node].right = parts.first;
        Update(node);
        nodes_[node].parent = -1;
        return {node, parts.second};
    }

    int Merge(int left, int right) {
        if (left == -1 || right == -1) {
            int node = left == -1 ? right : left;
            if (node != -1) {
                nodes_[node].parent = -1;
            }
            return node;
        }
        if (nodes_[left].priority > nodes_[right].priority) {
            nodes_[left].right = Merge(nodes_[left].right, right);
            Update(left);
            nodes_[left].parent = -1;
            return left;
        }
        nodes_[right].left = Merge(left, nodes_[right].left);
        Update(right);
        nodes_[right].parent = -1;
        return right;
    }

public:
    explicit ImplicitTreap(int n_nodes) : nodes_(n_nodes) {
        std::mt19937 random(20261030);
        for (auto& node: nodes_) {
            node.priority = random();
        }
    }

    int GetSize() const {
        return GetSize(root_);
    }

    int GetPosition(int node) const {
        int position = GetSize(nodes_[node].left);
        while (nodes_[node].parent != -1) {
            int parent = nodes_[node].parent;
            if (nodes_[parent].right == node) {
                position += 1 + GetSize(nodes_[parent].left);
            }
            node = parent;
        }
        return position;
    }

    void Insert(int node, int position) {
        auto parts = Split(root_, position);
        root_ = Merge(Merge(parts.first, node), parts.second);
    }

    void Erase(int node) {
        auto left = Split(root_, GetPosition(node));
        auto right = Split(left.second, 1);
        root_ = Merge(left.first, right.second);
        uint32_t priority = nodes_[node].priority;
        nodes_[node] = {};
        nodes_[node].priority = priority;
    }
};

struct ReachabilityState {
    std::vector<int> visited;
    std::vector<int> targets;
    std::vector<int> stack;
    int visit_id = 0;

    explicit ReachabilityState(int n_nodes) : visited(n_nodes, 0), targets(n_nodes, 0) {
    }
};

struct InsertionEvent {
    int position;
    int node;
    int delta;
};

struct InsertionMove {
    int position = -1;
    int conflict = -1;
};

struct LocalSearchStats {
    int swaps = 0;
    int returns = 0;
    int passes = 0;
};

struct DynamicSCCStats {
    int recomputations = 0;
    int excluded_vertices = 0;
};

struct BlockSearchStats {
    int64_t pairs = 0;
    int64_t triples = 0;
    int improvements = 0;
};

struct CycleSearchState {
    std::vector<unsigned char> color;
    std::vector<int> parent;
    std::vector<bool> in_cycle;
    std::vector<DFSState> stack;

    explicit CycleSearchState(int n) : color(n, 0), parent(n, -1), in_cycle(n, false) {
    }
};

struct SolutionStats {
    size_t two_cycle_size = 0;
    size_t greedy_size = 0;
    size_t insertion_size = 0;
    size_t reverse_delete_size = 0;
    size_t local_search_size = 0;
    double total_seconds = 0.0;
    int64_t completed_rounds = 0;
    bool time_limit_reached = false;
    std::array<int64_t, 5> turns{};
    std::array<int64_t, 5> gains{};
    std::array<double, 5> search_seconds{};
    LocalSearchStats local_search;
    DynamicSCCStats scc;
    BlockSearchStats blocks;
};
