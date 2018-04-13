//
// Created by Nikolay Yakovets on 2018-02-01.
//

#ifndef QS_SIMPLEESTIMATOR_H
#define QS_SIMPLEESTIMATOR_H

#include "Estimator.h"
#include "SimpleGraph.h"

class SimpleEstimator : public Estimator {

public:
    std::shared_ptr<SimpleGraph> graph;
    uint32_t* total_tuples_out;
    uint32_t* distinct_tuples_out;

    uint32_t* total_tuples_in;
    uint32_t* distinct_tuples_in;

    uint32_t noVertices;

    explicit SimpleEstimator(std::shared_ptr<SimpleGraph> &g);
    ~SimpleEstimator();

    void prepare() override ;
    cardStat estimate(RPQTree *q) override ;
};


#endif //QS_SIMPLEESTIMATOR_H
