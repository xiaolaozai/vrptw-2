#include "helpers/vrp_neighborhood_explorer.h"
#include <utils/Random.hh>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include "data/billing.h"
#include "helpers/billing_cost_component.h"

void InsMoveNeighborhoodExplorer::RandomMove(const RoutePlan &rp,
                                             InsMove &mv) const {
    do {
        AnyRandomMove(rp, mv);
    }while (!FeasibleMove(rp, mv));
}

bool InsMoveNeighborhoodExplorer::FeasibleMove(const RoutePlan &rp,
                                               const InsMove &mv) const {
    if (!rp[mv.old_route].size())   // old route is null
        return false;
    if (rp[mv.new_route].IsExcList())
        if (in.OrderGroupVect(mv.order).IsMandatory())
            return false;
    if (mv.new_route == mv.old_route)
        return false;
    return true;
}

void InsMoveNeighborhoodExplorer::AnyRandomMove(const RoutePlan &rp,
                                                InsMove &mv) const {
    mv.old_route = Random::Int(0, rp.size() - 1);
    mv.old_pos = mv.order = 0;
    if (rp[mv.old_route].size()) {
        mv.old_pos = Random::Int(0, rp[mv.old_route].size() - 1);
        mv.order = rp[mv.old_route][mv.old_pos];
    }
    mv.new_route = Random::Int(0, rp.size() - 1);
    mv.new_pos = 0;     // for null routes
    if (rp[mv.new_route].size())
        mv.new_pos = Random::Int(0, rp[mv.new_route].size());
}

void InsMoveNeighborhoodExplorer::FirstMove(const RoutePlan &rp,
                                            InsMove &mv) const {
    assert(rp.size() > 1);
    mv.old_route = 0;
    mv.old_pos = 0;
    mv.order = rp[mv.old_route][mv.old_pos];
    mv.new_route = 1;
    mv.new_pos = 0;
}

bool InsMoveNeighborhoodExplorer::NextMove(const RoutePlan &rp,
                                           InsMove &mv) const {
    bool not_last = true;
    do {
        not_last = AnyNextMove(rp, mv);
    }while(!FeasibleMove(rp, mv));
    return not_last;
}

bool InsMoveNeighborhoodExplorer::AnyNextMove(const RoutePlan &rp,
                                              InsMove &mv) const {
    if (mv.new_pos < rp[mv.new_route].size() - 1) {
        mv.new_pos++;
    } else if (mv.new_route < rp.size() - 1) {
        mv.new_route++;
        mv.new_pos = 0;
    } else {
        if (mv.old_pos < rp[mv.old_route].size() - 1) {
            mv.old_pos++;
        } else if (mv.old_route < rp.size() - 1) {
            mv.old_route++;
            mv.old_pos = 0;
        } else {
            return false;
        }
        mv.order = rp[mv.old_route][mv.old_pos];
        mv.new_route = 0;
        mv.new_pos = 0;
    }
    return true;
}

void
InsMoveNeighborhoodExplorer::MakeMove(RoutePlan &rp,
                                      const InsMove &mv) const {
    rp[mv.old_route].erase(mv.old_pos);
    rp[mv.new_route].insert(mv.new_pos, mv.order);
    // update timetable
    UpdateRouteTimetable(rp.timetable(mv.old_route), rp[mv.old_route]);
    UpdateRouteTimetable(rp.timetable(mv.new_route), rp[mv.new_route]);
}

int
InsMoveNeighborhoodExplorer::DeltaCostFunction(const RoutePlan &rp,
                                               const InsMove &mv) const {
    // ugly reduntant data
    routes.clear();
    routes.push_back(rp[mv.old_route]);
    routes.push_back(rp[mv.new_route]);
    routes[0].erase(mv.old_pos);
    routes[1].insert(mv.new_pos, mv.order);
    timetable.clear();
    timetable.resize(2);
    if (!routes[0].IsExcList())
        UpdateRouteTimetable(timetable[0], rp[mv.old_route]);
    if (!routes[1].IsExcList())
        UpdateRouteTimetable(timetable[1], rp[mv.new_route]);

    return DeltaObjective(rp, mv) + DeltaViolations(rp, mv);
}

