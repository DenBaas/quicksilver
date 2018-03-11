//
// Created by Nikolay Yakovets on 2018-02-01.
//

#include "SimpleGraph.h"
#include "SimpleEstimator.h"
#include <chrono>

using namespace std;
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
    uint32_t* previous_tuples_out = new uint32_t[noLabels] {};
    uint32_t* previous_tuples_in = new uint32_t[noLabels] {};
    for(int i = 0; i < noLabels; i++){
        previous_tuples_in[i] = 0;
        previous_tuples_out[i] = 0;
        total_tuples_in[i] = 0;
        total_tuples_out[i] = 0;
        distinct_tuples_in[i]= 0;
        distinct_tuples_out[i] = 0;
    }

    uint32_t noVertices = graph->getNoVertices();

    uint32_t distinct_out[WIDTH] = {};
    uint32_t distinct_in[WIDTH] = {};
    uint32_t out[WIDTH] = {};
    uint32_t in[WIDTH] = {};

    int tracker = 0;

    for(int i = 0; i < noVertices; i++) {

        if(i > noVertices/10.0 * (tracker + 1)) {
            tracker++;
        }

        if (!graph->adj[i].empty()) {
            distinct_out[tracker]++;
        }
        if (!graph->reverse_adj[i].empty()) {
            distinct_in[tracker]++;
        }

        for (auto labelTarget : graph->adj[i]) {
            total_tuples_out[labelTarget.first]++;
            out[tracker]++;
        }

        for (auto labelTarget : graph->reverse_adj[i]) {
            total_tuples_in[labelTarget.first]++;
            in[tracker]++;
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

    correction = 0;
    for(int i = 0; i < WIDTH; i++) {
        correction += ((double)distinct_out[i]/distinct_in[i]) / 10;
    }
    //correction = (double)distinct_out[0]/distinct_in[0];


    std::cout << "Sum: " << distinct_out << std::endl;
    std::cout << "Sum 2: " << distinct_in << std::endl;
    std::cout <<  "Total: " << noVertices << std::endl;
    std::cout << "Correction: " << correction << std::endl;
    for(int j = 0; j < noLabels; j++) {
        cout << j << "th noOut: " << distinct_tuples_out[j] << '\n';
        cout << j << "th label: " << total_tuples_out[j] << '\n';
        cout << j << "th label: " << total_tuples_in[j] << '\n';
        cout << j << "th noIn: " << distinct_tuples_in[j] << '\n';
    }
    /*
    for(int i = 0; i < WIDTH; i++) {
        std::cout << i << " out: " << out[i] << std::endl;
        std::cout << i << " distinct out: " << distinct_out[i] << std::endl;

        std::cout << i << " in: " << in[i] << std::endl;
        std::cout << i << " distinct in: " << distinct_in[i] << std::endl;
    }*/

    delete[] previous_tuples_out;
    delete[] previous_tuples_in;
}

cardStat SimpleEstimator::estimate(RPQTree *q) {

    // perform your estimation here

    // evaluate according to the AST bottom-up
    // project out the label in the AST

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

        // double outVertices = graph->getNoVertices();

        // intersection estimation
        uint32_t tr = leftGraph.noPaths;
        uint32_t ts = rightGraph.noPaths;
        uint32_t intersect = min(tr, ts)/2;


        // join estimation from the slides, R union S
        uint32_t vry = leftGraph.noOut;
        uint32_t vsy = rightGraph.noIn;
        uint32_t trts = leftGraph.noPaths * rightGraph.noPaths;
        uint32_t paths = (uint32_t)(min(trts/vsy,trts/vry)); //* correction);

        uint32_t outNodes = (uint32_t )(vry / ((double)paths/leftGraph.noPaths));
        uint32_t inNodes = (uint32_t )(vsy / ((double)paths/leftGraph.noPaths));

        std::cout << '\n' << outNodes;

        return cardStat{outNodes, paths, inNodes};
    }

    return cardStat {0, 0, 0};
}

SimpleEstimator::~SimpleEstimator() {
    delete[] total_tuples_out;
    delete[] total_tuples_in;
    delete[] distinct_tuples_in;
    delete[] distinct_tuples_out;
}