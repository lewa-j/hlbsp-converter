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
	"src/gltf_export.cpp")

install(TARGETS bsp-converter DESTINATION bin)
