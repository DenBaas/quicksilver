//
// Created by Nikolay Yakovets on 2018-02-01.
//

#include "SimpleGraph.h"
#include "SimpleEstimator.h"

using namespace std;

SimpleEstimator::SimpleEstimator(std::shared_ptr<SimpleGraph> &g){

    // works only with SimpleGraph
    graph = g;
    total_tuples_out = new int[graph->getNoLabels()] {};
    distinct_tuples_out = new int[graph->getNoLabels()] {};

    total_tuples_in = new int[graph->getNoLabels()] {};
    distinct_tuples_in = new int[graph->getNoLabels()] {};

    previous_tuples_out = new int[graph->getNoLabels()] {};
    previous_tuples_in = new int[graph->getNoLabels()] {};
}

void SimpleEstimator::prepare() {

    // do your prep here
    int noLabels = graph->getNoLabels();
    int noVertices = graph->getNoVertices();

    int sum = 0;
    for(int i = 0; i < noVertices; i++) {
        if (graph->reverse_adj[i].empty() && graph->adj[i].empty()) {
            sum++;
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

    std::cout << "Sum: " << sum << '\n' << std::endl;
    for(int j = 0; j < noLabels; j++) {
        cout << j << "th noOut: " << distinct_tuples_out[j] << '\n';
        cout << j << "th label: " << total_tuples_out[j] << '\n';
        cout << j << "th label: " << total_tuples_in[j] << '\n';
        cout << j << "th noIn: " << distinct_tuples_in[j] << '\n';
    }
}

cardStat SimpleEstimator::estimate(RPQTree *q) {

    // perform your estimation here

    // evaluate according to the AST bottom-up

    // project out the label in the AST
    std::regex directLabel (R"((\d+)\+)");
    std::regex inverseLabel (R"((\d+)\-)");
    std::smatch matches;
    bool inverse;
    uint32_t label;
    if(q->isLeaf()) {

        if(std::regex_search(q->data, matches, directLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            return cardStat{(uint32_t)(distinct_tuples_out[label]), (uint32_t)total_tuples_out[label], (uint32_t)(distinct_tuples_in[label])};

            //uint32_t out = outVerticesHistogram->data[label];
            //return cardStat{out, (uint32_t) num_of_edges[label], out};
        } else if(std::regex_search(q->data, matches, inverseLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            return cardStat{(uint32_t)(distinct_tuples_in[label]), (uint32_t)total_tuples_in[label], (uint32_t)(distinct_tuples_out[label])};
            //return  cardStat{out, (uint32_t) num_of_edges[label], out};
        } else {
            std::cerr << "Label parsing failed!" << std::endl;
            return cardStat{0, 0, 0};
        }
    }

    if(q->isConcat()) {

        // evaluate the children
        auto leftGraph = SimpleEstimator::estimate(q->left);
        auto rightGraph = SimpleEstimator::estimate(q->right);
        double outVertices = graph->getNoVertices();

        // join left with right
        //double ratio_out_left = (double)leftGraph.noPaths/leftGraph.noOut;
        //double ratio_in_left = (double)leftGraph.noPaths/leftGraph.noIn;
        double ratio_left_right = (double)rightGraph.noOut/outVertices;
        double paths_per = (double)rightGraph.noPaths/rightGraph.noOut;


        uint32_t noOut = leftGraph.noOut;
        uint32_t noIn = rightGraph.noIn;
        // uint32_t noPaths = leftGraph.noPaths * ratio_left_right * paths_per;
        uint32_t noPaths = min(leftGraph.noPaths*rightGraph.noPaths/leftGraph.noIn, leftGraph.noPaths*rightGraph.noPaths/rightGraph.noOut);

        return cardStat{noOut, noPaths, noIn};
    }

    return cardStat {0, 0, 0};
}

SimpleEstimator::~SimpleEstimator() {
    delete[] total_tuples_out;
    delete[] total_tuples_in;
    delete[] distinct_tuples_in;
    delete[] distinct_tuples_out;
    delete[] previous_tuples_out;
    delete[] previous_tuples_in;
}