cmake_minimum_required(VERSION 3.19)

project(csgfx 
	VERSION 0.1.0
	DESCRIPTION "TBD"
	LANGUAGES C CXX)

find_package(SDL2 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(csgfx_app 
	src/main.cpp
	src/app.cpp
	src/renderer.cpp
	src/mesh.cpp
	src/logger.cpp)


target_link_libraries(csgfx_app 
		PRIVATE
		SDL2::SDL2main			
		SDL2::SDL2
		spdlog::spdlog
		glm::glm
		assimp::assimp
		imgui::imgui)