int
InsMoveNeighborhoodExplorer::DeltaObjective(const RoutePlan &rp,
                                            const InsMove &mv) const {
    return DeltaDateViolationCost(rp, mv, 30) + DeltaTimeViolationCost(rp, mv, 10)
           + DeltaOptOrderCost(rp, mv, 250) + DeltaTranportationCost(rp, mv);
}

int
InsMoveNeighborhoodExplorer::DeltaViolations(const RoutePlan &rp,
                                             const InsMove &mv) const {
    return DeltaCapExceededCost(rp, mv, 1) + DeltaLateReturnCost(rp, mv, 1);
}

int
InsMoveNeighborhoodExplorer::DeltaDateViolationCost(const RoutePlan &rp,
                                        const InsMove &mv, int weight) const {
    const OrderGroup& o = in.OrderGroupVect(mv.order);
    unsigned old_day = rp[mv.old_route].get_day();
    unsigned new_day = rp[mv.new_route].get_day();
    unsigned d = o.get_demand();
    int delta = d * (o.IsDayFeasible(old_day) - o.IsDayFeasible(new_day));
    return (weight * delta);
}

int
InsMoveNeighborhoodExplorer::DeltaTimeViolationCost(const RoutePlan &rp,
                                        const InsMove &mv, int weight) const {
    int delta = 0;
    if (!rp[mv.old_route].IsExcList())
        delta += DeltaRouteTimeViolation(rp, 0, mv.old_route, mv.old_pos);

    if (!rp[mv.new_route].IsExcList())
        delta += DeltaRouteTimeViolation(rp, 1, mv.new_route, mv.new_pos);

    return delta;
}

// int
// InsMoveNeighborhoodExplorer::DeltaRouteTimeViolation(const RoutePlan &rp,
//                                   int isnew, unsigned route, unsigned pos) {
//     int delta = 0;
//     for (int i = pos; i < rp[route].size(); ++i) {
//         client = in.OrderVect(rp[route][i]).get_client();
//         int duetime = in.FindClient(client).get_due_time();
//         int arrive_time = rp(route, i);
//         if (arrive_time > duetime)
//             delta -= arrive_time - duetime;
//     }
//         // after move
//     for (int i = pos; i < routes[isnew].size(); ++i) {
//         client = in.OrderVect(routes[isnew][i]).get_client();
//         int duetime = in.FindClient(client).get_due_time();
//         if (timetable[isnew][i] > duetime)
//             delta += timetable[isnew][i] - duetime;
//     }
//     return delta;
// }

int
InsMoveNeighborhoodExplorer::DeltaOptOrderCost(const RoutePlan &rp,
                                        const InsMove &mv, int weight) const {
    int delta = 0;
    const OrderGroup& o = in.OrderGroupVect(mv.order);
    if (!o.IsMandatory()) {
        if (rp[mv.old_route].IsExcList())
            delta -= o.get_demand();
        else if (rp[mv.new_route].IsExcList())
            delta += o.get_demand();
    }
    return (weight * delta);
}

int
InsMoveNeighborhoodExplorer::DeltaTranportationCost(const RoutePlan &rp,
                                                    const InsMove &mv) const {
    int delta = 0;
    const Billing *cr = in.FindBilling(rp[mv.old_route].get_vehicle());
    if (!rp[mv.old_route].IsExcList())
        delta += cr->GetCostComponent().Cost(routes[0]) - 
                 cr->GetCostComponent().Cost(rp[mv.old_route]);
    cr = in.FindBilling(rp[mv.new_route].get_vehicle());
    if (!rp[mv.new_route].IsExcList())
        delta += cr->GetCostComponent().Cost(routes[1]) -
                 cr->GetCostComponent().Cost(rp[mv.new_route]);
    return delta;
}

int
InsMoveNeighborhoodExplorer::DeltaCapExceededCost(const RoutePlan &rp,
                                        const InsMove &mv, int weight) const {
    int delta = 0;
    unsigned vehicle_cap = 0, route_demand = 0;
    unsigned order_demand = in.OrderGroupVect(mv.order).get_demand();
    if (!rp[mv.old_route].IsExcList()) {
        vehicle_cap = in.VehicleVect(rp[mv.old_route].get_vehicle()).get_cap();
        route_demand = rp[mv.old_route].demand();
        if (route_demand > vehicle_cap) {
            if (route_demand - order_demand > vehicle_cap)
                delta -= order_demand;
            else
                delta -= route_demand - vehicle_cap;
        }
    }
    if (!rp[mv.new_route].IsExcList()) {
        vehicle_cap = in.VehicleVect(rp[mv.new_route].get_vehicle()).get_cap();
        route_demand = rp[mv.new_route].demand();
        if (route_demand > vehicle_cap)
            delta += order_demand;
        else if (route_demand + order_demand > vehicle_cap)
            delta += route_demand + order_demand - vehicle_cap;
    }
    return (weight * delta);
}

