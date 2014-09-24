-- default material values:
-- {
--   base_color = {1, 1, 1},
--   scale = 0.25,
--   metallic = 0.0,
--   roughness = 1.0,
-- }
--
-- scale means which part of the texture covers 1 meter of space
-- 0.25 means the full texture covers 4 meters

{
	name = "terrain",
	{
		name = "Gravel",
		top = {
			base_color = "textures/Gravel_512.dds",
			normal = "textures/Gravel_512_nm.dds",
		},
		side = {
			base_color = "textures/Cliffs2_512.dds",
			normal = "textures/Cliffs2_512_nm.dds",
			roughness = 0.7,
		},
	},
	{
		name = "Bricks",
		top = {
			base_color = "textures/Brick_512.dds",
			normal = "textures/Brick_512_nm.dds",
		},
	},
	{
		name = "Cliffs",
		top = {
			base_color = "textures/Cliffs_512.dds",
			normal = "textures/Cliffs_512_nm.dds",
			roughness = 1.0,
		},
	},
	{
		name = "Grass and Dirt",
		top = {
			base_color = "textures/grass_512_diffuse.dds",
			normal = "textures/grass_512_normal.dds",
		},
		side = {
			base_color = "textures/rock_512_diffuse.dds",
			normal = "textures/rock_512_normal.dds",
			roughness = 0.7,
		},
	},
	{
		name = "Test",
		top = {
			base_color = "textures/test_diffuse.dds",
			normal = "textures/test_normal.dds",
		},
	},
	{
		name = "Gold",
		top = {
			base_color = {0.9725490196, 0.7568627450, 0.30980392156},
			metallic = 1.0,
			roughness = 0.3,
		}
	},
	{
		name = "Metal Plate",
		top = {
			base_color = "textures/metal_plate_512_diffuse.dds",
			normal = "textures/metal_plate_512_normal.dds",
			roughness = "textures/metal_plate_512_roughness.dds",
			metallic = "textures/metal_plate_512_metallic.dds",
			bump_scale = 2.0,
			scale = 0.5,
		}
	}
}
