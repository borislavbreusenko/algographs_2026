#include <chrono>
#include <cstdint>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "basic_algos.cpp"
#include "structures.cpp"

Graph ReadData() {
    int n, m, flag;
    if (!(std::cin >> n >> m >> flag)) {
        CheckSignal();
        throw std::runtime_error("Cannot read graph header");
    }
    if (saved_solution != nullptr) {
        saved_solution->Reset(n);
    }
    CheckSignal();
    Graph graph(n);
    std::string line;
    std::getline(std::cin, line);
    for (int node = 0; node < n; ++node) {
        CheckSignal();
        std::getline(std::cin, line);
        CheckSignal();
        std::istringstream input(line);
        int target;
        while (input >> target) {
            CheckSignal();
            --target;
            graph.adj[node].push_back(target);
            graph.reversed_adj[target].push_back(node);
        }
    }
    return graph;
}

void RemoveNode(const Graph& graph, int node, GreedyState& state) {
    if (!state.active[node]) {
        return;
    }
    state.active[node] = false;
    --state.active_count;
    for (auto target: graph.adj[node]) {
        CheckSignal();
        if (!state.active[target]) {
            continue;
        }
        --state.in_degree[target];
        if (state.in_degree[target] == 0) {
            state.peeling.push(target);
        }
    }
    for (auto target: graph.reversed_adj[node]) {
        CheckSignal();
        if (!state.active[target]) {
            continue;
        }
        --state.out_degree[target];
        if (state.out_degree[target] == 0) {
            state.peeling.push(target);
        }
    }
}

void IterativePeeling(const Graph& graph, GreedyState& state) {
    while (!state.peeling.empty()) {
        CheckSignal();
        int node = state.peeling.front();
        state.peeling.pop();
        RemoveNode(graph, node, state);
    }
}

std::vector<int> CoverTwoCycles(const Graph& graph) {
    std::vector<std::vector<int>> reciprocal(graph.n);
    std::vector<int> marked(graph.n, -1);
    std::vector<int> degree(graph.n, 0);
    for (int node = 0; node < graph.n; ++node) {
        CheckSignal();
        for (int target: graph.reversed_adj[node]) {
            CheckSignal();
            marked[target] = node;
        }
        for (int target: graph.adj[node]) {
            CheckSignal();
            if (target != node && marked[target] == node) {
                reciprocal[node].push_back(target);
                marked[target] = -1;
            }
        }
        degree[node] = static_cast<int>(reciprocal[node].size());
    }

    std::vector<long double> score(graph.n, 0);
    std::set<std::pair<long double, int>> candidates;
    for (int node = 0; node < graph.n; ++node) {
        CheckSignal();
        for (int target: reciprocal[node]) {
            CheckSignal();
            score[node] += 1.0L / degree[target];
        }
        if (degree[node] > 0) {
            candidates.insert({score[node], -node});
        }
    }

    std::vector<int> answer;
    std::vector<bool> touched(graph.n, false);
    std::vector<int> changed;
    while (!candidates.empty()) {
        CheckSignal();
        auto best = std::prev(candidates.end());
        int node = -best->second;
        candidates.erase(best);
        answer.push_back(node);
        int old_degree = degree[node];
        degree[node] = 0;
        changed.clear();
        auto touch = [&](int target) {
            if (!touched[target]) {
                candidates.erase({score[target], -target});
                touched[target] = true;
                changed.push_back(target);
            }
        };
        for (int neighbor: reciprocal[node]) {
            CheckSignal();
            if (degree[neighbor] == 0) {
                continue;
            }
            touch(neighbor);
            score[neighbor] -= 1.0L / old_degree;
            int before = degree[neighbor]--;
            if (degree[neighbor] == 0) {
                continue;
            }
            long double delta = 1.0L / degree[neighbor] - 1.0L / before;
            for (int target: reciprocal[neighbor]) {
                CheckSignal();
                if (degree[target] == 0) {
                    continue;
                }
                touch(target);
                score[target] += delta;
            }
        }
        for (int target: changed) {
            CheckSignal();
            touched[target] = false;
            if (degree[target] > 0) {
                candidates.insert({score[target], -target});
            }
        }
    }
    return answer;
}

