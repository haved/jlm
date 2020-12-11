/*
 * Copyright 2020 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <test-operation.hpp>
#include <test-registry.hpp>
#include <test-types.hpp>

#include <jlm/ir/rvsdg-module.hpp>
#include <jlm/ir/operators/lambda.hpp>

static void
test_argument_iterators()
{
	using namespace jlm;

	valuetype vt;
	rvsdg_module rm(filepath(""), "", "");

	{
		jive::fcttype ft({&vt}, {&vt});

		auto lambda = lambda::node::create(rm.graph()->root(), ft, "f", linkage::external_linkage, {});
		lambda->finalize({lambda->fctargument(0)});

		std::vector<jive::argument*> args;
		for (auto it = lambda->begin_arg(); it != lambda->end_arg(); it++)
			args.push_back(it.value());

		assert(args.size() == 1 && args[0] == lambda->fctargument(0));
	}

	{
		jive::fcttype ft({}, {&vt});

		auto lambda = lambda::node::create(rm.graph()->root(), ft, "f", linkage::external_linkage, {});

		auto nullary = create_testop(lambda->subregion(), {}, {&vt});

		lambda->finalize({nullary});

		assert(lambda->nfctarguments() == 0);
	}

	{
		auto i = rm.graph()->add_import({vt, ""});

		jive::fcttype ft({&vt, &vt, &vt}, {&vt, &vt});

		auto lambda = lambda::node::create(rm.graph()->root(), ft, "f", linkage::external_linkage, {});

		auto cv = lambda->add_ctxvar(i);

		lambda->finalize({lambda->fctargument(0), cv});

		std::vector<jive::argument*> args;
		for (auto it = lambda->begin_arg(); it != lambda->end_arg(); it++)
			args.push_back(it.value());

		assert(args.size() == 3);
		assert(args[0] == lambda->fctargument(0));
		assert(args[1] == lambda->fctargument(1));
		assert(args[2] == lambda->fctargument(2));
	}
}

static int
test()
{
	test_argument_iterators();

	return 0;
}

JLM_UNIT_TEST_REGISTER("libjlm/ir/operators/test-lambda", test)
