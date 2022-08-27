# Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
cmake_minimum_required (VERSION 3.8)

project ("bsp-converter")

add_executable (bsp-converter
	"src/bsp-converter.h"
	"src/bsp-converter.cpp"
	"src/hlbsp.h"
	"src/hlbsp.cpp"
	"src/texture.h"
	"src/texture.cpp"
	"src/lightmap.h"
	"src/lightmap.cpp"
	"src/gltf_export.h"
	"src/gltf_export.cpp"
	"src/sourcebsp.h"
	"src/sourcebsp.cpp"
	"src/map.h"
	"src/map.cpp"
	"src/wad.h"
	"src/wad.cpp"
	"src/mip_texture.cpp"
	"src/parser.h"
	"src/parser.cpp"
	"src/vector_math.h"
	"src/vtf.h"
	"src/vtf_texture.cpp"
	"src/rgbcx.cpp"
	"src/vpk.h"
	"src/vpk.cpp"
	)

if(MSVC)
  target_compile_definitions(bsp-converter PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()

install(TARGETS bsp-converter DESTINATION bin)
