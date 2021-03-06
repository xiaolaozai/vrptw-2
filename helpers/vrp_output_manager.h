#ifndef _VRP_OUTPUT_MANAGER_H_
#define _VRP_OUTPUT_MANAGER_H_
#include <helpers/OutputManager.hh>
#include <string>
#include "data/route.h"
#include "data/prob_input.h"

typedef RoutePlan ProbOutput;

class VRPOutputManager: public OutputManager<ProbInput, ProbOutput, RoutePlan> {
 public:
    VRPOutputManager(const ProbInput &in, std::string name):
        OutputManager<ProbInput, ProbOutput, RoutePlan>(in, name) { }
    void OutputState(const RoutePlan &st, ProbOutput &out) const { out = st; }
    void InputState(RoutePlan &st, const ProbOutput &out) const { st = out; }
};

#endif
