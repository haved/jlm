
set(LIBLLVM_SOURCES
    "backend/jlm2llvm/instruction.cpp"
    "backend/jlm2llvm/jlm2llvm.cpp"
    "backend/jlm2llvm/type.cpp"
    "backend/rvsdg2jlm/rvsdg2jlm.cpp"

    "frontend/ControlFlowRestructuring.cpp"
    "frontend/InterProceduralGraphConversion.cpp"
    "frontend/LlvmInstructionConversion.cpp"
    "frontend/LlvmModuleConversion.cpp"
    "frontend/LlvmTypeConversion.cpp"

    "ir/aggregation.cpp"
    "ir/Annotation.cpp"
    "ir/attribute.cpp"
    "ir/basic-block.cpp"
    "ir/cfg.cpp"
    "ir/cfg-structure.cpp"
    "ir/cfg-node.cpp"
    "ir/domtree.cpp"
    "ir/ipgraph.cpp"
    "ir/ipgraph-module.cpp"
    "ir/operators/alloca.cpp"
    "ir/operators/call.cpp"
    "ir/operators/delta.cpp"
    "ir/operators/GetElementPtr.cpp"
    "ir/operators/lambda.cpp"
    "ir/operators/load.cpp"
    "ir/operators/operators.cpp"
    "ir/operators/Phi.cpp"
    "ir/operators/sext.cpp"
    "ir/operators/store.cpp"
    "ir/print.cpp"
    "ir/RvsdgModule.cpp"
    "ir/ssa.cpp"
    "ir/tac.cpp"
    "ir/types.cpp"
    "ir/variable.cpp"

    "opt/alias-analyses/AgnosticMemoryNodeProvider.cpp"
    "opt/alias-analyses/MemoryStateEncoder.cpp"
    "opt/alias-analyses/MemoryNodeProvider.cpp"
    "opt/alias-analyses/Operators.cpp"
    "opt/alias-analyses/Optimization.cpp"
    "opt/alias-analyses/PointsToGraph.cpp"
    "opt/alias-analyses/RegionAwareMemoryNodeProvider.cpp"
    "opt/alias-analyses/Steensgaard.cpp"
    "opt/cne.cpp"
    "opt/DeadNodeElimination.cpp"
    "opt/inlining.cpp"
    "opt/InvariantValueRedirection.cpp"
    "opt/inversion.cpp"
    "opt/optimization.cpp"
    "opt/OptimizationSequence.cpp"
    "opt/pull.cpp"
    "opt/push.cpp"
    "opt/reduction.cpp"
    "opt/unroll.cpp")

# Don't get confused by the library name, this "llvm" is jlm's own libllvm.a
add_library(llvm STATIC ${LIBLLVM_SOURCES})
jlm_target_options(llvm)

# llvm_interface is the actual LLVM
target_link_libraries(llvm PRIVATE rvsdg util llvm_interface)
