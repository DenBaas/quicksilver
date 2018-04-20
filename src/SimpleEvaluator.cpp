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

struct parseQuery {
    std::string s;
    std::string path;
    std::string t;

    void print() {
        std::cout << s << ", " << path << ", " << t << std::endl;
    }
};

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
    if(est != nullptr) {
        est->prepare();
    }

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

/*
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
*/
    if(q->isConcat()) {

        // evaluate the children
        auto leftGraph = SimpleEvaluator::evaluate_aux(q->left);
        auto rightGraph = SimpleEvaluator::evaluate_aux(q->right);

        // join left with right
        return SimpleEvaluator::join(leftGraph, rightGraph);
    }



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
        bool inverse;

        if(std::regex_search(q->data, matches, dirLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            cardStat cStat = cardStat{est->distinct_tuples_out[label], est->total_tuples_in[label], est->distinct_tuples_in[label]};
            query_labels.push_back(std::pair<uint32_t,cardStat>(label, cStat));
            inversed_list.push_back(false);
        } else if(std::regex_search(q->data, matches, invLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            cardStat cStat = cardStat{est->distinct_tuples_in[label], est->total_tuples_in[label], est->distinct_tuples_out[label]};
            query_labels.push_back(std::pair<uint32_t,cardStat>(label, cStat));
            inversed_list.push_back(true);
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

std::vector<uint32_t> SimpleEvaluator::findBestPlan(std::vector<std::pair<uint32_t, cardStat>> query) {
    cardStat stat {};
    uint32_t nPaths = -1;
    uint32_t tracker;
    if(query.size() == 1) {
        return std::vector<uint32_t> (0);
    } else {
        for (int i = 1; i < query.size(); i++) {
            uint32_t vry = query[i - 1].second.noOut;
            uint32_t vsy = query[i].second.noIn;
            uint32_t trts = query[i - 1].second.noPaths * query[i].second.noPaths;

            uint32_t paths = (uint32_t) (std::min(trts / vsy, trts / vry)); // * correction
            vry = std::min(vry, paths);
            vsy = std::min(vsy, paths);

            if (nPaths == -1) {
                stat = cardStat{vry, paths, vsy};
                nPaths = paths;
                tracker = i;
            } else if (paths < nPaths) {
                stat = cardStat{vry, paths, vsy};
                nPaths = paths;
                tracker = i;
            }
        }

        std::vector<std::pair<uint32_t, cardStat>> subquery = query;
        subquery.erase(subquery.begin()+ tracker - 1);
        subquery.at(tracker - 1).second = stat;
        std::vector<uint32_t> trackQuery = findBestPlan(subquery);
        trackQuery.push_back(tracker);

        return trackQuery;

    }
}

cardStat SimpleEvaluator::evaluate(RPQTree *query) {

    query_labels.clear();
    inversed_list.clear();
    planQuery(query);
    std::vector<uint32_t> bestPlan = findBestPlan(query_labels);

    std::vector<std::string> plan;
    std::string symbol;
    for(int i = 0; i < query_labels.size(); i++) {
        if(inversed_list[i] == true) {
            symbol = "-";
        } else {
            symbol = "+";
        }
        plan.push_back(std::to_string(query_labels[i].first) + symbol);
    }

    while(bestPlan.size() > 0) {
        uint32_t n = bestPlan.back();
        bestPlan.pop_back();
        std::string spot = plan[n];
        plan[n - 1] = "(" + plan[n - 1] + "/" + spot + ")";
        plan.erase(plan.begin() + n);
    }

    plan[0] = "*, " + plan[0] + ", *";

    // make a new query tree
    std::regex edgePat (R"((.+),(.+),(.+))");

    std::smatch matches;
    parseQuery p;
    std::string line = plan[0];

    // match edge data
    if(std::regex_search(line, matches, edgePat)) {
        auto s = matches[1];
        auto path = matches[2];
        auto t = matches[3];

        p = parseQuery{s, path, t};
    }

    RPQTree *pTree = RPQTree::strToTree(p.path);
    std::cout << "\nParsed pTree: ";
    pTree->print();

    auto res = evaluate_aux(pTree);
    res->sortEdgesOnLabelBackward(0);
    res->sortEdgesOnLabelForward(0);
    return SimpleEvaluator::computeStats(res);
}