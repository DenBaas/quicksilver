//
// Created by Nikolay Yakovets on 2018-01-31.
//

#include "../include/SimpleGraph.h"


SimpleGraph::SimpleGraph(uint32_t n)   {
    setNoVertices(n);
}

bool sortEdgeForward(std::pair<uint32_t,uint32_t> i, std::pair<uint32_t,uint32_t> j);
bool sortEdgeBackward(std::pair<uint32_t,uint32_t> i, std::pair<uint32_t,uint32_t> j);

void SimpleGraph::sortEdgesOnLabel(int label) {
    edges[label].sort(sortEdgeForward);
    reversedEdges[label].sort(sortEdgeBackward);
}

uint32_t SimpleGraph::getNoVertices() const {
    return V;
}

void SimpleGraph::setNoVertices(uint32_t n) {
    V = n;
}

uint32_t SimpleGraph::getNoEdges() const {
    return totalEdges;
}

uint32_t SimpleGraph::getNoDistinctEdges() const {
    //TODO: move the code from estimator to here

    uint32_t sum = 0;
    uint32_t prevTarget = -1;
    uint32_t prevSource = -1;
    bool sorted = true;

    for (auto ed : reversedEdges[0]) {
        std::cout << "Edge: " << ed.first << " - " << ed.second << std::endl;
        if (!(prevTarget == ed.second && prevSource == ed.first)) {
            sum++;
            prevTarget = ed.second;
            prevSource = ed.first;
        }
        else{
            sorted = prevTarget <= ed.second && prevSource <= ed.first;
        }
    }
    return sum;
}

uint32_t SimpleGraph::getNoLabels() const {
    return L;
}

void SimpleGraph::setNoLabels(uint32_t noLabels) {
    L = noLabels;
    //edges.resize(noLabels);
    edges.resize(noLabels);
    reversedEdges.resize(noLabels);
    for(int i = 0; i < noLabels; i++){
        endOfForwardEdges.push_back(edges[i].end());
        endOfBackwardEdges.push_back(edges[i].end());
    }
}

/*
 * Used on sorting
 */
bool sortEdgeForward(std::pair<uint32_t,uint32_t> i, std::pair<uint32_t,uint32_t> j){
    return i.second < j.second;
}

bool sortEdgeBackward(std::pair<uint32_t,uint32_t> i, std::pair<uint32_t,uint32_t> j){
    return i.first < j.first;
}

/*
 * It is sorted after insertion
 *
 * nvm about the pointers for now, the rev_adj list is faster to build without pointers. With pointers you have to find the edgeLabel again in the edges, which is O(log n) or O(n) if std::find is used.
 */
void SimpleGraph::addEdge(uint32_t from, uint32_t to, uint32_t edgeLabel) {


    //uint32_t size1 = sizeof(std::pair<uint32_t, uint32_t>); //= 8
    //uint32_t size2 = sizeof(std::pair<uint32_t, uint32_t>*);//= 4
    totalEdges++;
    if(endOfForwardEdges[edgeLabel] != edges[edgeLabel].end()){
        edges[edgeLabel].insert_after(endOfForwardEdges[edgeLabel], std::pair<uint32_t,uint32_t>(from, to));
        endOfForwardEdges[edgeLabel]++;
    }
    else{
        edges[edgeLabel].push_front(std::pair<uint32_t,uint32_t>(from, to));
        endOfForwardEdges[edgeLabel] = edges[edgeLabel].begin();
    }
}

//false means: add in front of list
void SimpleGraph::addReverseEdge(uint32_t from, uint32_t to, uint32_t edgeLabel, bool pushBack) {
    if(!pushBack)
        reversedEdges[edgeLabel].push_front(std::pair<uint32_t,uint32_t>(from, to));
    else{
        if(endOfBackwardEdges[edgeLabel] != edges[edgeLabel].end()){
            reversedEdges[edgeLabel].insert_after(endOfBackwardEdges[edgeLabel], std::pair<uint32_t,uint32_t>(from, to));
            endOfBackwardEdges[edgeLabel]++;
        }
        else{
            reversedEdges[edgeLabel].push_front(std::pair<uint32_t,uint32_t>(from, to));
            endOfBackwardEdges[edgeLabel] = reversedEdges[edgeLabel].begin();
        }
    }

}

void SimpleGraph::readFromContiguousFile(const std::string &fileName) {

    std::string line;
    std::ifstream graphFile { fileName };

    std::regex edgePat (R"((\d+)\s(\d+)\s(\d+)\s\.)"); // subject predicate object .
    std::regex headerPat (R"((\d+),(\d+),(\d+))"); // noNodes,noEdges,noLabels

    // parse the header (1st line)
    std::getline(graphFile, line);
    std::smatch matches;
    if(std::regex_search(line, matches, headerPat)) {
        uint32_t noNodes = (uint32_t) std::stoul(matches[1]);
        uint32_t noLabels = (uint32_t) std::stoul(matches[3]);

        setNoVertices(noNodes);
        setNoLabels(noLabels);
    } else {
        throw std::runtime_error(std::string("Invalid graph header!"));
    }

    // parse edge data
    std::clock_t begin = clock();
    while(std::getline(graphFile, line)) {

        if(std::regex_search(line, matches, edgePat)) {
            uint32_t subject = (uint32_t) std::stoul(matches[1]);
            uint32_t predicate = (uint32_t) std::stoul(matches[2]);
            uint32_t object = (uint32_t) std::stoul(matches[3]);
            addEdge(subject, object, predicate);
            addReverseEdge(subject, object, predicate, false);
        }
    }
    //sort edges
    for(int i = 0; i < L; ++i){
        //edges[i].sort(sortEdgeForward);
        sortEdgesOnLabel(i);
    }
    auto time = double(clock()-begin)/CLOCKS_PER_SEC;
    graphFile.close();

}