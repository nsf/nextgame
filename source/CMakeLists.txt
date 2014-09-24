function(add_nextgame_subdir NAME)
	file(GLOB SOURCES "${NAME}/*.c" "${NAME}/*.cpp" "${NAME}/*.h")
	list(APPEND ALL_SOURCES ${SOURCES})
	set(ALL_SOURCES "${ALL_SOURCES}" PARENT_SCOPE)
endfunction()

add_nextgame_subdir(Core/Trio)
add_nextgame_subdir(Core)
add_nextgame_subdir(Render)
add_nextgame_subdir(Math)
add_nextgame_subdir(GUI)
add_nextgame_subdir(Geometry)
add_nextgame_subdir(OS)
add_nextgame_subdir(NewMap)
add_nextgame_subdir(Map)
add_nextgame_subdir(Script)
add_nextgame_subdir(Game)
add_nextgame_subdir(OOP)
add_nextgame_subdir(Serialize)
add_nextgame_subdir(Serialize/Cmp)
add_nextgame_subdir(Serialize/MsgPack)
add_nextgame_subdir(Physics)

file(GLOB SOURCES "*.cpp" "*.h")

include_directories(${NEXTGAME_EXTERNAL_INCLUDES} ${NEXTGAME_SOURCE_ROOT})

add_library(NG ${ALL_SOURCES})
add_executable(${NEXTGAME_EXEC_NAME} ${SOURCES})

target_link_libraries(${NEXTGAME_EXEC_NAME} NG ${NEXTGAME_EXTERNAL_LIBRARIES})

if (NOT WIN32)
	set_target_properties(${NEXTGAME_EXEC_NAME} PROPERTIES LINK_FLAGS
		-Wl,--dynamic-list=${CMAKE_CURRENT_SOURCE_DIR}/symbols.txt)
endif()
