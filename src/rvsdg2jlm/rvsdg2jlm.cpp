/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jive/types/function/fctlambda.h>
#include <jive/vsdg/gamma.h>
#include <jive/vsdg/graph.h>
#include <jive/vsdg/phi.h>
#include <jive/vsdg/theta.h>
#include <jive/vsdg/traverser.h>

#include <jlm/common.hpp>
#include <jlm/ir/basic_block.hpp>
#include <jlm/ir/cfg-structure.hpp>
#include <jlm/ir/data.hpp>
#include <jlm/ir/module.hpp>
#include <jlm/ir/operators.hpp>
#include <jlm/ir/rvsdg.hpp>
#include <jlm/ir/tac.hpp>
#include <jlm/rvsdg2jlm/context.hpp>
#include <jlm/rvsdg2jlm/rvsdg2jlm.hpp>

#include <deque>

namespace jlm {
namespace rvsdg2jlm {

static inline const jive::output *
root_port(const jive::output * port)
{
	auto root = port->region()->graph()->root();
	if (port->region() == root)
		return port;

	auto node = port->node();
	JLM_DEBUG_ASSERT(node && dynamic_cast<const jive::fct::lambda_op*>(&node->operation()));
	JLM_DEBUG_ASSERT(node->output(0)->nusers() == 1);
	auto user = *node->output(0)->begin();
	node = port->region()->node();
	JLM_DEBUG_ASSERT(node && dynamic_cast<const jive::phi_op*>(&node->operation()));
	port = node->output(user->index());

	JLM_DEBUG_ASSERT(port->region() == root);
	return port;
}

static inline bool
is_exported(const jive::output * port)
{
	port = root_port(port);
	for (const auto & user : *port) {
		if (dynamic_cast<const jive::result*>(user))
			return true;
	}

	return false;
}

static inline std::string
get_name(const jive::output * port)
{
	port = root_port(port);
	for (const auto & user : *port) {
		if (auto result = dynamic_cast<const jive::result*>(user)) {
			JLM_DEBUG_ASSERT(result->gate());
			return result->gate()->name();
		}
	}

	static size_t c = 0;
	return strfmt("f", c++);
}

static inline const jlm::tac *
create_assignment_lpbb(const jlm::variable * argument, const jlm::variable * result, context & ctx)
{
	return append_last(ctx.lpbb(), create_assignment(argument->type(), argument, result));
}

static inline std::unique_ptr<const expr>
convert_port(const jive::output * port)
{
	auto node = port->node();
	std::vector<std::unique_ptr<const expr>> operands;
	for (size_t n = 0; n < node->ninputs(); n++)
		operands.push_back(convert_port(node->input(n)->origin()));

	return std::make_unique<const jlm::expr>(node->operation(), std::move(operands));
}

static void
convert_node(const jive::node & node, context & ctx);

static inline void
convert_region(jive::region & region, context & ctx)
{
	auto entry = create_basic_block_node(ctx.cfg());
	ctx.lpbb()->add_outedge(entry);
	ctx.set_lpbb(entry);

	for (const auto & node : jive::topdown_traverser(&region))
		convert_node(*node, ctx);

	auto exit = create_basic_block_node(ctx.cfg());
	ctx.lpbb()->add_outedge(exit);
	ctx.set_lpbb(exit);
}

static inline std::unique_ptr<jlm::cfg>
create_cfg(const jive::node & node, context & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const jive::fct::lambda_op*>(&node.operation()));
	auto region = static_cast<const jive::structural_node*>(&node)->subregion(0);
	auto & module = ctx.module();

	JLM_DEBUG_ASSERT(ctx.lpbb() == nullptr);
	std::unique_ptr<jlm::cfg> cfg(new jlm::cfg(ctx.module()));
	auto entry = create_basic_block_node(cfg.get());
	cfg->exit_node()->divert_inedges(entry);
	ctx.set_lpbb(entry);
	ctx.set_cfg(cfg.get());

	/* add arguments and dependencies */
	for (size_t n = 0; n < region->narguments(); n++) {
		const variable * v = nullptr;
		auto argument = region->argument(n);
		if (argument->input()) {
			v = ctx.variable(argument->input()->origin());
		} else {
			v = module.create_variable(argument->type(), "", false);
			cfg->entry().append_argument(v);
		}

		ctx.insert(argument, v);
	}

	convert_region(*region, ctx);

	/* add results */
	for (size_t n = 0; n < region->nresults(); n++)
		cfg->exit().append_result(ctx.variable(region->result(n)->origin()));

	ctx.lpbb()->add_outedge(cfg->exit_node());
	ctx.set_lpbb(nullptr);
	ctx.set_cfg(nullptr);

	straighten(*cfg);
	return cfg;
}

static inline void
convert_simple_node(const jive::node & node, context & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const jive::simple_op*>(&node.operation()));

	std::vector<const variable*> operands;
	for (size_t n = 0; n < node.ninputs(); n++)
		operands.push_back(ctx.variable(node.input(n)->origin()));

