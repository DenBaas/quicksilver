//
// Created by Nikolay Yakovets on 2018-02-01.
//

#include "SimpleGraph.h"
#include "SimpleEstimator.h"
#include <chrono>
#include "math.h"

uint32_t noLabels;
double correction;

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
    noVertices = graph->getNoVertices();
    int totalIn = 0;
    int totalOut = 0;
    for(int i = 0; i < noLabels; ++i){
        int distinctOut = 0;
        int distinctIn = 0;
        int total = 0;
        uint32_t lastIdIn = -1;
        uint32_t lastIdOut = -1;
        for(auto it = graph->edges[i].begin(); it != graph->edges[i].end(); ++it) {
            if (it->second != lastIdIn) {
                distinctIn++;
                lastIdIn = it->second;
            }
        }
        for(auto* re : graph->reversedEdges[i]) {
            if(re->first != lastIdOut){
                distinctOut++;
                lastIdOut = re->first;
            }
        }
        //TODO: totals are the same I guess?
        total_tuples_in[i] = total_tuples_out[i] = graph->noEdges[i];
        distinct_tuples_in[i] = distinctIn;
        distinct_tuples_out[i] = distinctOut;
        totalIn += distinctIn;
        totalOut += distinctOut;
    }
    //TODO: make this available per label or something?
    correction = (double)totalOut/totalIn;

    /*
    std::cout << "Distinct in:" << std::endl;
    for (int i = 0; i < noLabels; i++) {
        std::cout << "Label " << i << ": " << distinct_tuples_in[i] << std::endl;
    }

    std::cout << "Distinct out:" << std::endl;
    for (int i = 0; i < noLabels; i++) {
        std::cout << "Label " << i << ": " << distinct_tuples_out[i] << std::endl;
    }
    */
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