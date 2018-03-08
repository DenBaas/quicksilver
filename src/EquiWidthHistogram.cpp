#include "EquiWidthHistogram.h"

EquiWidthHistogram::EquiWidthHistogram() {

}

EquiWidthHistogram::~EquiWidthHistogram() {
    delete(data);
}

void EquiWidthHistogram::init(int size) {
    data = new uint32_t[size]{};
    for (int i = 0; i < size; ++i) {
        data[i] = 0;
    }
}

void EquiWidthHistogram::fillHistogram(std::vector<std::vector<std::pair<uint32_t, uint32_t>>> input) {
    for(std::vector<std::pair<uint32_t, uint32_t>> edges: input){
        for(std::pair<uint32_t , uint32_t > pair: edges){
            ++data[pair.first];
        }
    }
}