int
InsMoveNeighborhoodExplorer::DeltaLateReturnCost(const RoutePlan &rp,
                                        const InsMove &mv, int weight) const {
    int delta = 0;
    int shutdown_time = in.get_return_time() + 3600;  // plus 1 hour
    if (rp(mv.old_route, mv.old_pos) > shutdown_time)
        --delta;
    for (unsigned j = mv.old_pos; j < routes[0].size(); ++j) {
        if (rp(mv.old_route, j + 1) > shutdown_time)
            --delta;
        if (timetable[0][j] > shutdown_time)
            ++delta;
    }
    if (timetable[1][mv.new_pos] > shutdown_time)
        ++delta;
    for (unsigned j = mv.new_pos; j < rp[mv.new_route].size(); ++j) {
        if (rp(mv.new_route, j) > shutdown_time)
            --delta;
        if (timetable[1][j + 1] > shutdown_time)
            ++delta;
    }
    return (weight * delta);
}


// Inter Route Swap Implementation

void InterSwapNeighborhoodExplorer::RandomMove(const RoutePlan &rp,
                                             InterSwap &mv) const {
    do {
        AnyRandomMove(rp, mv);
    }while (!FeasibleMove(rp, mv));
}

bool InterSwapNeighborhoodExplorer::FeasibleMove(const RoutePlan &rp,
                                               const InterSwap &mv) const {
    if (!rp[mv.route1].size() || !rp[mv.route2].size())
        return false;
    if (rp[mv.route1].IsExcList() &&
        in.OrderGroupVect(mv.ord2).IsMandatory())
        return false;
    else if (rp[mv.route2].IsExcList() &&
             in.OrderGroupVect(mv.ord1).IsMandatory())
        return false;
    if (mv.route1 == mv.route2)
        return false;
    return true;
}

void InterSwapNeighborhoodExplorer::AnyRandomMove(const RoutePlan &rp,
                                                InterSwap &mv) const {
    mv.route1 = Random::Int(0, rp.size() - 1);
    mv.pos1 = mv.ord1 = 0;
    if (rp[mv.route1].size()) {
        mv.pos1 = Random::Int(0, rp[mv.route1].size() - 1);
        mv.ord1 = rp[mv.route1][mv.pos1];
    }
    mv.route2 = Random::Int(0, rp.size() - 1);
    mv.pos2 = mv.ord2 = 0;
    if (rp[mv.route2].size()) {
        mv.pos2 = Random::Int(0, rp[mv.route2].size() - 1);
        mv.ord2 = rp[mv.route2][mv.pos2];
    }
}

void InterSwapNeighborhoodExplorer::FirstMove(const RoutePlan &rp,
                                            InterSwap &mv) const {
    assert(rp.size() > 1);
    mv.route1 = 0;
    mv.pos1 = 0;
    mv.ord1 = rp[mv.route1][mv.pos1];
    mv.route2 = 1;
    mv.pos2 = 0;
    mv.ord2 = rp[mv.route2][mv.pos2];
}

bool InterSwapNeighborhoodExplorer::NextMove(const RoutePlan &rp,
                                           InterSwap &mv) const {
    bool not_last = true;
    do {
        not_last = AnyNextMove(rp, mv);
    }while(!FeasibleMove(rp, mv));
    return not_last;
}

bool InterSwapNeighborhoodExplorer::AnyNextMove(const RoutePlan &rp,
                                              InterSwap &mv) const {
    if (mv.pos2 < rp[mv.route2].size() - 1) {
        mv.pos2++;
    } else if (mv.route2 < rp.size() - 1) {
        mv.route2++;
        mv.pos2 = 0;
    } else {
        if (mv.pos1 < rp[mv.route1].size() - 1) {
            mv.pos1++;
            mv.route2 = mv.route1 + 1;
            mv.pos2 = 0;
        } else if (mv.route1 < rp.size() - 2) {
            mv.route1++;
            mv.pos1 = 0;
            mv.route2 = mv.route1 + 1;
            mv.pos2 = 0;
        } else {
            return false;   // last move in state
        }
    }
    mv.ord1 = rp[mv.route1][mv.pos1];
    mv.ord2 = rp[mv.route2][mv.pos2];
    return true;
}

