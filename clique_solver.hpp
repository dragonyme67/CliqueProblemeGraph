#pragma once

#include "graph.hpp"
#include "utils.hpp"
#include <functional>
#include <string>
#include <vector>

class Strategy {
public:
  virtual ~Strategy() {}
  virtual vertex select(const Graph &g,
                        const std::vector<vertex> &candidates) const = 0;
  virtual std::string toString() const = 0;
};

class FirstStrategy : public Strategy {
public:
  vertex select(const Graph &,
                const std::vector<vertex> &candidates) const override {
    return candidates.empty() ? -1 : candidates.front();
  }
  std::string toString() const override { return "First"; }
};

class LastStrategy : public Strategy {
public:
  vertex select(const Graph &,
                const std::vector<vertex> &candidates) const override {
    return candidates.empty() ? -1 : candidates.back();
  }
  std::string toString() const override { return "Last"; }
};

class MaxResidualDegreeStrategy : public Strategy {
public:
  vertex select(const Graph &g,
                const std::vector<vertex> &candidates) const override;
  std::string toString() const override { return "Max residual degree"; };
};

class RandomStrategy : public Strategy {
public:
  vertex select(const Graph &,
                const std::vector<vertex> &candidates) const override {
    if (candidates.empty())
      return -1;
    return candidates[rand() % candidates.size()];
  }
  std::string toString() const override { return "Random"; }
};

void print_vector(std::vector<vertex> vec);

std::vector<vertex> greedy_descent_n1(const Graph &g,
                                      std::vector<vertex> initial_clique,
                                      const Strategy &s);

std::vector<vertex> pair_descent_n2(const Graph &g,
                                    std::vector<vertex> initial_clique,
                                    const Strategy &s,
                                    bool use_fallback = true);

std::vector<vertex> triple_descent_n3(const Graph &g,
                                      std::vector<vertex> initial_clique,
                                      const Strategy &s);

std::vector<vertex> ruin_and_recreate(const Graph &g,
                                      std::vector<vertex> initial_clique,
                                      int iterations, int ruin_percent,
                                      const Strategy &s);