//
// Created by Nikolay Yakovets on 2018-02-01.
//

#ifndef QS_SIMPLEESTIMATOR_H
#define QS_SIMPLEESTIMATOR_H

#include "Estimator.h"
#include "SimpleGraph.h"

class SimpleEstimator : public Estimator {

    std::shared_ptr<SimpleGraph> graph;
    int* total_tuples_out;
    int* distinct_tuples_out;

    int* total_tuples_in;
    int* distinct_tuples_in;


public:
    explicit SimpleEstimator(std::shared_ptr<SimpleGraph> &g);
    ~SimpleEstimator();

    void prepare() override ;
    cardStat estimate(RPQTree *q) override ;
};


#endif //QS_SIMPLEESTIMATOR_H
