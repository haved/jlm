/*
 * Copyright 2015 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_FRONTEND_LLVM_JLM2RVSDG_INTERPROCEDURALGRAPHCONVERSION_H
#define JLM_FRONTEND_LLVM_JLM2RVSDG_INTERPROCEDURALGRAPHCONVERSION_H

#include <memory>

namespace jlm {

class ipgraph_module;
class RvsdgModule;
class StatisticsDescriptor;

std::unique_ptr<RvsdgModule>
ConvertInterProceduralGraphModule(const ipgraph_module & im, const StatisticsDescriptor & sd);

}

#endif