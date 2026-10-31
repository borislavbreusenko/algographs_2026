#pragma once

#include <algorithm>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <vector>

#include "structures.cpp"

void DFS(const Graph& graph, int start, const std::vector<bool>& active, std::vector<bool>& visited,
         std::vector<int>& order) {
    std::vector<DFSState> stack{{start, 0}};
    visited[start] = true;

    while (!stack.empty()) {
        CheckSignal();
        auto& state = stack.back();
        int node = state.node;
        if (state.next_index == graph.adj[node].size()) {
            order.push_back(node);
            stack.pop_back();
            continue;
        }

        int target = graph.adj[node][state.next_index];
        ++state.next_index;
        if (!active[target] || visited[target]) {
            continue;
        }
        visited[target] = true;
        stack.push_back({target, 0});
    }
}

std::vector<std::vector<int>> SCCSearch(const Graph& graph, const std::vector<int>& nodes,
                                        const std::vector<bool>& active,
                                        std::vector<bool>& visited) {
    for (auto node: nodes) {
        CheckSignal();
        visited[node] = false;
    }
    std::vector<int> order;
    order.reserve(nodes.size());
    for (auto node: nodes) {
        CheckSignal();
        if (active[node] && !visited[node]) {
            DFS(graph, node, active, visited, order);
        }
    }

    for (auto node: nodes) {
        CheckSignal();
        visited[node] = false;
    }
    std::vector<std::vector<int>> components;
    std::vector<int> stack;
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        CheckSignal();
        int start = *it;
        if (visited[start]) {
            continue;
        }
        components.push_back({});
        auto& component = components.back();
        stack.push_back(start);
        visited[start] = true;

        while (!stack.empty()) {
            CheckSignal();
            int node = stack.back();
            stack.pop_back();
            component.push_back(node);
            for (auto target: graph.reversed_adj[node]) {
                CheckSignal();
                if (!active[target] || visited[target]) {
                    continue;
                }
                visited[target] = true;
                stack.push_back(target);
            }
        }
    }
    return components;
}

std::vector<std::vector<int>> SCCSearch(const Graph& graph) {
    std::vector<int> nodes(graph.n);
    std::iota(nodes.begin(), nodes.end(), 0);
    std::vector<bool> active(graph.n, true);
    std::vector<bool> visited(graph.n, false);
    return SCCSearch(graph, nodes, active, visited);
}

std::vector<int> TopologicalSort(const Graph& graph, const std::vector<bool>& active) {
    std::vector<int> in_degree(graph.n, 0);
    size_t active_count = 0;
    for (int node = 0; node < graph.n; ++node) {
        CheckSignal();
        if (!active[node]) {
            continue;
        }
        ++active_count;
        for (auto target: graph.adj[node]) {
            CheckSignal();
            if (active[target]) {
                ++in_degree[target];
            }
        }
    }

    std::queue<int> queue;
    for (int node = 0; node < graph.n; ++node) {
        CheckSignal();
        if (active[node] && in_degree[node] == 0) {
            queue.push(node);
        }
    }
    std::vector<int> order;
    order.reserve(active_count);
    while (!queue.empty()) {
        CheckSignal();
        int node = queue.front();
        queue.pop();
        order.push_back(node);
        for (auto target: graph.adj[node]) {
            CheckSignal();
            if (active[target]) {
                --in_degree[target];
                if (in_degree[target] == 0) {
                    queue.push(target);
                }
            }
        }
    }
    if (order.size() != active_count) {
        throw std::logic_error("Active graph contains a directed cycle");
    }
    return order;
}

bool CanReturnVertex(const Graph& graph, int node, const std::vector<bool>& active,
                     ReachabilityState& state) {
    CheckSignal();
    ++state.visit_id;
    state.stack.clear();
    for (auto target: graph.reversed_adj[node]) {
        CheckSignal();
        if (active[target]) {
            state.targets[target] = state.visit_id;
        }
    }
    for (auto target: graph.adj[node]) {
        CheckSignal();
        if (active[target]) {
            state.visited[target] = state.visit_id;
            state.stack.push_back(target);
        }
    }
    while (!state.stack.empty()) {
        CheckSignal();
        int current = state.stack.back();
        state.stack.pop_back();
        if (state.targets[current] == state.visit_id) {
            return false;
        }
        for (auto target: graph.adj[current]) {
            CheckSignal();
            if (active[target] && state.visited[target] != state.visit_id) {
                state.visited[target] = state.visit_id;
                state.stack.push_back(target);
            }
        }
    }
    return true;
}

InsertionMove FindInsertion(const Graph& graph, int node, const std::vector<bool>& active,
                            const ImplicitTreap& tree, uint32_t choice = 0) {
    std::vector<InsertionEvent> events;
    int conflicts = 0;
    int conflict_xor = 0;
    for (auto target: graph.reversed_adj[node]) {
        CheckSignal();
        if (active[target]) {
            ++conflicts;
            conflict_xor ^= target + 1;
            events.push_back({tree.GetPosition(target), target, -1});
        }
    }
    for (auto target: graph.adj[node]) {
        CheckSignal();
        if (active[target]) {
            events.push_back({tree.GetPosition(target), target, 1});
        }
    }
    if (conflicts == 0) {
        return {0, -1};
    }
    std::sort(events.begin(), events.end(),
              [](const InsertionEvent& left, const InsertionEvent& right) {
                  return left.position < right.position;
              });
    std::vector<InsertionMove> options;
    if (conflicts == 1) {
        options.push_back({0, conflict_xor - 1});
    }
    size_t index = 0;
    while (index < events.size()) {
        CheckSignal();
        int position = events[index].position;
        while (index < events.size() && events[index].position == position) {
            CheckSignal();
            conflicts += events[index].delta;
            conflict_xor ^= events[index].node + 1;
            ++index;
        }
        if (conflicts == 0) {
            return {position + 1, -1};
        }
        if (conflicts == 1) {
            options.push_back({position + 1, conflict_xor - 1});
        }
    }
    return options.empty() ? InsertionMove{} : options[choice % options.size()];
}

