# hlbsp-converter

A tool to convert bsp maps (Half-Life and other GoldSrc games) into glTF scenes.

## Key features:

* Export embedded texture, and optionally from wads
* Lightmaps!
* Option to exclude sky polygons
* BSP31 support (Xash3D)
* Basic VBSP support (Source Engine)

## Usage

```sh
./bsp-converter map-name.bsp [options]
```

### Options

* `-lm <number>` - set a maximum lightmap atlas size (default 2048). Actual size is calculated based on surfaces and can be smaller.
* `-skip_sky` - exclude polygons with 'sky' texture from export 
* `-lstyle <number>|all` - export lightmap with a specified lightstyle index or all lightyles in one.
* `-uint16` - sets index buffer type to usigned short. Useful for old mobile GPU without GL_OES_element_index_uint. Will split models into smaller meshes if required.
* `-tex` - export all textures, including loaded from wads.
* `-game <path>` - directory containing wads

## Dependencies (already included)

* [nlohmann/json](https://github.com/nlohmann/json)
* [stb_image_write](https://github.com/nothings/stb)

## Acknowledgments

* id for [Quake 2](https://github.com/id-Software/Quake-2)
* UnkleMike and FWGS for [Xash3D](https://github.com/FWGS/xash3d-fwgs)

## License

This project is licensed under the MIT License - see the LICENSE file for details