std::vector<int> SolveGreedy(const Graph& graph, DynamicSCCStats* stats = nullptr,
                             size_t* two_cycle_size = nullptr) {
    if (stats != nullptr) {
        *stats = {};
    }
    auto answer = CoverTwoCycles(graph);
    if (two_cycle_size != nullptr) {
        *two_cycle_size = answer.size();
    }
    std::vector<bool> remaining(graph.n, true);
    for (int node: answer) {
        remaining[node] = false;
    }
    std::vector<int> nodes(graph.n);
    std::iota(nodes.begin(), nodes.end(), 0);
    std::vector<bool> visited(graph.n, false);
    auto components = SCCSearch(graph, nodes, remaining, visited);
    std::reverse(components.begin(), components.end());
    GreedyState state(graph.n);

    while (!components.empty()) {
        CheckSignal();
        auto component = std::move(components.back());
        components.pop_back();
        if (component.size() == 1) {
            continue;
        }
        for (auto node: component) {
            CheckSignal();
            state.active[node] = true;
            state.in_degree[node] = 0;
            state.out_degree[node] = 0;
        }
        state.active_count = static_cast<int>(component.size());
        for (auto node: component) {
            CheckSignal();
            for (auto target: graph.adj[node]) {
                CheckSignal();
                if (state.active[target]) {
                    ++state.out_degree[node];
                }
            }
            for (auto target: graph.reversed_adj[node]) {
                CheckSignal();
                if (state.active[target]) {
                    ++state.in_degree[node];
                }
            }
            if (state.in_degree[node] == 0 || state.out_degree[node] == 0) {
                state.peeling.push(node);
            }
        }

        IterativePeeling(graph, state);
        for (auto node: component) {
            CheckSignal();
            if (state.active[node]) {
                int64_t score =
                    static_cast<int64_t>(state.in_degree[node]) * state.out_degree[node];
                state.heap.push({score, node});
            }
        }

        int removed_since_scc = 0;
        int scc_interval = 1;
        while (!state.heap.empty()) {
            CheckSignal();
            auto current = state.heap.top();
            state.heap.pop();
            int node = current.node;
            if (!state.active[node]) {
                continue;
            }
            int64_t score = static_cast<int64_t>(state.in_degree[node]) * state.out_degree[node];
            if (current.score != score) {
                state.heap.push({score, node});
                continue;
            }
            answer.push_back(node);
            RemoveNode(graph, node, state);
            IterativePeeling(graph, state);
            ++removed_since_scc;

            if (stats == nullptr || state.active_count == 0 || removed_since_scc < scc_interval) {
                continue;
            }
            ++stats->recomputations;
            component.erase(std::remove_if(component.begin(), component.end(),
                                           [&](int target) { return !state.active[target]; }),
                            component.end());
            auto new_components = SCCSearch(graph, component, state.active, visited);
            if (new_components.size() == 1) {
                removed_since_scc = 0;
                scc_interval = (state.active_count + 19) / 20;
                continue;
            }

            for (auto target: component) {
                CheckSignal();
                state.active[target] = false;
            }
            state.active_count = 0;
            state.heap = {};
            for (auto& next: new_components) {
                CheckSignal();
                if (next.size() == 1) {
                    ++stats->excluded_vertices;
                } else {
                    auto first = std::min_element(next.begin(), next.end());
                    std::iter_swap(next.begin(), first);
                }
            }
            new_components.erase(
                std::remove_if(new_components.begin(), new_components.end(),
                               [](const std::vector<int>& next) { return next.size() == 1; }),
                new_components.end());
            std::sort(new_components.begin(), new_components.end(),
                      [](const std::vector<int>& left, const std::vector<int>& right) {
                          return left.front() < right.front();
                      });
            for (auto it = new_components.rbegin(); it != new_components.rend(); ++it) {
                CheckSignal();
                components.push_back(std::move(*it));
            }
            break;
        }
    }
    return answer;
}

