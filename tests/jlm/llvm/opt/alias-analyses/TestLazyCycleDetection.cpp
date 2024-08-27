/*
 * Copyright 2023, 2024 Håvard Krogstie <krogstie.havard@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <TestRvsdgs.hpp>

#include <test-registry.hpp>

#include <jlm/llvm/opt/alias-analyses/LazyCycleDetection.hpp>
#include <jlm/llvm/opt/alias-analyses/PointerObjectSet.hpp>
#include <jlm/util/HashSet.hpp>

#include <cassert>
#include <vector>

static int
TestUnifiesCycles()
{
  using namespace jlm;
  using namespace jlm::llvm::aa;

  // Arrange
  PointerObjectSet set;
  PointerObjectIndex node[6];
  for (unsigned int & i : node)
  {
    i = set.CreateDummyRegisterPointerObject();
  }

  // Create a graph that looks like
  //   --> 1 --> 2 --> 3
  //  /          |
  // 0           |
  //  \          V
  //   --> 5 --> 4
  std::vector<util::HashSet<PointerObjectIndex>> successors(set.NumPointerObjects());
  successors[node[0]].Insert(node[1]);
  successors[node[1]].Insert(node[2]);
  successors[node[2]].Insert(node[3]);
  successors[node[2]].Insert(node[4]);
  successors[node[0]].Insert(node[5]);
  successors[node[5]].Insert(node[4]);

  auto GetSuccessors = [&](PointerObjectIndex i)
  {
    assert(set.IsUnificationRoot(i));
    return successors[i].Items();
  };

  auto UnifyPointerObjects = [&](PointerObjectIndex a, PointerObjectIndex b)
  {
    assert(set.IsUnificationRoot(a));
    assert(set.IsUnificationRoot(b));
    assert(a != b);
    auto newRoot = set.UnifyPointerObjects(a, b);
    auto notRoot = a + b - newRoot;

    successors[newRoot].UnionWith(successors[notRoot]);
    return newRoot;
  };

  LazyCycleDetector lcd(set, GetSuccessors, UnifyPointerObjects);
  lcd.Initialize();

  // Act 1 - an edge that is not a part of a cycle
  lcd.OnPropagatedNothing(node[0], node[1]);

  // Assert that nothing happened
  assert(lcd.NumCycleDetectionAttempts() == 1);
  assert(lcd.NumCyclesDetected() == 0);
  assert(lcd.NumCycleUnifications() == 0);

  // Act 2 - Try the same edge again
  lcd.OnPropagatedNothing(node[0], node[1]);

  // Assert that the second attempt is ignored
  assert(lcd.NumCycleDetectionAttempts() == 1);
  assert(lcd.NumCyclesDetected() == 0);
  assert(lcd.NumCycleUnifications() == 0);

  // Act 3 - add the edge 3->1 that creates a cycle 3-1-2-3
  successors[node[3]].Insert(node[1]);
  lcd.OnPropagatedNothing(node[3], node[1]);

  // Assert that the cycle was found and unified
  assert(lcd.NumCycleDetectionAttempts() == 2);
  assert(lcd.NumCyclesDetected() == 1);
  assert(lcd.NumCycleUnifications() == 2);
  assert(set.GetUnificationRoot(node[1]) == set.GetUnificationRoot(node[2]));
  assert(set.GetUnificationRoot(node[1]) == set.GetUnificationRoot(node[3]));

  // Act 4 - add the edge 4 -> 0, creating two cycles 4-0-5-4 and 4-0-(1/2/3)-4
  successors[node[4]].Insert(node[0]);
  lcd.OnPropagatedNothing(node[4], node[0]);

  // Assert that both cycles were found.
  // They are only counted as one cycle, but everything should be unified now
  assert(lcd.NumCyclesDetected() == 2);
#ifdef ANDERSEN_NO_FLAGS
  assert(lcd.NumCycleUnifications() == set.NumPointerObjects() - 2);
#else
  assert(lcd.NumCycleUnifications() == set.NumPointerObjects() - 1);
#endif
  for (PointerObjectIndex i = 1; i < sizeof(node)/sizeof(*node); i++)
  {
    assert(set.GetUnificationRoot(node[0]) == set.GetUnificationRoot(node[i]));
  }

  return 0;
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/opt/alias-analyses/TestLazyCycleDetection-TestUnifiesCycles",
    TestUnifiesCycles)
