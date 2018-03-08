//
// Created by basva on 7-3-2018.
//

#ifndef QUICKSILVER_EQUIWIDTHHISTOGRAM_H
#define QUICKSILVER_EQUIWIDTHHISTOGRAM_H


#include <cstdint>
#include <utility>
#include <vector>

class EquiWidthHistogram {
public:
    EquiWidthHistogram();
    ~EquiWidthHistogram();

    void fillHistogram(std::vector<std::vector<std::pair<uint32_t,uint32_t>>>);
    void init(int size);

    uint32_t* data;
};


#endif //QUICKSILVER_EQUIWIDTHHISTOGRAM_H