void
InterSwapNeighborhoodExplorer::MakeMove(RoutePlan &rp,
                                      const InterSwap &mv) const {
    std::swap(rp[mv.route1][mv.pos1], rp[mv.route2][mv.pos2]);
    // update timetable
    UpdateRouteTimetable(rp.timetable(mv.route1), rp[mv.route1]);
    UpdateRouteTimetable(rp.timetable(mv.route2), rp[mv.route2]);
}

int
InterSwapNeighborhoodExplorer::DeltaCostFunction(const RoutePlan &rp,
                                               const InterSwap &mv) const {
    // ugly reduntant data
    routes.clear();
    routes.push_back(rp[mv.route1]);
    routes.push_back(rp[mv.route2]);
    std::swap(routes[0][mv.pos1], routes[1][mv.pos2]);
    timetable.clear();
    timetable.resize(2);
    if (!routes[0].IsExcList())
        UpdateRouteTimetable(timetable[0], rp[mv.route1]);
    if (!routes[1].IsExcList())
        UpdateRouteTimetable(timetable[1], rp[mv.route2]);

    return DeltaObjective(rp, mv) + DeltaViolations(rp, mv);
}

int
InterSwapNeighborhoodExplorer::DeltaObjective(const RoutePlan &rp,
                                            const InterSwap &mv) const {
    int w = 100;
    return DeltaDateViolationCost(rp, mv, w) + DeltaTimeViolationCost(rp, mv, w)
           + DeltaOptOrderCost(rp, mv, w) + DeltaTranportationCost(rp, mv);
}

int
InterSwapNeighborhoodExplorer::DeltaViolations(const RoutePlan &rp,
                                             const InterSwap &mv) const {
    int w = 100;
    return DeltaCapExceededCost(rp, mv, w) + DeltaLateReturnCost(rp, mv, w);
}

int
InterSwapNeighborhoodExplorer::DeltaDateViolationCost(const RoutePlan &rp,
                                        const InterSwap &mv, int weight) const {
    const OrderGroup& ord1 = in.OrderGroupVect(mv.ord1);
    const OrderGroup& ord2 = in.OrderGroupVect(mv.ord2);
    unsigned old_day = rp[mv.route1].get_day();
    unsigned new_day = rp[mv.route2].get_day();
    unsigned d1 = ord1.get_demand(), d2 = ord2.get_demand();
    int delta = d1 * (ord1.IsDayFeasible(old_day) - ord1.IsDayFeasible(new_day)) +
                d2 * (ord2.IsDayFeasible(new_day) - ord2.IsDayFeasible(old_day));
    return (weight * delta);
}

int
InterSwapNeighborhoodExplorer::DeltaTimeViolationCost(const RoutePlan &rp,
                                        const InterSwap &mv, int weight) const {
    int isnew = 0, delta = 0;
    if (!rp[mv.route1].IsExcList()) {
        delta += DeltaRouteTimeViolation(rp, isnew, mv.route1, mv.pos1);
    }

    isnew = 1;
    if (!rp[mv.route2].IsExcList())
        delta += DeltaRouteTimeViolation(rp, isnew, mv.route2, mv.pos2);

    return delta;
}

int
InterSwapNeighborhoodExplorer::DeltaOptOrderCost(const RoutePlan &rp,
                                        const InterSwap &mv, int weight) const {
    int delta = 0;
    const OrderGroup& ord1 = in.OrderGroupVect(mv.ord1);
    const OrderGroup& ord2 = in.OrderGroupVect(mv.ord2);
    if (!ord1.IsMandatory()) {
        if (rp[mv.route1].IsExcList())
            delta -= ord1.get_demand();
        else if (rp[mv.route2].IsExcList())
            delta += ord1.get_demand();
    }
    if (!ord2.IsMandatory()) {
        if (rp[mv.route2].IsExcList())
            delta -= ord2.get_demand();
        else if (rp[mv.route1].IsExcList())
            delta += ord2.get_demand();
    }
    return (weight * delta);
}

