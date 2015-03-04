#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include "data/billing.h"

const int kBufSize = 200;

void LoadKmBilling::ReadInputData(std::istream &is, unsigned num_region) {
    char buffer[kBufSize];
    double n;

    is  >> n;
    km_rate = static_cast<unsigned>(ceil(n*1000));

    is >> n;
    full_load_value = static_cast<unsigned>(n*100);

    is.getline(buffer, kBufSize);

    load_cost.resize(num_region);

    for (int i = 0; i < load_cost.size(); ++i) {
      is  >> n;
      load_cost[i] = static_cast<unsigned>(ceil(n*1000));
  }

  is.getline(buffer, kBufSize);
}

void VarLoadBilling::ReadInputData(std::istream &is, unsigned num_region) {
    char buffer[kBufSize];
    double n;
    unsigned num_range;

    is >> num_range;
    levels.resize(num_range-1);
    for (int i = 0; i < levels.size(); ++i) {
        is >>  levels[i];  // in kg
    }
    is.getline(buffer, kBufSize);

    load_cost.resize(num_region);

    for (int i = 0; i < num_region; ++i) {
        load_cost[i].resize(num_range);
        is.getline(buffer, kBufSize, ';');
        std::string s = buffer;
        istringstream instr(s);
        for (int j = 0; j < num_range; ++j) {
            instr  >> n;
            load_cost[i][j] = static_cast<unsigned>(ceil(n*1000));
        }
    }
    is.getline(buffer, kBufSize);
}

void LoadBilling::ReadInputData(std::istream &is, unsigned num_region) {
    char buffer[kBufSize];
    double n;

    load_cost.resize(num_region);

    for (int i = 0; i < load_cost.size(); ++i) {
      is  >> n;
      load_cost[i] = static_cast<unsigned>(ceil(n*1000));
    }

    is.getline(buffer, kBufSize);
}