	std::vector<tacvariable*> tvs;
	std::vector<const variable*> results;
	for (size_t n = 0; n < node.noutputs(); n++) {
		auto v = ctx.module().create_tacvariable(node.output(n)->type());
		ctx.insert(node.output(n), v);
		results.push_back(v);
		tvs.push_back(v);
	}

	append_last(ctx.lpbb(), create_tac(node.operation(), operands, results));
	/* FIXME: remove again once tacvariables owner's are tacs */
	for (const auto & tv : tvs)
		tv->set_tac(static_cast<const basic_block*>(&ctx.lpbb()->attribute())->last());
}

static inline void
convert_gamma_node(const jive::node & node, context & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const jive::gamma_op*>(&node.operation()));
	auto nalternatives = static_cast<const jive::gamma_op*>(&node.operation())->nalternatives();
	auto & snode = *static_cast<const jive::structural_node*>(&node);
	auto predicate = node.input(0)->origin();
	auto & module = ctx.module();
	auto cfg = ctx.cfg();

	std::vector<cfg_node*> phi_nodes;
	auto entry = create_basic_block_node(cfg);
	auto exit = create_basic_block_node(cfg);
	append_last(entry, create_branch_tac(nalternatives, ctx.variable(predicate)));
	ctx.lpbb()->add_outedge(entry);

	/* convert gamma regions */
	for (size_t n = 0; n < snode.nsubregions(); n++) {
		auto subregion = snode.subregion(n);

		/* add arguments to context */
		for (size_t i = 0; i < subregion->narguments(); i++) {
			auto argument = subregion->argument(i);
			ctx.insert(argument, ctx.variable(argument->input()->origin()));
		}

		/* convert subregion */
		auto region_entry = create_basic_block_node(cfg);
		entry->add_outedge(region_entry);
		ctx.set_lpbb(region_entry);
		convert_region(*subregion, ctx);

		phi_nodes.push_back(ctx.lpbb());
		ctx.lpbb()->add_outedge(exit);
	}

	/* add phi instructions */
	for (size_t n = 0; n < snode.noutputs(); n++) {
		auto output = snode.output(n);
		std::unordered_set<const variable*> variables;
		std::vector<std::pair<const variable*, cfg_node*>> arguments;
		for (size_t i = 0; i < snode.nsubregions(); i++) {
			auto v = ctx.variable(snode.subregion(i)->result(n)->origin());
			arguments.push_back(std::make_pair(v, phi_nodes[i]));
			variables.insert(v);
		}

		if (variables.size() != 1) {
			auto v = module.create_variable(output->type(), false);
			append_last(exit, create_phi_tac(arguments, v));
			ctx.insert(output, v);
		} else {
			/* all phi operands are the same */
			ctx.insert(output, *variables.begin());
		}
	}

	ctx.set_lpbb(exit);
}

static inline void
convert_theta_node(const jive::node & node, context & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const jive::theta_op*>(&node.operation()));
	auto subregion = static_cast<const jive::structural_node*>(&node)->subregion(0);
	auto predicate = subregion->result(0)->origin();

	auto pre_entry = ctx.lpbb();
	auto entry = create_basic_block_node(ctx.cfg());
	ctx.lpbb()->add_outedge(entry);
	ctx.set_lpbb(entry);

	/* create loop variables and add arguments to context */
	std::deque<jlm::variable*> lvs;
	for (size_t n = 0; n < subregion->narguments(); n++) {
		auto argument = subregion->argument(n);
		auto lv = ctx.variable(node.input(n)->origin());
		if (!is_gblvariable(lv)) {
			auto v = ctx.module().create_variable(argument->type(), false);
			lvs.push_back(v);
			lv = v;
		}
		ctx.insert(argument, lv);
	}

	convert_region(*subregion, ctx);

	/* add results to context and phi instructions */
	for (size_t n = 1; n < subregion->nresults(); n++) {
		auto result = subregion->result(n);

		auto v1 = ctx.variable(node.input(n-1)->origin());
		if (is_gblvariable(v1)) {
			ctx.insert(result->output(), v1);
			continue;
		}

		auto v2 = ctx.variable(result->origin());
		auto lv = lvs.front();
		lvs.pop_front();
		append_last(entry, create_phi_tac({{v1, pre_entry}, {v2, ctx.lpbb()}}, lv));
	}
	JLM_DEBUG_ASSERT(lvs.empty());

	append_last(ctx.lpbb(), create_branch_tac(2, ctx.variable(predicate)));
	auto exit = create_basic_block_node(ctx.cfg());
	ctx.lpbb()->add_outedge(entry);
	ctx.lpbb()->add_outedge(exit);
	ctx.set_lpbb(exit);
}

