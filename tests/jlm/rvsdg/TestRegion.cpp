/*
 * Copyright 2020 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include "test-registry.hpp"
#include "test-operation.hpp"
#include "test-types.hpp"

#include <cassert>

/**
 * Test check for adding argument to input of wrong structural node.
 */
static void
TestArgumentNodeMismatch()
{
  using namespace jlm::rvsdg;

  jlm::tests::valuetype vt;

  jlm::rvsdg::graph graph;
  auto import = graph.add_import({vt, "import"});

  auto structuralNode1 = jlm::tests::structural_node::create(graph.root(), 1);
  auto structuralNode2 = jlm::tests::structural_node::create(graph.root(), 2);

  auto structuralInput = structural_input::create(structuralNode1, import, vt);

  bool inputErrorHandlerCalled = false;
  try {
    argument::create(structuralNode2->subregion(0), structuralInput, vt);
  } catch (jlm::util::error & e) {
    inputErrorHandlerCalled = true;
  }

  assert(inputErrorHandlerCalled);
}

/**
 * Test check for adding result to output of wrong structural node.
 */
static void
TestResultNodeMismatch()
{
  using namespace jlm::rvsdg;

  jlm::tests::valuetype vt;

  jlm::rvsdg::graph graph;
  auto import = graph.add_import({vt, "import"});

  auto structuralNode1 = jlm::tests::structural_node::create(graph.root(), 1);
  auto structuralNode2 = jlm::tests::structural_node::create(graph.root(), 2);

  auto structuralInput = structural_input::create(structuralNode1, import, vt);

  auto argument = argument::create(structuralNode1->subregion(0), structuralInput, vt);
  auto structuralOutput = structural_output::create(structuralNode1, vt);

  bool outputErrorHandlerCalled = false;
  try {
    result::create(structuralNode2->subregion(0), argument, structuralOutput, vt);
  } catch (jlm::util::error & e) {
    outputErrorHandlerCalled = true;
  }

  assert(outputErrorHandlerCalled);
}

/**
 * Test region::Contains().
 */
static void
TestContainsMethod()
{
  using namespace jlm::tests;

  valuetype vt;

  jlm::rvsdg::graph graph;
  auto import = graph.add_import({vt, "import"});

  auto structuralNode1 = structural_node::create(graph.root(), 1);
  auto structuralInput1 = jlm::rvsdg::structural_input::create(structuralNode1, import, vt);
  auto regionArgument1 = jlm::rvsdg::argument::create(structuralNode1->subregion(0), structuralInput1, vt);
  unary_op::create(structuralNode1->subregion(0), {vt}, regionArgument1, {vt});

  auto structuralNode2 = structural_node::create(graph.root(), 1);
  auto structuralInput2 = jlm::rvsdg::structural_input::create(structuralNode2, import, vt);
  auto regionArgument2 = jlm::rvsdg::argument::create(structuralNode2->subregion(0), structuralInput2, vt);
  binary_op::create({vt}, {vt}, regionArgument2, regionArgument2);

  assert(jlm::rvsdg::region::Contains<structural_op>(*graph.root(), false));
  assert(jlm::rvsdg::region::Contains<unary_op>(*graph.root(), true));
  assert(jlm::rvsdg::region::Contains<binary_op>(*graph.root(), true));
  assert(!jlm::rvsdg::region::Contains<test_op>(*graph.root(), true));
}

/**
 * Test region::IsRootRegion().
 */
static void
TestIsRootRegion()
{
  jlm::rvsdg::graph graph;

  auto structuralNode = jlm::tests::structural_node::create(graph.root(), 1);

  assert(graph.root()->IsRootRegion());
  assert(!structuralNode->subregion(0)->IsRootRegion());
}

/**
 * Test region::NumRegions()
 */
static void
TestNumRegions()
{
  using namespace jlm::rvsdg;

  {
    jlm::rvsdg::graph graph;

    assert(region::NumRegions(*graph.root()) == 1);
  }

  {
    jlm::rvsdg::graph graph;
    auto structuralNode = jlm::tests::structural_node::create(graph.root(), 4);
    jlm::tests::structural_node::create(structuralNode->subregion(0), 2);
    jlm::tests::structural_node::create(structuralNode->subregion(3), 5);

    assert(region::NumRegions(*graph.root()) == 1+4+2+5);
  }
}

static int
Test()
{
  TestArgumentNodeMismatch();
  TestResultNodeMismatch();

  TestContainsMethod();
  TestIsRootRegion();
  TestNumRegions();

  return 0;
}

JLM_UNIT_TEST_REGISTER("jlm/rvsdg/TestRegion", Test)