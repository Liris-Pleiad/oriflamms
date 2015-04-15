# Seth build script

# add path to oriflamms_config.h
include_directories("${PROJECT_BINARY_DIR}/oriflamms")
# add path to oriflamms
include_directories("${PROJECT_SOURCE_DIR}/utils")
# add path to dependencies
include_directories(${LIBGTKCRNMM_INCLUDE_DIRS})
link_directories(${LIBGTKCRNMM_LIBRARY_DIRS})

# create entities
add_executable(entities entities.cpp)
set_target_properties(entities PROPERTIES COMPILE_FLAGS "${GTKCRNMM_CFLAGS_OTHER}" LINK_FLAGS "${GTKCRNMM_LDFLAGS_OTHER} -lstdc++")
# add dependencies
if(UNIX)
	target_link_libraries(entities ${LIBGTKCRNMM_LIBRARIES})
endif(UNIX)
if(MSVC)
	target_link_libraries(entities debug "${LIBGTKCRNMMDEBUG_LIBRARIES}" optimized "${LIBGTKCRNMM_LIBRARIES}")
endif(MSVC)

# create entities
add_executable(signatures signatures.cpp)
set_target_properties(signatures PROPERTIES COMPILE_FLAGS "${GTKCRNMM_CFLAGS_OTHER}" LINK_FLAGS "${GTKCRNMM_LDFLAGS_OTHER} -lstdc++")
# add dependencies
if(UNIX)
	target_link_libraries(signatures ${LIBGTKCRNMM_LIBRARIES})
endif(UNIX)
if(MSVC)
	target_link_libraries(signatures debug "${LIBGTKCRNMMDEBUG_LIBRARIES}" optimized "${LIBGTKCRNMM_LIBRARIES}")
endif(MSVC)

