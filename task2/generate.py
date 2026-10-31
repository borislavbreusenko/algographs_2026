import argparse
from pathlib import Path
import random


ROOT = Path(__file__).resolve().parent


def make_paths(graph, k, block_sizes, randomizer):
    paths = [[] for _ in range(k)]
    last = [-1] * k
    start = 0
    for size in block_sizes:
        for attempt in range(100):
            slots = list(range(k)) + [randomizer.randrange(k) for _ in range(size - k)]
            randomizer.shuffle(slots)
            next_last = last.copy()
            additions = [[] for _ in range(k)]
            for index, node in enumerate(range(start, start + size)):
                chosen = next((candidate for candidate in range(index, size)
                               if next_last[slots[candidate]] == -1
                               or node not in graph[next_last[slots[candidate]]]), None)
                if chosen is None:
                    break
                slots[index], slots[chosen] = slots[chosen], slots[index]
                path = slots[index]
                additions[path].append(node)
                next_last[path] = node
            else:
                for path, extra in zip(paths, additions):
                    path.extend(extra)
                last = next_last
                break
        else:
            raise ValueError("Could not avoid duplicate path arcs; use more paths or another seed")
        start += size
    return paths


def assign_closing_vertices(graph, paths, dag_size, randomizer):
    k = len(paths)
    for attempt in range(100):
        vertices = list(range(dag_size, dag_size + k))
        randomizer.shuffle(vertices)
        for index, path in enumerate(paths):
            chosen = next((candidate for candidate in range(index, k)
                           if path[0] not in graph[vertices[candidate]]
                           and vertices[candidate] not in graph[path[-1]]), None)
            if chosen is None:
                break
            vertices[index], vertices[chosen] = vertices[chosen], vertices[index]
        else:
            return vertices
    raise ValueError("Could not assign distinct closing arcs; use more paths or another seed")


def generate(n=200000, m=1000000, k=1000, seed=20261031, covers=4,
             boundary=5000, max_step=6000, decoys=8000):
    dag_size = n - k
    if covers < 1 or k < 2 * covers or dag_size <= 2 * boundary or max_step < 2:
        raise ValueError("Require covers>=1, k>=2*covers, n-k>2*boundary and max_step>=2")
    if m < covers * n:
        raise ValueError("Not enough arcs for the cycle covers")
    block_count = (dag_size + max_step // 2 - 1) // (max_step // 2)
    quotient, remainder = divmod(dag_size, block_count)
    block_sizes = [quotient + (index < remainder) for index in range(block_count)]
    if min(block_sizes) < k or max(block_sizes) > boundary:
        raise ValueError("Blocks must contain every path and fit inside the boundary window")
    if not 1 <= decoys <= dag_size - 2 * boundary:
        raise ValueError("Decoys must fit strictly between the boundary windows")

    randomizer = random.Random(seed)
    graph = [set() for _ in range(n)]
    for _ in range(covers):
        paths = make_paths(graph, k, block_sizes, randomizer)
        closing = assign_closing_vertices(graph, paths, dag_size, randomizer)
        for path, vertex in zip(paths, closing):
            for source, target in zip(path, path[1:]):
                graph[source].add(target)
            graph[path[-1]].add(vertex)
            graph[vertex].add(path[0])
    assert sum(map(len, graph)) == covers * n

    trap_random = random.Random(seed ^ 0x5A17)
    hubs = trap_random.sample(range(boundary, dag_size - boundary), decoys)
    hub_set = set(hubs)
    prefix = [0]
    for node in range(dag_size):
        prefix.append(prefix[-1] + (node in hub_set))
    available = 0
    for source in range(dag_size):
        end = min(dag_size, source + max_step + 1)
        available += end - source - 1 if source in hub_set else prefix[end] - prefix[source + 1]
        available -= sum(target < dag_size and (source in hub_set or target in hub_set)
                         for target in graph[source])
    trap_count = m - covers * n
    if trap_count > available:
        raise ValueError("Too many trap arcs for the allowed forward spans and decoys")
    added = 0
    while added < trap_count:
        hub = trap_random.choice(hubs)
        if trap_random.getrandbits(1):
            source, target = trap_random.randrange(max(0, hub - max_step), hub), hub
        else:
            source, target = hub, trap_random.randrange(hub + 1, min(dag_size, hub + max_step + 1))
        if target not in graph[source]:
            graph[source].add(target)
            added += 1

    labels = list(range(n))
    random.Random(seed ^ 0x1ABE1).shuffle(labels)
    shuffled = [[] for _ in range(n)]
    row_random = random.Random(seed ^ 0xA11CE)
    for node, neighbors in enumerate(graph):
        row = [labels[target] for target in sorted(neighbors)]
        row_random.shuffle(row)
        shuffled[labels[node]] = row
    answer = labels[dag_size:]
    return shuffled, answer


def write_instance(folder, graph, answer):
    folder.mkdir(parents=True, exist_ok=True)
    graph_path = folder / "graph.gr"
    with graph_path.open("w") as output:
        output.write(f"{len(graph)} {sum(map(len, graph))} 0\n")
        for row in graph:
            output.write(" ".join(str(node + 1) for node in row) + "\n")
    (folder / "optimal.ans").write_text("".join(f"{node + 1}\n" for node in answer))
    print(f"vertices={len(graph)} arcs={sum(map(len, graph))} optimum={len(answer)}")


def main():
    parser = argparse.ArgumentParser(description="Generate interwoven long cycles with trap arcs")
    parser.add_argument("--vertices", type=int, default=200000)
    parser.add_argument("--arcs", type=int, default=1000000)
    parser.add_argument("--optimum", type=int, default=1000)
    parser.add_argument("--seed", type=int, default=20261031)
    parser.add_argument("--covers", type=int, default=4)
    parser.add_argument("--boundary", type=int, default=5000)
    parser.add_argument("--max-step", type=int, default=6000)
    parser.add_argument("--decoys", type=int, default=8000)
    parser.add_argument("--output", type=Path, default=ROOT)
    args = parser.parse_args()
    graph, answer = generate(args.vertices, args.arcs, args.optimum, args.seed,
                             args.covers, args.boundary, args.max_step, args.decoys)
    write_instance(args.output, graph, answer)


if __name__ == "__main__":
    main()
