/*
** 2024-05-20
**
** Jindřich Bär, https://jindrich.bar
**   | https://github.com/barjin/master-thesis
**
******************************************************************************
**
** For a given graph (represented as a list of edges in CSV) and a list of node identifiers,
** this program calculates the distance matrix for the given nodes.
**
** For more context, see the thesis "Social network analysis in academic environment"
**    at
** https://jindrich.bar/master-thesis/bar-social-network-analysis-in-academic-environment-2024.pdf
*/

#include <iostream>
#include <map>
#include <queue>
#include <thread>
#include <unordered_set>

#include "csv.h"

#define DEBUG 0
#define debug_print(fmt, ...)                         \
    do {                                              \
        if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); \
    } while (0)
#define THREAD_COUNT 8

/**
 * A class representing an undirected graph.
 */
class Graph {
    /**
     * A map from (internal) node identifiers to a list of (internal) node
     * identifiers that are connected to the key node.
     */
    std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> edges;
    /**
     * A map from public node identifiers to internal node identifiers.
     */
    std::unordered_map<std::string, std::uint32_t> node_mapping;
    /**
     * A map from internal node identifiers to public node identifiers.
     */
    std::unordered_map<std::uint32_t, std::string> reverse_node_mapping;

    /**
     * Returns the list of neighbors of the given node.
     */
    const std::vector<std::uint32_t> &get_neighbors(std::uint32_t node) {
        return edges.at(node);
    }

   public:
    Graph() {}

    /**
     * Returns the number of distinct nodes in the graph.
     */
    std::uint32_t node_count() { return node_mapping.size(); }

    /**
     * Adds an edge between two nodes with the given identifiers.
     *
     * If the identifiers have not been seen before yet, it inserts them into
     * the (reverse_)node_mapping
     */
    void add_edge(std::string from, std::string to) {
        std::uint32_t from_id;
        std::uint32_t to_id;

        if (node_mapping.find(from) == node_mapping.end()) {
            from_id = node_mapping.size();
            node_mapping[from] = from_id;
            reverse_node_mapping[from_id] = from;
        } else {
            from_id = node_mapping[from];
        }

        if (node_mapping.find(to) == node_mapping.end()) {
            to_id = node_mapping.size();
            node_mapping[to] = to_id;
            reverse_node_mapping[to_id] = to;
        } else {
            to_id = node_mapping[to];
        }

        if (edges.find(from_id) == edges.end()) {
            edges[from_id] = std::vector<std::uint32_t>();
        }

        edges[from_id].push_back(to_id);
    }

    /**
     * Returns the distance between two nodes in the graph using the public node
     * identifiers.
     */
    std::uint32_t get_distance(const std::string &from, const std::string &to) {
        std::uint32_t from_id = node_mapping[from];
        std::uint32_t to_id = node_mapping[to];

        std::vector<std::uint32_t> visited(node_mapping.size(), 0);
        std::vector<std::uint32_t> distance(node_mapping.size(), 0);

        std::queue<std::uint32_t> q;
        q.push(from_id);
        visited[from_id] = 1;

        while (!q.empty() && visited[to_id] == 0) {
            std::uint32_t current = q.front();

            debug_print("Processing node %s\n",
                        reverse_node_mapping[current].c_str());
            q.pop();

            for (auto &nei : edges[current]) {
                if (visited[nei] == 0) {
                    debug_print("    Processing neighbor %s\n",
                                reverse_node_mapping[nei].c_str());
                    visited[nei] = 1;
                    distance[nei] = distance[current] + 1;
                    if (nei == to_id) {
                        break;
                    }
                    q.push(nei);
                }
            }
        }

        return distance[to_id];
    }

    /**
     * "Vectorized" version of get_distance that returns a list of distances
     * between a single start node and multiple other nodes.
     */
    std::vector<std::uint32_t> get_distance_v(
        const std::string &from, const std::vector<std::string> &to) {
        std::uint32_t from_id = node_mapping[from];
        std::unordered_set<std::uint32_t> to_ids;
        std::uint32_t found_targets = 0;

        for (auto &t : to) {
            to_ids.insert(node_mapping[t]);
        }

        std::vector<bool> visited(node_mapping.size(), 0);
        std::vector<std::uint32_t> distance(node_mapping.size(), 0);

        std::queue<std::uint32_t> q;
        q.push(from_id);
        visited[from_id] = true;

        while (!q.empty() && found_targets < to_ids.size()) {
            std::uint32_t current = q.front();

            debug_print("Processing node %s\n",
                        reverse_node_mapping[current].c_str());

            q.pop();
            for (auto &nei : edges[current]) {
                if (!visited[nei]) {
                    debug_print("    Processing neighbor %s\n",
                                reverse_node_mapping[nei].c_str());
                    visited[nei] = true;
                    distance[nei] = distance[current] + 1;
                    q.push(nei);
                    if (to_ids.find(nei) != to_ids.end()) {
                        found_targets++;
                        if (found_targets % 10 == 0) {
                            debug_print("Found %d/%ld targets\n", found_targets,
                                        to_ids.size());
                        }
                        if (found_targets == to_ids.size()) {
                            break;
                        }
                    }
                }
            }
        }

        std::vector<std::uint32_t> result;
        for (auto &t : to) {
            result.push_back(distance[node_mapping[t]]);
        }

        return result;
    }
};

/**
 * A list of public node identifiers to be used as the starting point for the
 * distance calculation.
 */
std::vector<std::string> merge_candidates = {"1025-roser-fernandezcliment",
                                             "2-joachim-sodequist",
                                             "3-xiaoyu-sheng",
                                             "4-zhizhan-qiu",
                                             "5-anton-tadich",
                                             "6-qile-li"};

int main() {
    Graph g;

    {
        // The file with the edges - the small version is packaged with the
        // repository for testing
        io::CSVReader<2> in("edges.small.csv");
        std::string from;
        std::string to;

        std::size_t i = 0;

        while (in.read_row(from, to)) {
            if (++i % 1000000 == 0) {
                std::cout << "Read " << i << " edges" << std::endl;
            }
            g.add_edge(from, to);
            g.add_edge(to, from);
        }
    }

    printf("Graph loaded, %d nodes\n", g.node_count());

    std::chrono::steady_clock::time_point begin =
        std::chrono::steady_clock::now();

    std::mutex cout_mutex;
    std::vector<std::thread> threads;
    std::size_t THREAD_BATCH_SIZE = merge_candidates.size() / THREAD_COUNT;

    for (std::uint8_t thread_n = 0; thread_n < THREAD_COUNT; ++thread_n) {
        threads.push_back(std::thread([&g, thread_n, THREAD_BATCH_SIZE,
                                       &cout_mutex] {
            for (auto &id : std::vector<std::string>(
                     merge_candidates.begin() + thread_n * THREAD_BATCH_SIZE,
                     thread_n == THREAD_COUNT - 1
                         ? merge_candidates.end()
                         : merge_candidates.begin() +
                               (thread_n + 1) * THREAD_BATCH_SIZE)) {
                auto distances = g.get_distance_v(id, merge_candidates);

                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << id << ",";
                for (auto &d : distances) {
                    std::cout << d << ',';
                }
                std::cout << std::endl;
            }
        }));
    }

    for (auto &t : threads) {
        t.join();
    }
    std::chrono::steady_clock::time_point end =
        std::chrono::steady_clock::now();

    std::cout << "Duration: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                       begin)
                     .count()
              << "ms" << std::endl;

    return 0;
}
