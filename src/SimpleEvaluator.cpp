//
// Created by Nikolay Yakovets on 2018-02-02.
//

#include <regex>
#include "../include/SimpleEvaluator.h"
#include "../include/SimpleEstimator.h"
#include <iterator>
#include <vector>
#include <initializer_list>
#include <sstream>
#include <set>

std::regex dirLabel (R"((\d+)\+)");
std::regex invLabel (R"((\d+)\-)");
int imax = std::numeric_limits<int>::max();
int sid = 0;

std::map<std::string, int> costs;

SimpleEvaluator::SimpleEvaluator(std::shared_ptr<SimpleGraph> &g) {

    // works only with SimpleGraph
    graph = g;
    est = nullptr; // estimator not attached by default
    total_tuples = new uint32_t[graph->getNoLabels()];
}

void SimpleEvaluator::attachEstimator(std::shared_ptr<SimpleEstimator> &e) {
    est = e;
}

void SimpleEvaluator::prepare() {

    // if attached, prepare the estimator
    if(est != nullptr) est->prepare();

    // prepare other things here.., if necessary
    int labels = graph->getNoLabels();
    for(int i = 0; i < labels; i++){
        costs.insert(std::pair<std::string, int>(std::to_string(i),0));
    }
}

cardStat SimpleEvaluator::computeStats(std::shared_ptr<SimpleGraph> &g) {
    cardStat stats {};
    //nopaths is the total amount of edges
    stats.noPaths = g->getNoDistinctEdges();
    //these are used to calculate the distinct vertices
    //1 means that the vertice is used, 0 means it is not used
    std::vector<uint32_t> outs;
    std::vector<uint32_t> ins;
    outs.resize(g->getNoVertices());
    ins.resize(g->getNoVertices());
    for(int i = 0; i < g->getNoLabels(); i++){
        auto it = g->edges[i].begin();
        while(it != g->edges[i].end()){
            outs[it->first] = 1;
            ins[it->second] = 1;
            it++;
        }
    }
    for(int i = 0; i < ins.size(); ++i){
        stats.noOut += outs[i];
        stats.noIn += ins[i];
    }
    return stats;
}

std::shared_ptr<SimpleGraph> SimpleEvaluator::project(uint32_t projectLabel, bool inverse, std::shared_ptr<SimpleGraph> &in) {

    auto out = std::make_shared<SimpleGraph>(in->getNoVertices());
    out->setNoLabels(in->getNoLabels());
    /*
     * we add the edges backwards if the label is inverse.
     */
    if(inverse){
        for(auto e: in->edges[projectLabel]){
            out->addEdge(e.second, e.first, 0);
        }
        for(auto e: in->edges[projectLabel]){
            out->addReverseEdge(e.second, e.first, 0, true);
        }
    }
    else{
        for(auto e: in->edges[projectLabel]){
            out->addEdge(e.first, e.second, 0);
        }
        for(auto e: in->reversedEdges[projectLabel]){
            out->addReverseEdge(e.first, e.second, 0, true);
        }
    }
    return out;
}

std::shared_ptr<SimpleGraph> SimpleEvaluator::join(std::shared_ptr<SimpleGraph> &left, std::shared_ptr<SimpleGraph> &right) {
    auto out = std::make_shared<SimpleGraph>(left->getNoVertices());
    out->setNoLabels(std::max(left->getNoLabels(), right->getNoLabels()));
    /*
     * 1. compare each list per label from the left side to each list per label on the right side
     */
    uint32_t traceLabel = 0;
    left->sortEdgesOnLabelForward(traceLabel);
    right->sortEdgesOnLabelBackward(traceLabel);

    auto leftIt = left->edges[traceLabel].begin();
    auto leftEnd = left->edges[traceLabel].end();
    auto rightIt = right->reversedEdges[traceLabel].begin();
    auto rightEnd = right->reversedEdges[traceLabel].end();

    while(leftIt != leftEnd && rightIt != rightEnd){

        if(leftIt->second < rightIt->first){
            leftIt++;
        }
        else if(leftIt->second > rightIt->first){
            rightIt++;
        }
            //we have a match, so we add the edges
        else if(rightIt-> first == leftIt->second){
            //it might happen that there are multiple edges with the same endpoints (left) and beginpoints (right). add all of them
            //as long the list is not finished or we hit the next edge

            uint32_t node = leftIt->second;
            while(leftIt != leftEnd){

                if(leftIt->second != node)
                    break;

                auto tempIt = rightIt;
                while(tempIt != rightEnd) {
                    if(tempIt->first != node)
                        break;

                    //add edge twice, forwards and backwards
                    out->addEdge(leftIt->first, tempIt->second, traceLabel);
                    out->addReverseEdge(leftIt->first, tempIt->second, traceLabel, false);
                    tempIt++;
                }
                leftIt++;
            }
        }
    }

    //unfortunately we need to sort everything again
    return out;
}

