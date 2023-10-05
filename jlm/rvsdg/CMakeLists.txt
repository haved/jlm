set(LIBRVSDG_SOURCES
	"binary.cpp"
	"control.cpp"
	"gamma.cpp"
	"graph.cpp"
	"node-normal-form.cpp"
	"node.cpp"
	"notifiers.cpp"
	"nullary.cpp"
	"operation.cpp"
	"record.cpp"
	"region.cpp"
	"simple-normal-form.cpp"
	"simple-node.cpp"
	"statemux.cpp"
	"structural-normal-form.cpp"
	"structural-node.cpp"
	"theta.cpp"
	"tracker.cpp"
	"traverser.cpp"
	"type.cpp"
	"unary.cpp"
	"view.cpp"

	"bitstring/arithmetic.cpp"
	"bitstring/bitoperation-classes.cpp"
	"bitstring/comparison.cpp"
	"bitstring/concat.cpp"
	"bitstring/constant.cpp"
	"bitstring/slice.cpp"
	"bitstring/type.cpp"
	"bitstring/value-representation.cpp" rvsdg2html.cpp rvsdg2html.hpp)

add_library(rvsdg STATIC ${LIBRVSDG_SOURCES})
jlm_target_options(rvsdg)
target_link_libraries(rvsdg PRIVATE util)
