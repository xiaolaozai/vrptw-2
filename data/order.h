#ifndef _ORDER_H
#define _ORDER_H
#include <iostream>
#include <string>
#include <vector>
#include <utility>

class Order {
    friend std::istream& operator>>(std::istream&, const Order&);
 public:
    Order(): quantity(0), mandatory(0), group(0) { }
    Order(const Order &o):
        id(o.id), id_client(o.id_client), quantity(o.quantity),
        mandatory(o.mandatory), date_window(o.date_window)
        { }
    Order(std::string id_o, std::string id_cli, unsigned qty, bool m,
          int rd = 1, int dd = 1):
        id(id_o), id_client(id_cli), quantity(qty), mandatory(m),
        date_window(rd, dd) { }
    std::string get_id() const { return id; }
    std::string get_client() const { return id_client; }
    unsigned get_demand() const { return quantity; }
    std::pair<int, int> get_dw() const { return date_window; }
    bool IsMandatory() const { return mandatory; }
    bool IsDayFeasible(int day) const {
        return (day >= date_window.first && day <= date_window.second); }
    void set_group(int i) { group = i; }
    int get_group() const { return group; }

 protected:
    std::string id, id_client;
    unsigned quantity;
    bool mandatory;
    std::pair<int, int> date_window;

 private:
    int group;
};

class OrderGroup : public Order {
 public:
    OrderGroup(const Order &o): Order(o), members() { members.push_back(o.get_id()); }
    OrderGroup(const OrderGroup &og): Order(og), members(og.members) { }
    unsigned size() const { return members.size(); }
    void insert(const Order &);
    bool IsGroupCompatible(const Order& o) const {
        return ((o.get_client() == id_client) && (o.IsMandatory() == mandatory)
                 && (o.get_dw() == date_window));
    }
    std::string& operator[](unsigned i) { return members[i]; }
    OrderGroup& operator=(const OrderGroup&);
 private:
    std::vector<std::string> members;
};
#endif
