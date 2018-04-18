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
//std::pair<std::vector<int>, int> bestPlan({},imax);
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
    //TODO: remove this function? or if it is used, get the total number of edges from somewhere or something
    cardStat stats {};
    //out= distinct vertices, paths = distinct edges = total edges, in = distinct vertices
    stats.noPaths = g->totalEdges;
    for(int i = 0; i < g->getNoLabels(); i++){
        auto it = g->edges[i].begin();
        auto sourceStart = -1;
        auto sourceEnd = -1;
        auto end = -1;
        while(it != g->edges[i].end()){
            if(sourceStart != it->first){
                sourceStart = it->first;
                stats.noOut++;
            }
            if(sourceEnd != it->second){
                sourceEnd = it->second;
                stats.noIn++;
            }
            it++;
        }
    }
    /*
    for(int source = 0; source < g->getNoVertices(); source++) {
        if(!g->adj[source].empty()) stats.noOut++;
    }

    stats.noPaths = g->getNoDistinctEdges();

    for(int target = 0; target < g->getNoVertices(); target++) {
        if(!g->reverse_adj[target].empty()) stats.noIn++;
    }
    */
    return stats;
}

std::shared_ptr<SimpleGraph> SimpleEvaluator::project(uint32_t projectLabel, bool inverse, std::shared_ptr<SimpleGraph> &in) {

    auto out = std::make_shared<SimpleGraph>(in->getNoVertices());
    out->setNoLabels(in->getNoLabels());
    out->labelsToJoinForward.push_back(projectLabel);
    for(auto e: in->edges[projectLabel]){
        out->addEdge(e.first, e.second, projectLabel);
    }
    for(auto e: in->reversedEdges[projectLabel]){
        out->addReverseEdge(e.first, e.second, projectLabel, true);
    }
    return out;
}

std::shared_ptr<SimpleGraph> SimpleEvaluator::join(std::shared_ptr<SimpleGraph> &left, std::shared_ptr<SimpleGraph> &right) {

    auto out = std::make_shared<SimpleGraph>(left->getNoVertices());
    out->setNoLabels(std::max(left->getNoLabels(), right->getNoLabels()));
    for(int labelLeft = 0; labelLeft < left->getNoLabels(); ++labelLeft){
        for(int labelRight = 0; labelRight < right->getNoLabels(); ++labelRight) {
            auto leftIt = left->edges[labelLeft].begin();
            auto leftEnd = left->edges[labelLeft].end();
            auto rightIt = right->reversedEdges[labelRight].begin();
            auto rightEnd = right->reversedEdges[labelRight].end();
            while(leftIt != leftEnd && rightIt != rightEnd){
                auto l1 = leftIt->second;
                auto l2 = rightIt->first;
                if(leftIt->second < rightIt->first){
                    leftIt++;
                }
                else if(leftIt->second > rightIt->first){
                    rightIt++;
                }
                else if(rightIt-> first == leftIt->second){
                    //join
                    uint32_t node = leftIt->second;
                    while(leftIt != leftEnd){
                        if(leftIt->second != node)
                            break;
                        out->addEdge(leftIt->first, leftIt->second, labelLeft);
                        out->addReverseEdge(leftIt->first, leftIt->second, labelLeft, false);
                        leftIt++;
                    }
                    while(rightIt != rightEnd){
                        if(rightIt->first != node)
                            break;
                        out->addEdge(rightIt->first, rightIt->second, labelRight);
                        out->addReverseEdge(rightIt->first, rightIt->second, labelRight, false);
                        rightIt++;
                    }

                }
            }
        }
    }
    for(int i = 0; i < out->getNoLabels(); i++){
        out->sortReversedEdges(i);
        //remove duplicates?
    }
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

    /* if(q->isConcat()) {

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
    } */

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
std::pair<std::vector<std::string>, int> SimpleEvaluator::findBestPlan(std::pair<std::vector<std::string>, int> plan){
    if(plan.second != imax){
        return plan;
    }
    if(plan.first.size() == 1){
        //calculate cost
        plan.second = costs[plan.first[0]];
        return plan;
    }
    else{
        auto subsets = getAllSubsets(plan.first);
        for(std::vector<std::string> subset: subsets){
            if(subset.size() > 0) {
                std::vector<std::string> s = plan.first;
                for (auto j: subset) {
                    auto i = std::find(s.begin(), s.end(), j);
                    if (i != s.end()) {
                        s.erase(i);
                    }
                }
                if (s.size() != 0){ //this means s == subset
                    auto p1 = findBestPlan(std::pair<std::vector<std::string>, int>(subset, imax));
                    auto p2 = findBestPlan(std::pair<std::vector<std::string>, int>(s, imax));
                    //estimate join cost
                    int p12 = p1.second * p2.second;
                    int cost = p1.second + p2.second + std::min(p12/p1.second,p12/p2.second);
                    if(cost < plan.second){
                        //TODO: refine this
                        std::string sid1 = "(" + std::to_string(sid) + ")";
                        sid++;
                        std::string sid2 = "(" + std::to_string(sid) + ")";
                        std::string job1 = sid1 + "--";
                        std::string job2 = sid2 + "-";
                        for(std::string x: p1.first){
                            job1+=(x + "-");
                        }
                        for(std::string x: p2.first){
                            job2+=(x + "-");
                        }
                        std::string join = "join between" + sid1 + sid2;
                        sid++;


                        std::pair<std::vector<std::string>,int> result;
                        result.second = cost;
                        result.first.push_back(job1);
                        result.first.push_back(job2);
                        result.first.push_back(join);


                        /*result.first = p1.first;
                        result.first.insert(std::end(result.first), std::begin(p2.first), std::end(p2.first));
                        std::string j1 = "---";
                        std::string j2 = " with ";
                        for(std::string x: p1.first){
                            j1+=x;
                        }
                        for(std::string x: p2.first){
                            j2+=x;
                        }
                        j2+="---";
                        result.first.clear();
                        result.first.push_back(j1.append(j2));*/
                        return result;
                    }
                    return plan;
                }
            }
        }
    }
}

std::vector<std::vector<std::string>> SimpleEvaluator::getAllSubsets(std::vector<std::string> plan)
{
    std::vector<std::vector<std::string>> subsets;
    int n = plan.size();
    for (int i = 0; i < (int) pow(2, n); i++)
    {
        std::vector<std::string> sset;

        // consider each element in the set
        for (int j = 0; j < n; j++)
        {
            // Check if jth bit in the i is set. If the bit
            // is set, we consider jth element from set
            if ((i & (1 << j)) != 0) {
                sset.push_back(plan[j]);
            }
        }

        // if subset is encountered for the first time
        // If we use set<string>, we can directly insert
        if (std::find(subsets.begin(), subsets.end(), sset) == subsets.end()){
            subsets.push_back(sset);
        }
    }
    return subsets;
}

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
    /*
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
*/

    auto res = evaluate_aux(query);
    return SimpleEvaluator::computeStats(res);
}