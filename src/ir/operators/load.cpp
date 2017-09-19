/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jive/arch/memorytype.h>
#include <jive/vsdg/statemux.h>

#include <jlm/ir/operators/alloca.hpp>
#include <jlm/ir/operators/load.hpp>
#include <jlm/ir/operators/store.hpp>

namespace jlm {

/* load operator */

load_op::~load_op() noexcept
{}

bool
load_op::operator==(const operation & other) const noexcept
{
	auto op = dynamic_cast<const load_op*>(&other);
	return op
	    && op->nstates_ == nstates_
	    && op->aport_ == aport_
	    && op->vport_ == vport_
	    && op->alignment_ == alignment_;
}

size_t
load_op::narguments() const noexcept
{
	return 1 + nstates();
}

const jive::port &
load_op::argument(size_t index) const noexcept
{
	JLM_DEBUG_ASSERT(index < narguments());
	if (index == 0)
		return aport_;

	static const jive::port p(jive::mem::type::instance());
	return p;
}

size_t
load_op::nresults() const noexcept
{
	return 1;
}

const jive::port &
load_op::result(size_t index) const noexcept
{
	JLM_DEBUG_ASSERT(index < nresults());
	return vport_;
}

std::string
load_op::debug_string() const
{
	return "LOAD";
}

std::unique_ptr<jive::operation>
load_op::copy() const
{
	return std::unique_ptr<jive::operation>(new load_op(*this));
}

/* load normal form */

static bool
is_load_mux_reducible(const std::vector<jive::output*> & operands)
{
	auto muxnode = operands[1]->node();
	if (!muxnode || !is_mux_op(muxnode->operation()))
		return false;

	for (size_t n = 1; n < operands.size(); n++) {
		JLM_DEBUG_ASSERT(dynamic_cast<const jive::mem::type*>(&operands[n]->type()));
		if (operands[n]->node() && operands[n]->node() != muxnode)
			return false;
	}

	return true;
}

static std::vector<jive::output*>
is_load_alloca_reducible(const std::vector<jive::output*> & operands)
{
	auto address = operands[0];

	auto allocanode = address->node();
	if (!allocanode || !is_alloca_op(allocanode->operation()))
		return {std::next(operands.begin()), operands.end()};

	std::vector<jive::output*> new_states;
	for (size_t n = 1; n < operands.size(); n++) {
		JLM_DEBUG_ASSERT(dynamic_cast<const jive::mem::type*>(&operands[n]->type()));
		auto node = operands[n]->node();
		if (node && is_alloca_op(node->operation()) && node != allocanode)
			continue;

		new_states.push_back(operands[n]);
	}

	JLM_DEBUG_ASSERT(!new_states.empty());
	return new_states;
}

static bool
is_multiple_origin_reducible(const std::vector<jive::output*> & operands)
{
	std::unordered_set<jive::output*> states(std::next(operands.begin()), operands.end());
	return states.size() != operands.size()-1;
}

static std::vector<jive::output*>
perform_load_mux_reduction(
	const jlm::load_op & op,
	const std::vector<jive::output*> & operands)
{
	auto muxnode = operands[1]->node();
	return {create_load(operands[0], jive::operands(muxnode), op.alignment())};
}

static std::vector<jive::output*>
perform_load_alloca_reduction(
	const jlm::load_op & op,
	const std::vector<jive::output*> & operands,
	const std::vector<jive::output*> & new_states)
{
	JLM_DEBUG_ASSERT(!new_states.empty());
	return {create_load(operands[0], new_states, op.alignment())};
}

static std::vector<jive::output*>
perform_multiple_origin_reduction(
	const jlm::load_op & op,
	const std::vector<jive::output*> & operands)
{
	std::unordered_set<jive::output*> states(std::next(operands.begin()), operands.end());
	return {create_load(operands[0], {states.begin(), states.end()}, op.alignment())};
}

load_normal_form::~load_normal_form()
{}

load_normal_form::load_normal_form(
	const std::type_info & opclass,
	jive::node_normal_form * parent,
	jive::graph * graph) noexcept
: simple_normal_form(opclass, parent, graph)
, enable_load_mux_(false)
, enable_load_alloca_(false)
, enable_multiple_origin_(false)
{}

bool
load_normal_form::normalize_node(jive::node * node) const
{
	JLM_DEBUG_ASSERT(is_load_op(node->operation()));
	auto op = static_cast<const jlm::load_op*>(&node->operation());
	auto operands = jive::operands(node);

	if (!get_mutable())
		return true;

	if (get_load_mux_reducible() && is_load_mux_reducible(operands)) {
		replace(node, perform_load_mux_reduction(*op, operands));
		remove(node);
		return false;
	}

	auto new_states = is_load_alloca_reducible(operands);
	if (get_load_alloca_reducible() && new_states.size() != operands.size()-1) {
		replace(node, perform_load_alloca_reduction(*op, operands, new_states));
		remove(node);
		return false;
	}

	if (get_multiple_origin_reducible() && is_multiple_origin_reducible(operands)) {
		replace(node, perform_multiple_origin_reduction(*op, operands));
		remove(node);
		return false;
	}

	return simple_normal_form::normalize_node(node);
}

std::vector<jive::output*>
load_normal_form::normalized_create(
	jive::region * region,
	const jive::simple_op & op,
	const std::vector<jive::output*> & operands) const
{
	JLM_DEBUG_ASSERT(is_load_op(op));
	auto lop = static_cast<const jlm::load_op*>(&op);

	if (!get_mutable())
		return simple_normal_form::normalized_create(region, op, operands);

	if (get_load_mux_reducible() && is_load_mux_reducible(operands))
		return perform_load_mux_reduction(*lop, operands);

	auto new_states = is_load_alloca_reducible(operands);
	if (get_load_alloca_reducible() && new_states.size() != operands.size()-1)
		return perform_load_alloca_reduction(*lop, operands, new_states);

	if (get_multiple_origin_reducible() && is_multiple_origin_reducible(operands))
		return perform_multiple_origin_reduction(*lop, operands);

	return simple_normal_form::normalized_create(region, op, operands);
}

}

namespace {

static jive::node_normal_form *
create_load_normal_form(
	const std::type_info & opclass,
	jive::node_normal_form * parent,
	jive::graph * graph)
{
	return new jlm::load_normal_form(opclass, parent, graph);
}

static void __attribute__((constructor))
register_normal_form()
{
	jive::node_normal_form::register_factory(typeid(jlm::load_op), create_load_normal_form);
}

}