std::vector<int> FindCycle(const Graph& graph, const std::vector<int>& nodes,
                           const std::vector<bool>& active, CycleSearchState& state) {
    for (auto node: nodes) {
        CheckSignal();
        state.color[node] = 0;
    }
    state.stack.clear();
    for (auto start: nodes) {
        CheckSignal();
        if (!active[start] || state.color[start] != 0) {
            continue;
        }
        state.color[start] = 1;
        state.parent[start] = -1;
        state.stack.push_back({start, 0});
        while (!state.stack.empty()) {
            CheckSignal();
            auto& current = state.stack.back();
            int node = current.node;
            if (current.next_index == graph.adj[node].size()) {
                state.color[node] = 2;
                state.stack.pop_back();
                continue;
            }
            int target = graph.adj[node][current.next_index++];
            if (!active[target]) {
                continue;
            }
            if (state.color[target] == 1) {
                std::vector<int> cycle{target};
                for (int vertex = node; vertex != target; vertex = state.parent[vertex]) {
                    CheckSignal();
                    cycle.push_back(vertex);
                }
                return cycle;
            }
            if (state.color[target] == 0) {
                state.parent[target] = node;
                state.color[target] = 1;
                state.stack.push_back({target, 0});
            }
        }
    }
    return {};
}

bool FindReplacement(const Graph& graph, const std::vector<int>& component,
                     const std::vector<int>& block, int max_deleted,
                     std::vector<int>& deleted, std::vector<bool>& active,
                     CycleSearchState& cycles) {
    auto candidates = FindCycle(graph, component, active, cycles);
    CheckSignal();
    if (candidates.empty()) {
        return true;
    }
    if (max_deleted <= 0) {
        return false;
    }
    std::reverse(candidates.begin(), candidates.end());
    while (!candidates.empty()) {
        CheckSignal();
        int node = candidates.back();
        candidates.pop_back();
        if (std::find(block.begin(), block.end(), node) != block.end()) {
            continue;
        }
        active[node] = false;
        deleted.push_back(node);
        std::vector<int> cycle;
        bool found;
        if (max_deleted == 1) {
            cycle = FindCycle(graph, component, active, cycles);
            found = cycle.empty();
        } else {
            found = FindReplacement(graph, component, block, max_deleted - 1, deleted,
                                    active, cycles);
        }
        active[node] = true;
        if (found) {
            return true;
        }
        deleted.pop_back();
        if (max_deleted == 1) {
            for (auto target: cycle) {
                CheckSignal();
                cycles.in_cycle[target] = true;
            }
            candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
                                             [&](int target) {
                                                 CheckSignal();
                                                 return !cycles.in_cycle[target];
                                             }),
                             candidates.end());
            for (auto target: cycle) {
                CheckSignal();
                cycles.in_cycle[target] = false;
            }
        }
    }
    return false;
}

bool TryBlock(const Graph& graph, const std::vector<int>& component,
              const std::vector<int>& answer, const std::vector<int>& block,
              std::vector<int>& deleted, std::vector<bool>& active, CycleSearchState& cycles,
              std::vector<int>& improved) {
    size_t initial_size = deleted.size();
    bool found = block.size() == 3 && deleted.empty() &&
        FindCycle(graph, component, active, cycles).empty();
    if (!found && block.size() == 3 && deleted.empty()) {
        std::vector<bool> visited(graph.n, false);
        auto components = SCCSearch(graph, component, active, visited);
        components.erase(std::remove_if(components.begin(), components.end(),
                                         [&](const std::vector<int>& current) {
                                             CheckSignal();
                                             int node = current.front();
                                             return current.size() == 1 &&
                                                 std::find(graph.adj[node].begin(),
                                                           graph.adj[node].end(), node) ==
                                                     graph.adj[node].end();
                                         }),
                         components.end());
        if (components.size() > 2) {
            return false;
        }
        std::vector<bool> local_active(graph.n, false);
        int max_deleted = 3 - static_cast<int>(components.size());
        found = true;
        for (const auto& current: components) {
            for (auto node: current) {
                CheckSignal();
                local_active[node] = true;
            }
            found = FindReplacement(graph, current, block, max_deleted, deleted,
                                    local_active, cycles);
            for (auto node: current) {
                CheckSignal();
                local_active[node] = false;
            }
            if (!found) {
                break;
            }
        }
    } else if (!found) {
        int max_deleted = static_cast<int>(block.size()) - 1 - static_cast<int>(deleted.size());
        found = FindReplacement(graph, component, block, max_deleted, deleted, active, cycles);
    }
    if (found) {
        std::vector<int> trial;
        for (auto node: answer) {
            CheckSignal();
            if (std::find(block.begin(), block.end(), node) == block.end()) {
                trial.push_back(node);
            }
        }
        trial.insert(trial.end(), deleted.begin(), deleted.end());
        SaveAnswer(trial);
        improved = std::move(trial);
    }
    deleted.resize(initial_size);
    return found;
}
