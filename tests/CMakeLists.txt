set(COMMON_TEST_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

function(nextgame_test NAME)
	add_executable(${NAME} "${NAME}.cpp")
	target_link_libraries(${NAME} NG)
	add_test(${NAME} ${NAME})
endfunction()

add_subdirectory(GUI)
add_subdirectory(Core)
add_subdirectory(OS)
add_subdirectory(Serialize)