int
InterSwapNeighborhoodExplorer::DeltaTranportationCost(const RoutePlan &rp,
                                                    const InterSwap &mv) const {
    int delta = 0;
    const Billing *cr = in.FindBilling(rp[mv.route1].get_vehicle());
    if (!rp[mv.route1].IsExcList())
        delta += cr->GetCostComponent().Cost(routes[0]) -
                 cr->GetCostComponent().Cost(rp[mv.route1]);
    cr = in.FindBilling(rp[mv.route2].get_vehicle());
    if (!rp[mv.route2].IsExcList())
        delta += cr->GetCostComponent().Cost(routes[1]) -
                 cr->GetCostComponent().Cost(rp[mv.route2]);
    return delta;
}

int
InterSwapNeighborhoodExplorer::DeltaCapExceededCost(const RoutePlan &rp,
                                        const InterSwap &mv, int weight) const {
    int delta = 0;
    unsigned vehicle_cap = 0, route_demand = 0;
    unsigned demand_from = in.OrderGroupVect(mv.ord1).get_demand();
    unsigned demand_to = in.OrderGroupVect(mv.ord2).get_demand();
    if (!rp[mv.route1].IsExcList()) {
        vehicle_cap = in.VehicleVect(rp[mv.route1].get_vehicle()).get_cap();
        route_demand = rp[mv.route1].demand();
        if (route_demand > vehicle_cap) {
            if (route_demand + demand_to - demand_from > vehicle_cap)
                delta += demand_to - demand_from;
            else
                delta -= route_demand - vehicle_cap;
        } else if (route_demand + demand_to - demand_from > vehicle_cap) {
            delta += route_demand + demand_to - demand_from - vehicle_cap;
        }
    }
    if (!rp[mv.route2].IsExcList()) {
        vehicle_cap = in.VehicleVect(rp[mv.route2].get_vehicle()).get_cap();
        route_demand = rp[mv.route2].demand();
        if (route_demand > vehicle_cap) {
            if (route_demand + demand_from - demand_to > vehicle_cap)
                delta += demand_from - demand_to;
            else
                delta -= route_demand - vehicle_cap;
        } else if (route_demand + demand_from - demand_to > vehicle_cap) {
            delta += route_demand - demand_to + demand_from - vehicle_cap;
        }
    }
    return (weight * delta);
}

int
InterSwapNeighborhoodExplorer::DeltaLateReturnCost(const RoutePlan &rp,
                                        const InterSwap &mv, int weight) const {
    int delta = 0;
    int shutdown_time = in.get_return_time() + 3600;  // plus 1 hour

    for (unsigned j = mv.pos1; j < rp[mv.route1].size(); ++j) {
        if (rp(mv.route1, j) > shutdown_time)
            --delta;
        if (timetable[0][j] > shutdown_time)
            ++delta;
    }

    for (unsigned j = mv.pos2; j < rp[mv.route2].size(); ++j) {
        if (rp(mv.route2, j) > shutdown_time)
            --delta;
        if (timetable[1][j] > shutdown_time)
            ++delta;
    }

    return (weight * delta);
}


// Intra Route Swap Implementation

void IntraSwapNeighborhoodExplorer::RandomMove(const RoutePlan &rp,
                                             IntraSwap &mv) const {
    do {
        AnyRandomMove(rp, mv);
    }while (!FeasibleMove(rp, mv));
}

bool IntraSwapNeighborhoodExplorer::FeasibleMove(const RoutePlan &rp,
                                               const IntraSwap &mv) const {
    if (rp[mv.route].IsExcList())
        return false;
    if (rp[mv.route].size() < 2)
        return false;
    if (mv.pos1 == mv.pos2)
        return false;
    return true;
}

void IntraSwapNeighborhoodExplorer::AnyRandomMove(const RoutePlan &rp,
                                                IntraSwap &mv) const {
    mv.route = Random::Int(0, rp.size() - 1);
    assert(rp[mv.route].size() > 1);
    mv.pos1 = Random::Int(0, rp[mv.route].size() - 1);
    mv.pos2 = Random::Int(0, rp[mv.route].size() - 1);
    mv.ord1 = rp[mv.route][mv.pos1];
    mv.ord2 = rp[mv.route][mv.pos2];
}

