//
// Created by Nikolay Yakovets on 2018-02-01.
//

#include "SimpleGraph.h"
#include "SimpleEstimator.h"
#include <chrono>

uint32_t noLabels;


constexpr int WIDTH = 10;

std::regex inverseLabel (R"((\d+)\-)");
std::regex directLabel (R"((\d+)\+)");

SimpleEstimator::SimpleEstimator(std::shared_ptr<SimpleGraph> &g){

    // works only with SimpleGraph
    graph = g;
    noLabels = graph->getNoLabels();

    total_tuples_out = new uint32_t[noLabels] {};
    distinct_tuples_out = new uint32_t[noLabels] {};
    total_tuples_in = new uint32_t[noLabels] {};
    distinct_tuples_in = new uint32_t[noLabels] {};

}

void SimpleEstimator::prepare() {
    // do your prep here
    uint32_t* previous_tuples_out = new uint32_t[noLabels] {};
    uint32_t* previous_tuples_in = new uint32_t[noLabels] {};

    uint32_t noVertices = graph->getNoVertices();

    int tracker = 0;
    uint32_t distinct_out = 0;
    uint32_t distinct_in = 0;

    for(int i = 0; i < noVertices; i++) {

        if(i > noVertices/10.0 * (tracker + 1)) {
            tracker++;
        }

        if (!graph->adj[i].empty()) {
            distinct_out++;
        }
        if (!graph->reverse_adj[i].empty()) {
            distinct_in++;
        }

        for (auto labelTarget : graph->adj[i]) {
            total_tuples_out[labelTarget.first]++;
        }

        for (auto labelTarget : graph->reverse_adj[i]) {
            total_tuples_in[labelTarget.first]++;
        }

        for (int j = 0; j < noLabels; j++) {
            if (total_tuples_out[j] != previous_tuples_out[j]) {
                distinct_tuples_out[j]++;
                previous_tuples_out[j] = total_tuples_out[j];
            }

            if (total_tuples_in[j] != previous_tuples_in[j]) {
                distinct_tuples_in[j]++;
                previous_tuples_in[j] = total_tuples_in[j];
            }
        }
    }

    correction = (double)distinct_out/distinct_in;

    delete[] previous_tuples_out;
    delete[] previous_tuples_in;
}

cardStat SimpleEstimator::estimate(RPQTree *q) {

    // perform your estimation here

    std::smatch matches;
    uint32_t label;

    if(q->isLeaf()) {

        if(std::regex_search(q->data, matches, directLabel)) {
            label = std::stoul(matches[1]);
            return cardStat{(distinct_tuples_out[label]), total_tuples_out[label], (distinct_tuples_in[label])};
        } else if(std::regex_search(q->data, matches, inverseLabel)) {
            label = std::stoul(matches[1]);
            return cardStat{(distinct_tuples_in[label]), total_tuples_in[label], (distinct_tuples_out[label])};
        } else {
            std::cerr << "Label parsing failed!" << std::endl;
            return cardStat{0, 0, 0};
        }
    }

    if(q->isConcat()) {
        // evaluate the children
        auto leftGraph = SimpleEstimator::estimate(q->left);
        auto rightGraph = SimpleEstimator::estimate(q->right);

        // join estimation from the slides, R join S
        uint32_t vry = leftGraph.noOut;
        uint32_t vsy = rightGraph.noIn;
        uint32_t trts = leftGraph.noPaths * rightGraph.noPaths;
        uint32_t paths = (uint32_t)(std::min(trts/vsy,trts/vry) * correction);

        return cardStat{vry, paths, vsy};
    }

    return cardStat {0, 0, 0};
}

SimpleEstimator::~SimpleEstimator() {
    delete[] total_tuples_out;
    delete[] total_tuples_in;
    delete[] distinct_tuples_in;
    delete[] distinct_tuples_out;
}