set(LIBUTIL_SOURCES
	"callbacks.cpp"
	"common.cpp"
	"Statistics.cpp")

add_library(util STATIC ${LIBUTIL_SOURCES})
jlm_target_options(util)