void IntraSwapNeighborhoodExplorer::FirstMove(const RoutePlan &rp,
                                            IntraSwap &mv) const {
    assert(rp.size() > 1);
    mv.route = 0;
    mv.pos1 = 0;
    mv.pos2 = 1;
    mv.ord1 = rp[mv.route][mv.pos1];
    mv.ord2 = rp[mv.route][mv.pos2];
}

bool IntraSwapNeighborhoodExplorer::NextMove(const RoutePlan &rp,
                                           IntraSwap &mv) const {
    bool not_last = true;
    do {
       not_last = AnyNextMove(rp, mv);
    }while(!FeasibleMove(rp, mv));
    return not_last;
}

bool IntraSwapNeighborhoodExplorer::AnyNextMove(const RoutePlan &rp,
                                              IntraSwap &mv) const {
    if (mv.pos2 < rp[mv.route].size() - 1) {
        mv.pos2++;
    } else if (mv.pos1 < rp[mv.route].size() - 2) {
        mv.pos1++;
        mv.pos2 = mv.pos1 + 1;
    } else if (mv.route < rp.size() - 1) {
        mv.route++;
        mv.pos1 = 0;
        mv.pos2 = 1;
    } else {
        return false;
    }
    mv.ord1 = rp[mv.route][mv.pos1];
    mv.ord2 = rp[mv.route][mv.pos2];
    return true;
}

void
IntraSwapNeighborhoodExplorer::MakeMove(RoutePlan &rp,
                                      const IntraSwap &mv) const {
    std::swap(rp[mv.route][mv.pos1], rp[mv.route][mv.pos2]);
    // update timetable
    UpdateRouteTimetable(rp.timetable(mv.route), rp[mv.route]);
}

int
IntraSwapNeighborhoodExplorer::DeltaCostFunction(const RoutePlan &rp,
                                               const IntraSwap &mv) const {
    // ugly reduntant data
    routes.clear();
    routes.push_back(rp[mv.route]);
    std::swap(routes[0][mv.pos1], routes[0][mv.pos2]);
    timetable.clear();
    timetable.resize(1);
    if (!routes[0].IsExcList())
        UpdateRouteTimetable(timetable[0], rp[mv.route]);

    return DeltaObjective(rp, mv) + DeltaViolations(rp, mv);
}

int
IntraSwapNeighborhoodExplorer::DeltaObjective(const RoutePlan &rp,
                                            const IntraSwap &mv) const {
    int w = 100;
    return DeltaTimeViolationCost(rp, mv, w) + DeltaTranportationCost(rp, mv);
}

int
IntraSwapNeighborhoodExplorer::DeltaViolations(const RoutePlan &rp,
                                             const IntraSwap &mv) const {
    int w = 100;
    return DeltaCapExceededCost(rp, mv, w) + DeltaLateReturnCost(rp, mv, w);
}


int
IntraSwapNeighborhoodExplorer::DeltaTimeViolationCost(const RoutePlan &rp,
                                        const IntraSwap &mv, int weight) const {
    int delta = 0;
    if (!rp[mv.route].IsExcList()) {
        unsigned pos = mv.pos1;
        if (mv.pos1 > mv.pos2)
            pos = mv.pos2;
        delta += DeltaRouteTimeViolation(rp, 0, mv.route, pos);
    }

    return delta;
}

int
IntraSwapNeighborhoodExplorer::DeltaTranportationCost(const RoutePlan &rp,
                                                    const IntraSwap &mv) const {
    return 0;
}

int
IntraSwapNeighborhoodExplorer::DeltaCapExceededCost(const RoutePlan &rp,
                                        const IntraSwap &mv, int weight) const {
    return 0;
}

int
IntraSwapNeighborhoodExplorer::DeltaLateReturnCost(const RoutePlan &rp,
                                        const IntraSwap &mv, int weight) const {
    int delta = 0;
    int shutdown_time = in.get_return_time() + 3600;  // plus 1 hour

    for (unsigned j = mv.pos1; j < rp[mv.route].size(); ++j) {
        if (rp(mv.route, j) > shutdown_time)
            --delta;
        if (timetable[0][j] > shutdown_time)
            ++delta;
    }

    return (weight * delta);
}