std::vector<int> ImproveSolution(const Graph& graph, const std::vector<int>& removal_order) {
    SaveAnswer(removal_order);
    CheckSignal();
    std::vector<bool> active(graph.n, true);
    for (auto node: removal_order) {
        CheckSignal();
        active[node] = false;
    }
    auto order = TopologicalSort(graph, active);
    ImplicitTreap tree(graph.n);
    for (auto node: order) {
        CheckSignal();
        tree.Insert(node, tree.GetSize());
    }

    for (auto it = removal_order.rbegin(); it != removal_order.rend(); ++it) {
        CheckSignal();
        int node = *it;
        int left = -1;
        int right = tree.GetSize();
        for (auto target: graph.reversed_adj[node]) {
            CheckSignal();
            if (active[target]) {
                left = std::max(left, tree.GetPosition(target));
            }
        }
        for (auto target: graph.adj[node]) {
            CheckSignal();
            if (active[target]) {
                right = std::min(right, tree.GetPosition(target));
            }
        }
        if (left < right) {
            tree.Insert(node, left + 1);
            active[node] = true;
            if (saved_solution != nullptr) {
                saved_solution->Restore(node);
            }
        }
    }

    TopologicalSort(graph, active);
    std::vector<int> answer;
    for (auto node: removal_order) {
        CheckSignal();
        if (!active[node]) {
            answer.push_back(node);
        }
    }
    return answer;
}

std::vector<int> ReverseDelete(const Graph& graph, const std::vector<int>& removal_order,
                               std::vector<int>& pending) {
    SaveAnswer(removal_order);
    CheckSignal();
    std::vector<bool> active(graph.n, true);
    for (auto node: removal_order) {
        CheckSignal();
        active[node] = false;
    }
    while (!pending.empty() && active[pending.back()]) {
        CheckSignal();
        pending.pop_back();
    }
    if (pending.empty()) {
        pending = removal_order;
    }
    if (pending.empty()) {
        return removal_order;
    }
    int node = pending.back();
    pending.pop_back();
    ReachabilityState state(graph.n);
    if (!CanReturnVertex(graph, node, active, state)) {
        return removal_order;
    }
    auto answer = removal_order;
    answer.erase(std::find(answer.begin(), answer.end(), node));
    if (saved_solution != nullptr) {
        saved_solution->Restore(node);
    }
    return answer;
}

std::vector<int> LocalSearch(const Graph& graph, const std::vector<int>& answer,
                             LocalSearchStats& stats, std::mt19937& random) {
    SaveAnswer(answer);
    CheckSignal();
    ++stats.passes;
    auto removed = answer;
    std::vector<bool> active(graph.n, true);
    for (auto node: removed) {
        CheckSignal();
        active[node] = false;
    }
    auto order = TopologicalSort(graph, active);
    ImplicitTreap tree(graph.n);
    for (auto node: order) {
        CheckSignal();
        tree.Insert(node, tree.GetSize());
    }
    std::vector<int> candidates(removed.size());
    std::iota(candidates.begin(), candidates.end(), 0);
    std::shuffle(candidates.begin(), candidates.end(), random);
    for (auto index: candidates) {
        CheckSignal();
        int node = removed[index];
        auto move = FindInsertion(graph, node, active, tree, random());
        if (move.position == -1) {
            continue;
        }
        if (move.conflict != -1) {
            int position = tree.GetPosition(move.conflict);
            tree.Erase(move.conflict);
            if (position < move.position) {
                --move.position;
            }
            active[move.conflict] = false;
            removed[index] = move.conflict;
            ++stats.swaps;
        } else {
            removed[index] = -1;
            ++stats.returns;
        }
        tree.Insert(node, move.position);
        active[node] = true;
        if (saved_solution != nullptr) {
            if (move.conflict == -1) {
                saved_solution->Restore(node);
            } else {
                saved_solution->Exchange(node, move.conflict);
            }
        }
    }
    TopologicalSort(graph, active);
    removed.erase(std::remove(removed.begin(), removed.end(), -1), removed.end());
    return removed;
}

std::vector<int> BlockSearch(const Graph& graph, const std::vector<int>& answer,
                             BlockSearchStats& stats, int block_size,
                             const std::vector<std::vector<int>>& components,
                             std::mt19937& random) {
    SaveAnswer(answer);
    CheckSignal();
    std::vector<bool> removed(graph.n, false);
    for (auto node: answer) {
        CheckSignal();
        removed[node] = true;
    }
    std::vector<int> order(components.size());
    std::iota(order.begin(), order.end(), 0);
    std::shuffle(order.begin(), order.end(), random);
    for (auto index: order) {
        CheckSignal();
        const auto& component = components[index];
        std::vector<int> candidates;
        for (auto node: component) {
            CheckSignal();
            if (removed[node]) {
                candidates.push_back(node);
            }
        }
        if (candidates.size() < static_cast<size_t>(block_size)) {
            continue;
        }
        std::shuffle(candidates.begin(), candidates.end(), random);
        std::vector<int> block(candidates.begin(), candidates.begin() + block_size);
        std::vector<bool> active(graph.n, false);
        for (auto node: component) {
            CheckSignal();
            active[node] = !removed[node];
        }
        for (auto node: block) {
            active[node] = true;
        }
        CycleSearchState cycles(graph.n);
        std::vector<int> deleted;
        std::vector<int> improved;
        if (block_size == 2) {
            ++stats.pairs;
        } else {
            ++stats.triples;
        }
        if (TryBlock(graph, component, answer, block, deleted, active, cycles, improved)) {
            SaveAnswer(improved);
            ++stats.improvements;
            return improved;
        }
        return answer;
    }
    return answer;
}