static inline void
convert_lambda_node(const jive::node & node, context & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const jive::fct::lambda_op*>(&node.operation()));
	auto & module = ctx.module();
	auto & clg = module.clg();

	const auto & ftype = *static_cast<const jive::fct::type*>(&node.output(0)->type());
	/* FIXME: create/get names for lambdas */
	auto name = get_name(node.output(0));
	auto exported = is_exported(node.output(0));
	auto f = clg_node::create(clg, name, ftype, exported);
	auto linkage = exported ? jlm::linkage::external_linkage : jlm::linkage::internal_linkage;
	auto v = module.create_variable(f, linkage);

	f->add_cfg(create_cfg(node, ctx));
	ctx.insert(node.output(0), v);
}

static inline void
convert_phi_node(const jive::node & node, context & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const jive::phi_op*>(&node.operation()));
	auto subregion = static_cast<const jive::structural_node*>(&node)->subregion(0);
	auto & module = ctx.module();
	auto & clg = module.clg();

	/* FIXME: handle phi node dependencies */
	JLM_DEBUG_ASSERT(subregion->narguments() == subregion->nresults());

	/* forward declare all functions */
	for (size_t n = 0; n < subregion->nresults(); n++) {
		auto result = subregion->result(n);
		auto lambda = result->origin()->node();
		JLM_DEBUG_ASSERT(dynamic_cast<const jive::fct::lambda_op*>(&lambda->operation()));

		auto name = get_name(lambda->output(0));
		auto exported = is_exported(lambda->output(0));
		auto & ftype = *static_cast<const jive::fct::type*>(&result->type());
		auto f = clg_node::create(clg, name, ftype, exported);
		auto linkage = exported ? jlm::linkage::external_linkage : jlm::linkage::internal_linkage;
		ctx.insert(subregion->argument(n), module.create_variable(f, linkage));
	}

	/* convert function bodies */
	for (size_t n = 0; n < subregion->nresults(); n++) {
		auto lambda = subregion->result(n)->origin()->node();
		auto v = static_cast<const jlm::fctvariable*>(ctx.variable(subregion->argument(n)));
		v->function()->add_cfg(create_cfg(*lambda, ctx));
		ctx.insert(lambda->output(0), v);
	}

	/* add functions to context */
	JLM_DEBUG_ASSERT(node.noutputs() == subregion->nresults());
	for (size_t n = 0; n < node.noutputs(); n++)
		ctx.insert(node.output(0), ctx.variable(subregion->result(n)->origin()));
}

static inline void
convert_data_node(const jive::node & node, context & ctx)
{
	JLM_DEBUG_ASSERT(is_data_op(node.operation()));
	auto subregion = static_cast<const jive::structural_node*>(&node)->subregion(0);
	const auto & op = *static_cast<const jlm::data_op*>(&node.operation());
	auto & module = ctx.module();

	JLM_DEBUG_ASSERT(subregion->nresults() == 1);
	auto result = subregion->result(0);

	auto name = get_name(result->output());
	const auto & type = result->output()->type();
	auto expression = convert_port(result->origin());

	auto linkage = op.linkage();
	auto constant = op.constant();
	auto v = module.create_global_value(type, name, linkage, constant, std::move(expression));
	ctx.insert(result->output(), v);
}

static inline void
convert_node(const jive::node & node, context & ctx)
{
	static std::unordered_map<
	  std::type_index
	, std::function<void(const jive::node & node, context & ctx)
	>> map({
	  {std::type_index(typeid(jive::fct::lambda_op)), convert_lambda_node}
	, {std::type_index(typeid(jive::gamma_op)), convert_gamma_node}
	, {std::type_index(typeid(jive::theta_op)), convert_theta_node}
	, {std::type_index(typeid(jive::phi_op)), convert_phi_node}
	, {std::type_index(typeid(jlm::data_op)), convert_data_node}
	});

	if (dynamic_cast<const jive::simple_op*>(&node.operation())) {
		convert_simple_node(node, ctx);
		return;
	}

	JLM_DEBUG_ASSERT(map.find(std::type_index(typeid(node.operation()))) != map.end());
	map[std::type_index(typeid(node.operation()))](node, ctx);
}

std::unique_ptr<jlm::module>
rvsdg2jlm(const jlm::rvsdg & rvsdg)
{
	auto & dl = rvsdg.data_layout();
	auto & tt = rvsdg.target_triple();
	std::unique_ptr<jlm::module> module(new jlm::module(tt, dl));
	auto graph = rvsdg.graph();
	auto & clg = module->clg();

	context ctx(*module);
	for (size_t n = 0; n < graph->root()->narguments(); n++) {
		auto argument = graph->root()->argument(n);
		if (auto ftype = dynamic_cast<const jive::fct::type*>(&argument->type())) {
			auto f = clg_node::create(clg, argument->gate()->name(), *ftype, false);
			auto v = module->create_variable(f, jlm::linkage::external_linkage);
			ctx.insert(argument, v);
		} else {
			const auto & type = argument->type();
			const auto & name = argument->gate()->name();
			auto v = module->create_global_value(type, name, jlm::linkage::external_linkage, false,
				nullptr);
			ctx.insert(argument, v);
		}
	}

	for (const auto & node : jive::topdown_traverser(graph->root()))
		convert_node(*node, ctx);

	return module;
}

}}
