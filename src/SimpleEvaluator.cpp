//
// Created by Nikolay Yakovets on 2018-02-02.
//

#include "SimpleEstimator.h"
#include "SimpleEvaluator.h"

std::regex dirLabel (R"((\d+)\+)");
std::regex invLabel (R"((\d+)\-)");

struct parseQuery {
    std::string s;
    std::string path;
    std::string t;

    void print() {
        std::cout << s << ", " << path << ", " << t << std::endl;
    }
};

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
    uint32_t noVertices = graph->getNoVertices();

    for(int i = 0; i < noVertices; i++) {
        for(auto labelTarget : graph->adj[i]) {
            total_tuples[labelTarget.first]++;
        }
    }

}

cardStat SimpleEvaluator::computeStats(std::shared_ptr<SimpleGraph> &g) {

    cardStat stats {};

    for(int source = 0; source < g->getNoVertices(); source++) {
        if(!g->adj[source].empty()) stats.noOut++;
    }

    stats.noPaths = g->getNoDistinctEdges();

    for(int target = 0; target < g->getNoVertices(); target++) {
        if(!g->reverse_adj[target].empty()) stats.noIn++;
    }

    return stats;
}

std::shared_ptr<SimpleGraph> SimpleEvaluator::project(uint32_t projectLabel, bool inverse, std::shared_ptr<SimpleGraph> &in) {

    auto out = std::make_shared<SimpleGraph>(in->getNoVertices());
    out->setNoLabels(in->getNoLabels());

    if(!inverse) {
        // going forward
        for(uint32_t source = 0; source < in->getNoVertices(); source++) {
            for (auto labelTarget : in->adj[source]) {

                auto label = labelTarget.first;
                auto target = labelTarget.second;

                if (label == projectLabel)
                    out->addEdge(source, target, label);
            }
        }
    } else {
        // going backward
        for(uint32_t source = 0; source < in->getNoVertices(); source++) {
            for (auto labelTarget : in->reverse_adj[source]) {

                auto label = labelTarget.first;
                auto target = labelTarget.second;

                if (label == projectLabel)
                    out->addEdge(source, target, label);
            }
        }
    }

    return out;
}

std::shared_ptr<SimpleGraph> SimpleEvaluator::join(std::shared_ptr<SimpleGraph> &left, std::shared_ptr<SimpleGraph> &right) {

    auto out = std::make_shared<SimpleGraph>(left->getNoVertices());
    out->setNoLabels(1);

    for(uint32_t leftSource = 0; leftSource < left->getNoVertices(); leftSource++) {
        for (auto labelTarget : left->adj[leftSource]) {

            int leftTarget = labelTarget.second;
            // try to join the left target with right source
            for (auto rightLabelTarget : right->adj[leftTarget]) {

                auto rightTarget = rightLabelTarget.second;
                out->addEdge(leftSource, rightTarget, 0);

            }
        }
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

    if(q->isConcat()) {

        // evaluate the children
        auto leftGraph = SimpleEvaluator::evaluate_aux(q->left);
        auto rightGraph = SimpleEvaluator::evaluate_aux(q->right);

        // join left with right
        return SimpleEvaluator::join(leftGraph, rightGraph);
    }

    return nullptr;
}

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
    return SimpleEvaluator::computeStats(res);
}