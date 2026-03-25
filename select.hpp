#pragma once

#include "graph.hpp"
#include <vector>
#include <string>

void print_vector(vector<vertex> vec);


class Strategy {
public:
    virtual ~Strategy() {}
    virtual vertex select(const Graph &g, const std::vector<vertex> &restants) const = 0;
    virtual string toString() const=0;
};

bool descente_n1(const Graph &g, std::vector<vertex> &clique, std::vector<vertex> restants, const Strategy &s);

// Répond à la Question 1 (MCP)
class MaxResidualDegreeStrategy : public Strategy {
public:
    ~MaxResidualDegreeStrategy()=default;
    vertex select(const Graph &g, const std::vector<vertex> &restants) const override;
    string toString() const override {return "Max residual degree";};
};

// Répond à la Question 3 (WMCP)
class MaxWeightStrategy : public Strategy {
public:
    ~MaxWeightStrategy()=default;
    vertex select(const Graph &g, const std::vector<vertex> &restants) const override;
    string toString() const override {return "Max weight";};
};