std::array<int, 5> GetStrategyCalls(const std::array<int64_t, 5>& gains) {
    std::array<int, 5> calls{};
    int remaining = 100;
    int64_t total = std::accumulate(gains.begin(), gains.end(), int64_t{0});
    if (total == 0) {
        calls.fill(20);
        return calls;
    }
    while (true) {
        bool changed = false;
        for (int strategy = 0; strategy < 5; ++strategy) {
            if (calls[strategy] == 0 && remaining * gains[strategy] < 5 * total) {
                calls[strategy] = 5;
                remaining -= 5;
                total -= gains[strategy];
                changed = true;
            }
        }
        if (!changed) {
            break;
        }
    }
    int available = remaining;
    std::array<int64_t, 5> remainders;
    remainders.fill(-1);
    for (int strategy = 0; strategy < 5; ++strategy) {
        if (calls[strategy] == 0) {
            calls[strategy] = available * gains[strategy] / total;
            remainders[strategy] = available * gains[strategy] % total;
            remaining -= calls[strategy];
        }
    }
    while (remaining > 0) {
        auto next = std::max_element(remainders.begin(), remainders.end());
        ++calls[next - remainders.begin()];
        *next = -1;
        --remaining;
    }
    return calls;
}

std::vector<int> Solve(const Graph& graph, SolutionStats& stats,
                       const TimeLimit& timer = TimeLimit()) {
    stats = {};
    auto total_start = TimeLimit::Clock::now();
    auto* checkpoint = saved_solution;
    BestSolution best;
    best.Reset(graph.n);
    saved_solution = &best;
    auto* previous_timer = active_timer;
    active_timer = &timer;
    int current_strategy = -1;
    auto stage_start = total_start;
    size_t stage_size = best.size;
    auto finish = [&] {
        if (current_strategy != -1) {
            stats.search_seconds[current_strategy] +=
                std::chrono::duration<double>(TimeLimit::Clock::now() - stage_start).count();
            stats.gains[current_strategy] += static_cast<int64_t>(stage_size - best.size);
        }
        if (checkpoint != nullptr) {
            *checkpoint = best;
        }
        saved_solution = checkpoint;
        active_timer = previous_timer;
        stats.total_seconds =
            std::chrono::duration<double>(TimeLimit::Clock::now() - total_start).count();
    };
    try {
        auto answer = SolveGreedy(graph, &stats.scc, &stats.two_cycle_size);
        SaveAnswer(answer);
        stats.greedy_size = answer.size();
        stats.insertion_size = stats.reverse_delete_size = stats.local_search_size = answer.size();
        const auto components = SCCSearch(graph);
        std::vector<int> reverse_delete_order;
        std::array<std::mt19937, 5> random;
        for (int strategy = 0; strategy < 5; ++strategy) {
            random[strategy].seed(20261031 + strategy);
        }
        while (true) {
            auto calls = GetStrategyCalls(stats.gains);
            for (int strategy = 0; strategy < 5; ++strategy) {
                for (int iteration = 0; iteration < calls[strategy]; ++iteration) {
                    CheckSignal();
                    if (timer.Expired()) {
                        throw TimeLimitReached{};
                    }
                    current_strategy = strategy;
                    stage_start = TimeLimit::Clock::now();
                    stage_size = best.size;
                    ++stats.turns[strategy];
                    if (strategy == 0 && stats.turns[strategy] > 1) {
                        std::shuffle(answer.begin(), answer.end(), random[strategy]);
                    }
                    if (strategy == 0) {
                        answer = ImproveSolution(graph, answer);
                        stats.insertion_size = answer.size();
                    } else if (strategy == 1) {
                        answer = ReverseDelete(graph, answer, reverse_delete_order);
                        stats.reverse_delete_size = answer.size();
                    } else if (strategy == 2) {
                        answer = LocalSearch(graph, answer, stats.local_search, random[strategy]);
                        stats.local_search_size = answer.size();
                    } else {
                        answer = BlockSearch(graph, answer, stats.blocks, strategy - 1, components,
                                             random[strategy]);
                    }
                    SaveAnswer(answer);
                    stats.gains[strategy] += static_cast<int64_t>(stage_size - best.size);
                    stats.search_seconds[strategy] +=
                        std::chrono::duration<double>(TimeLimit::Clock::now() - stage_start).count();
                    current_strategy = -1;
                }
            }
            ++stats.completed_rounds;
        }
    } catch (const TimeLimitReached&) {
        stats.time_limit_reached = true;
    } catch (...) {
        finish();
        throw;
    }
    finish();
    return best.GetAnswer();
}

