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

#include "csv.h"
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <thread>
#include <unordered_set>

#define DEBUG 0
#define debug_print(fmt, ...) \
    do { \
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
    std::uint32_t node_count() {
        return node_mapping.size();
    }

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

            debug_print(
                "Processing node %s\n", reverse_node_mapping[current].c_str()
            );
            q.pop();

            for (auto &nei : edges[current]) {
                if (visited[nei] == 0) {
                    debug_print(
                        "    Processing neighbor %s\n",
                        reverse_node_mapping[nei].c_str()
                    );
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
     * If there are no paths connecting the start node to a target node, the
     * distance is set to 999.
     */
    std::vector<std::uint32_t> get_distance_v(
        const std::string &from,
        const std::vector<std::string> &to,
        const std::unordered_set<std::string> &forbidden = {}
    ) {
        if (node_mapping.find(from) == node_mapping.end()) {
            auto x = std::find(to.begin(), to.end(), from);
            auto res = std::vector<std::uint32_t>(to.size(), 999);
            if (x != to.end()) {
                res[x - to.begin()] = 0;
            }
            return res;
        }
        std::uint32_t from_id = node_mapping.at(from);
        std::unordered_set<std::uint32_t> to_ids;
        std::uint32_t found_targets = 0;

        for (auto &t : to) {
            if (node_mapping.find(t) != node_mapping.end()) {
                to_ids.insert(node_mapping.at(t));
            }
        }

        std::vector<bool> visited(node_mapping.size(), 0);
        std::vector<std::uint32_t> distance(node_mapping.size(), 999);

        std::queue<std::uint32_t> q;
        q.push(from_id);
        visited[from_id] = true;
        distance[from_id] = 0;

        while (!q.empty() && found_targets < to_ids.size()) {
            std::uint32_t current = q.front();

            debug_print(
                "Processing node %s\n", reverse_node_mapping[current].c_str()
            );

            q.pop();
            for (auto &nei : edges[current]) {
                if (!visited[nei] &&
                    forbidden.find(reverse_node_mapping[nei]) ==
                        forbidden.end()) {
                    debug_print(
                        "    Processing neighbor %s\n",
                        reverse_node_mapping[nei].c_str()
                    );
                    visited[nei] = true;
                    distance[nei] = distance[current] + 1;
                    q.push(nei);
                    if (to_ids.find(nei) != to_ids.end()) {
                        found_targets++;
                        if (found_targets % 10 == 0) {
                            debug_print(
                                "Found %d/%ld targets\n",
                                found_targets,
                                to_ids.size()
                            );
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
            if (node_mapping.find(t) == node_mapping.end()) {
                result.push_back(999);
                continue;
            }
            result.push_back(distance.at(node_mapping.at(t)));
        }

        return result;
    }
};
std::string candidates_input =
    "1025-roser-fernandezcliment,2-joachim-sodequist,3-xiaoyu-sheng,4-zhizhan-qiu,5-anton-tadich,6-qile-li,out.test.csv;1025-roser-fernandezcliment,2-joachim-sodequist,out.small.csv";

int main(int argc, char *argv[]) {
    Graph g;

    std::fstream candidates_stream;
    candidates_stream.open(argv[1], std::ios::in);
    std::string candidates_input;
    std::vector<std::string> candidate_lines;

    while (getline(candidates_stream, candidates_input)) {
        candidate_lines.push_back(candidates_input);
    }

    std::vector<std::vector<std::string>> candidates;

    for (auto &line : candidate_lines) {
        std::vector<std::string> current_candidates;
        std::string current_candidate;

        for (auto &c : line) {
            if (c == ',') {
                current_candidates.push_back(current_candidate);
                current_candidate.clear();
            } else {
                current_candidate.push_back(c);
            }
        }

        if (current_candidates.size() > 0) {
            current_candidates.push_back(current_candidate);
        }

        candidates.push_back(current_candidates);
    }

    printf("Candidates loaded, %ld sets\n", candidates.size());

    {
        // The file with the edges - the small version is packaged with the
        // repository for testing
        io::CSVReader<2> in("edges.fixed.csv");
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

    for (auto &candidates : candidates) {
        std::chrono::steady_clock::time_point begin =
            std::chrono::steady_clock::now();

        std::ofstream out_file;
        std::string file_name = candidates.at(candidates.size() - 1);
        out_file.open(candidates.at(candidates.size() - 1), std::ios::out);
        candidates.pop_back();

        std::mutex cout_mutex;
        std::vector<std::thread> threads;
        std::size_t THREAD_BATCH_SIZE = candidates.size() / THREAD_COUNT;

        for (std::uint8_t thread_n = 0; thread_n < THREAD_COUNT; ++thread_n) {
            threads.push_back(std::thread([&g,
                                           &cout_mutex,
                                           &out_file,
                                           thread_n,
                                           THREAD_BATCH_SIZE,
                                           candidates] {
                for (auto &id : std::vector<std::string>(
                         candidates.begin() + thread_n * THREAD_BATCH_SIZE,
                         thread_n == THREAD_COUNT - 1 ? candidates.end()
                                                      : candidates.begin() +
                                 (thread_n + 1) * THREAD_BATCH_SIZE
                     )) {
                    auto person_id = id.substr(0, id.find('-'));

                    auto distances =
                        g.get_distance_v(id, candidates, {person_id});

                    std::lock_guard<std::mutex> lock(cout_mutex);
                    out_file << id << ",";
                    for (auto &d : distances) {
                        out_file << d << ',';
                    }
                    out_file << std::endl;
                }
            }));
        }

        for (auto &t : threads) {
            t.join();
        }

        out_file.close();

        std::chrono::steady_clock::time_point end =
            std::chrono::steady_clock::now();

        std::cout << "(" << file_name << ") Duration: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(
                         end - begin
                     )
                         .count()
                  << "ms" << std::endl;
    }

    return 0;
}
