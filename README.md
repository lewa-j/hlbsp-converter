# hlbsp-converter

A tool to convert bsp maps (Half-Life and other GoldSrc games) into gltf scenes.

## Key features:

* Export embedded texture
* Lightmaps!
* Option to exclude sky polygons

## Usage

```sh
./bsp-converter map-name.bsp [options]
```

### Options

* `-lm <number>` - set a lightmap atlas size
* `-skip_sky` - exclude polygons with 'sky' texture from export 
* `-lstyle <number>|all` - export lightmap with a specified lightstyle index or all lightyles in one

## Dependencies (already included)

* [nlohmann/json](https://github.com/nlohmann/json)
* [stb_image_write](https://github.com/nothings/stb)

## Acknowledgments

* id for [Quake 2](https://github.com/id-Software/Quake-2)
* UnkleMike and FWGS for [Xash3D](https://github.com/FWGS/xash3d-fwgs)

## License

This project is licensed under the MIT License - see the LICENSE file for details
