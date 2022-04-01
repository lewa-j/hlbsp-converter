import bpy
from io_scene_gltf2.io.com.gltf2_io_extensions import Extension
from io_scene_gltf2.io.com.gltf2_io import TextureInfo
from io_scene_gltf2.blender.imp.gltf2_blender_texture import texture
from io_scene_gltf2.blender.imp.gltf2_blender_pbrMetallicRoughness import MaterialHelper

bl_info = {
	"name": "glTF lightmap Importer Extension",
	"category": "Import",
	"version": (1, 1, 0),
	"blender": (3, 1, 0),
	'location': 'File > Import > glTF 2.0',
	'description': 'Addon to add a lightmap feature to an imported glTF file.',
	'isDraft': True,
	'developer': "lewa_j",
	'url': 'https://github.com/lewa-j',
}

glTF_extension_name = "EXT_materials_lightmap"

class glTF2ImportUserExtension:

	def __init__(self):
		self.extensions = [Extension(name="EXT_materials_lightmap", extension={}, required=False)]
		pass

	def gather_import_material_before_hook(self, gltf_material, vertex_color, import_settings):
		pass

	def gather_import_material_after_hook(self, gltf_material, vertex_color, blender_mat, import_settings):
		try:
			ext = gltf_material.extensions['EXT_materials_lightmap']
		except Exception:
			return

		tex_info = ext.get('lightmapTexture')
		if tex_info is not None:
			tex_info = TextureInfo.from_dict(tex_info)

		if tex_info is None:
			return

		parent = blender_mat.node_tree.nodes[0]
		emission_socket = parent.inputs['Emission']
		base_socket = parent.inputs['Base Color']

		base_color_node = None
		if len(base_socket.links)!=0:
			base_color_node = base_socket.links[0].from_socket.node

		#if len(emission_socket.links)==0:
		#	return
		#emission_node = emission_socket.links[0].from_socket.node
		
		node = blender_mat.node_tree.nodes.new('ShaderNodeMixRGB')
		node.location = -150, 0
		node.blend_type = 'MULTIPLY'
		# Outputs
		blender_mat.node_tree.links.new(emission_socket, node.outputs[0])
		# Inputs
		node.inputs['Fac'].default_value = 1.0
		if base_color_node is not None:
			blender_mat.node_tree.links.new(node.inputs['Color1'], base_color_node.outputs[0])
		#blender_mat.node_tree.links.new(node.inputs['Color2'], emission_node.outputs[0])
		
		mh = MaterialHelper(import_settings, gltf_material, blender_mat, vertex_color)
		
		texture(
			mh,
			tex_info=tex_info,
			label='LIGHTMAP',
			location=(-200, -80),
			is_data=False,
			color_socket=node.inputs['Color2'],
		)

def register():
	pass

def unregister():
	pass