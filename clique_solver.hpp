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
  virtual std::vector<gint> get_ordered_indices(gint n) const {
    std::vector<gint> indices(n);
    for (gint i = 0; i < n; ++i) indices[i] = i;
    return indices;
  }
  virtual bool is_best_improvement() const { return true; };
};

class WeightedStrategy : public Strategy {
  public:
  vertex select(const Graph &,
                const std::vector<vertex> &candidates) const;
  std::string toString() const override { return "WeightedResidual"; }
  bool is_best_improvement() const override { return true; }
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
  virtual std::vector<gint> get_ordered_indices(gint n) const override;
  std::string toString() const override { return "Last"; }
};

class MaxResidualDegreeStrategy : public Strategy {
public:
  vertex select(const Graph &g,
                const std::vector<vertex> &candidates) const override;
  std::string toString() const override { return "Max residual degree"; };
  bool is_best_improvement() const override { return true; }
};

class RandomStrategy : public Strategy {
public:
  vertex select(const Graph &,
                const std::vector<vertex> &candidates) const override {
    if (candidates.empty())
      return -1;
    return candidates[rand() % candidates.size()];
  }
  virtual std::vector<gint> get_ordered_indices(gint n) const override;
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