void PrintStats(const SolutionStats& stats, size_t answer_size) {
    std::cerr << "two_cycle_size=" << stats.two_cycle_size << "\n";
    std::cerr << "greedy_size=" << stats.greedy_size << "\n";
    std::cerr << "insertion_size=" << stats.insertion_size << "\n";
    std::cerr << "reverse_delete_size=" << stats.reverse_delete_size << "\n";
    std::cerr << "local_search_size=" << stats.local_search_size << "\n";
    std::cerr << "final_size=" << answer_size << "\n";
    constexpr const char* names[] = {"insertion", "reverse_delete", "local_search", "pairs",
                                     "triples"};
    for (int strategy = 0; strategy < 5; ++strategy) {
        std::cerr << names[strategy] << "_turns=" << stats.turns[strategy] << "\n";
        std::cerr << names[strategy] << "_gain=" << stats.gains[strategy] << "\n";
        std::cerr << names[strategy] << "_seconds=" << stats.search_seconds[strategy] << "\n";
    }
    std::cerr << "pairs_tried=" << stats.blocks.pairs << "\n";
    std::cerr << "triples_tried=" << stats.blocks.triples << "\n";
    std::cerr << "block_improvements=" << stats.blocks.improvements << "\n";
    std::cerr << "swaps=" << stats.local_search.swaps << "\n";
    std::cerr << "local_search_returns=" << stats.local_search.returns << "\n";
    std::cerr << "local_search_passes=" << stats.local_search.passes << "\n";
    std::cerr << "completed_rounds=" << stats.completed_rounds << "\n";
    std::cerr << "time_limit_reached=" << stats.time_limit_reached << "\n";
    std::cerr << "total_seconds=" << stats.total_seconds << "\n";
    std::cerr << "scc_recomputations=" << stats.scc.recomputations << "\n";
    std::cerr << "scc_excluded_vertices=" << stats.scc.excluded_vertices << "\n";
}

void PrintSolution(const std::vector<int>& answer) {
    for (auto node: answer) {
        std::cout << node + 1 << "\n";
    }
}

int main(int argc, char* argv[]) {
    TimeLimit timer;
    struct sigaction action{};
    action.sa_handler = RequestStop;
    sigemptyset(&action.sa_mask);
    sigaction(SIGTERM, &action, nullptr);
    sigaction(SIGINT, &action, nullptr);
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    bool show_stats = false;
    for (int index = 1; index < argc; ++index) {
        std::string arg = argv[index];
        if (arg == "--stats") {
            show_stats = true;
        } else {
            std::cerr << "Usage: solution [--stats]\n";
            return 1;
        }
    }
    BestSolution checkpoint;
    saved_solution = &checkpoint;
    active_timer = &timer;
    SolutionStats stats;
    try {
        auto graph = ReadData();
        Solve(graph, stats, timer);
    } catch (const TimeLimitReached&) {
        stats.time_limit_reached = true;
    } catch (const StopRequested&) {
        if (show_stats) {
            std::cerr << "stopped_by_signal=" << stop_signal << "\n";
        }
    }
    active_timer = nullptr;
    auto answer = checkpoint.GetAnswer();
    PrintSolution(answer);
    if (show_stats) {
        PrintStats(stats, answer.size());
    }
    saved_solution = nullptr;
    return 0;
}