std::shared_ptr<SimpleGraph> SimpleEvaluator::evaluate_aux(RPQTree *q) {

    // evaluate according to the AST bottom-up

    if(q->isLeaf()) {
        // project out the label in the AST
        std::smatch matches;

        uint32_t label;
        bool inverse;

        if(std::regex_search(q->data, matches, dirLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = false;
        } else if(std::regex_search(q->data, matches, invLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = true;
        } else {
            std::cerr << "Label parsing failed!" << std::endl;
            return nullptr;
        }

        return SimpleEvaluator::project(label, inverse, graph);
    }
    //old code
    if(q->isConcat()) {

        // evaluate the children
        bool decision = query_order[0];
        query_order.erase(query_order.begin());

        if(decision) {
            auto leftGraph = SimpleEvaluator::evaluate_aux(q->left);
            auto rightGraph = SimpleEvaluator::evaluate_aux(q->right);

            // join left with right
            return SimpleEvaluator::join(leftGraph, rightGraph);

        } else {
            auto rightGraph = SimpleEvaluator::evaluate_aux(q->right);
            auto leftGraph = SimpleEvaluator::evaluate_aux(q->left);

            // join left with right
            return SimpleEvaluator::join(leftGraph, rightGraph);
        }
    }
/*
    if(q->isConcat()) {

        // evaluate the children
        auto leftGraph = SimpleEvaluator::evaluate_aux(q->left);
        auto rightGraph = SimpleEvaluator::evaluate_aux(q->right);

        // join left with right
        return SimpleEvaluator::join(leftGraph, rightGraph);

    }
    */

    return nullptr;
}

/*
 * TODO: parse result to query
 */

void SimpleEvaluator::planQuery(RPQTree* q) {
    if(q->isLeaf()) {
        // project out the label in the AST
        std::smatch matches;

        uint32_t label;

        if(std::regex_search(q->data, matches, dirLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            query_labels.push_back(label);
        } else if(std::regex_search(q->data, matches, invLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            query_labels.push_back(label);
        } else {
            std::cerr << "Label parsing failed!" << std::endl;
        }
    }

    if(q->isConcat()) {
        // evaluate the children
        SimpleEvaluator::planQuery(q->left);
        SimpleEvaluator::planQuery(q->right);
    }
}

cardStat SimpleEvaluator::evaluate(RPQTree *query) {
    //bestPlan.first.clear();
    //bestPlan.second = imax;
/*
    planQuery(query);
    std::vector<std::string> labelsAsInts{};
    for(uint32_t i: query_labels){
        labelsAsInts.push_back(std::to_string((int) i));
    }
    std::pair<std::vector<std::string>, int> input(labelsAsInts, imax);
    auto plan = findBestPlan(input);
    query_labels.clear();
    return cardStat{1,1,1};
*/

    uint32_t size_query[query_labels.size() - 1];
    for(int i = 0; i < query_labels.size() - 1; i++) {
        size_query[i] = total_tuples[query_labels[i]] * total_tuples[query_labels[i]];
        std::cout << "Size: " << size_query[i] << std::endl;
    }
    for(int i = 0; i < query_labels.size() - 2; i++) {
        if(size_query[i] > size_query[i+1]) {
            query_order.push_back(false);
        }
    }
    query_order.push_back(true);


    auto res = evaluate_aux(query);
    res->sortEdgesOnLabelBackward(0);
    res->sortEdgesOnLabelForward(0);
    return SimpleEvaluator::computeStats(res);
}