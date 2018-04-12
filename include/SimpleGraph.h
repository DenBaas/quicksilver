//
// Created by Nikolay Yakovets on 2018-01-31.
//

#ifndef QS_SIMPLEGRAPH_H
#define QS_SIMPLEGRAPH_H

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>
#include <regex>
#include <fstream>
#include "Graph.h"
#include "forward_list"

class SimpleGraph : public Graph {
public:
    //beginpoint-label-endpoint
    std::vector<std::vector<std::pair<uint32_t,uint32_t>>> adj;
    std::vector<std::vector<std::pair<uint32_t,uint32_t>>> reverse_adj; // vertex adjacency list


    //NEW DATASTRUCTURE
    /*
     *
     *
     * edges:           each entry in this vector corespondents to a label. Each entry contains the begin and endpoints of the labels.
     *                  The edges will be sorted on the endpoints, because the endpoints are needed for sort-merge join.
     *                  There is a separate sorted list with pointers for merge join stored in reversedIndexes
     * reversedIndexes: each entry in this vector corespondents to a label. Each entry is a list of pointers where each
     *                  pointer points to an edge pair. It is sorted on the begin points.
     */
    /*
     * TODO: add this to the report: https://stackoverflow.com/questions/2209224/vector-vs-list-in-stl#2209564
     * TODO: the pair is 8 bytes and the pointer to the pair only 4 bytes.
     *
     * TODO: forward list?
     */
    //std::vector<std::vector<std::pair<uint32_t, uint32_t>>> edges;
    //std::vector<std::vector<std::pair<uint32_t, uint32_t>*>> reversedIndexes;
    //std::vector<std::vector<std::pair<uint32_t, uint32_t>>> reversedIndexes;
    std::vector<std::forward_list<std::pair<uint32_t, uint32_t>>> edges2;
    std::vector<std::forward_list<std::pair<uint32_t, uint32_t>*>> reversedIndexes2;
    std::vector<uint32_t> noEdges;

protected:
    uint32_t V;
    uint32_t L;

public:

    /*
     * TODO: extract number of edges
     * TODO: what are distinct edges? (never used)
     *
     * TODO: change rev_adj to pointers
     */

    SimpleGraph() : V(0), L(0) {};
    ~SimpleGraph() = default;
    explicit SimpleGraph(uint32_t n);

    uint32_t getNoVertices() const override ;
    uint32_t getNoEdges() const override ;
    uint32_t getNoDistinctEdges() const override ;
    uint32_t getNoLabels() const override ;

    void addEdge(uint32_t from, uint32_t to, uint32_t edgeLabel) override ;
    void readFromContiguousFile(const std::string &fileName) override ;

    //these will be removed
    void setNoVertices(uint32_t n);
    void setNoLabels(uint32_t noLabels);

};

#endif //QS_SIMPLEGRAPH_H
