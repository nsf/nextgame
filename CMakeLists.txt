project(NEXTGAME)
cmake_minimum_required(VERSION 2.8)

set(NEXTGAME_EXEC_NAME "nextgame")
set(NEXTGAME_SANITIZE "" CACHE STRING "Sanitizer address/memory/thread")
set(NEXTGAME_LOCAL_STATIC_LIB_PATH "~/local/lib" CACHE PATH "Path to static local libraries")
set(NEXTGAME_LOCAL_DYNAMIC_LIB_PATH "~/local/lib" CACHE PATH "Path to dynamic local libraries")
set(NEXTGAME_LOCAL_INCLUDE_BASE_PATH "~/local/include" CACHE PATH "Path to local includes dir")
set(NEXTGAME_BUILD_TESTS NO CACHE BOOL "Build tests")

if ("${NEXTGAME_SANITIZE}" STREQUAL "address")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
elseif("${NEXTGAME_SANITIZE}" STREQUAL "memory")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory -fno-omit-frame-pointer")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=memory")
elseif("${NEXTGAME_SANITIZE}" STREQUAL "thread")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fno-omit-frame-pointer -fPIE -pie")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread -fPIE -pie")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "") # suppress -rdynamic

#=======================================================================
# FIND LIBRARIES
#=======================================================================

if (WIN32)
	find_library(SDL2_MAIN_LIB
		NAMES SDL2main
		PATHS ${NEXTGAME_LOCAL_STATIC_LIB_PATH}
		NO_DEFAULT_PATH)
endif()

find_library(SDL2_LIB
	NAMES SDL2
	PATHS ${NEXTGAME_LOCAL_DYNAMIC_LIB_PATH}
	NO_DEFAULT_PATH)
find_library(PNG_LIB
	NAMES png png16
	PATHS ${NEXTGAME_LOCAL_DYNAMIC_LIB_PATH}
	NO_DEFAULT_PATH)
find_library(GLEW_LIB
	NAMES GLEW glew32
	PATHS ${NEXTGAME_LOCAL_DYNAMIC_LIB_PATH}
	NO_DEFAULT_PATH)
find_library(LUAJIT_LIB
	NAMES luajit-5.1 lua51
	PATHS ${NEXTGAME_LOCAL_DYNAMIC_LIB_PATH}
	NO_DEFAULT_PATH)
find_library(BULLET_DYNAMICS_LIB
	NAMES BulletDynamics
	PATHS ${NEXTGAME_LOCAL_STATIC_LIB_PATH}
	NO_DEFAULT_PATH)
find_library(BULLET_COLLISION_LIB
	NAMES BulletCollision
	PATHS ${NEXTGAME_LOCAL_STATIC_LIB_PATH}
	NO_DEFAULT_PATH)
find_library(BULLET_LINEARMATH_LIB
	NAMES LinearMath
	PATHS ${NEXTGAME_LOCAL_STATIC_LIB_PATH}
	NO_DEFAULT_PATH)
find_library(LZ4_LIB
	NAMES lz4
	PATHS ${NEXTGAME_LOCAL_DYNAMIC_LIB_PATH}
	NO_DEFAULT_PATH)
find_library(CAIRO_LIB
	NAMES cairo
	PATHS ${NEXTGAME_LOCAL_DYNAMIC_LIB_PATH}
	NO_DEFAULT_PATH)

#=======================================================================
# INCLUDES
#=======================================================================

set(NEXTGAME_EXTERNAL_INCLUDES
	${NEXTGAME_LOCAL_INCLUDE_BASE_PATH}/luajit-2.0
	${NEXTGAME_LOCAL_INCLUDE_BASE_PATH}/bullet
	${NEXTGAME_LOCAL_INCLUDE_BASE_PATH})

#=======================================================================
# LIBS
#=======================================================================

if (WIN32)
	list(APPEND NEXTGAME_EXTERNAL_LIBRARIES
		mingw32
		${SDL2_MAIN_LIB})
endif()

list(APPEND NEXTGAME_EXTERNAL_LIBRARIES
	${SDL2_LIB}
	${PNG_LIB}
	${GLEW_LIB}
	${LUAJIT_LIB}
	${BULLET_DYNAMICS_LIB}
	${BULLET_COLLISION_LIB}
	${BULLET_LINEARMATH_LIB}
	${LZ4_LIB}
	${CAIRO_LIB})

if (WIN32)
	list(APPEND NEXTGAME_EXTERNAL_LIBRARIES
		winmm
		opengl32)
else()
	list(APPEND NEXTGAME_EXTERNAL_LIBRARIES
		GL)
endif()

set(NEXTGAME_SOURCE_ROOT
	${CMAKE_SOURCE_DIR}/source
	${CMAKE_BINARY_DIR}/source)
add_subdirectory(source)

if (NEXTGAME_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()
