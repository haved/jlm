/*
 * Copyright 2025 Håvard Krogstie <krogstie.havard@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_LLVM_OPT_ALIAS_ANALYSES_PRECISIONEVALUATOR_HPP
#define JLM_LLVM_OPT_ALIAS_ANALYSES_PRECISIONEVALUATOR_HPP

#include <jlm/rvsdg/RvsdgModule.h>
#include <jlm/llvm/opt/alias-analyses/AliasAnalysis.hpp>
#include <jlm/util/Statistics.hpp>

namespace jlm::llvm::aa
{

/**
 * Class providing methods for evaluating the precision of a PointsToGraph on an RVSDG module.
 */
class PrecisionEvaluator {

public:
  void
  EvaluateAliasAnalysisClient(const rvsdg::RvsdgModule & rvsdgModule,
    const PointsToGraph & pointsToGraph,
    util::StatisticsCollector & statisticsCollector);

private:

  struct Context {

  };

  Context Context_;
};

}

#endif //JLM_LLVM_OPT_ALIAS_ANALYSES_PRECISIONEVALUATOR_HPP
