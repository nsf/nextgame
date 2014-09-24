#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <cstdio>

#include "Core/Utils.h"
#include "Core/UTF8.h"
#include "Core/Defer.h"
#include "OOP/EventManager.h"
#include "Math/Vec.h"
#include "Math/Frustum.h"
#include "Math/Noise.h"
#include "Math/Color.h"
#include "OS/Timer.h"
#include "Script/Lua.h"
#include "Script/Events.h"
#include "Script/EventDispatcher.h"
#include "Script/Environment.h"
#include "Render/LuaLoadShader.h"
#include "Render/Meshes.h"
#include "Render/DDS.h"
#include "Render/DeferredShading.h"
#include "GUI/Font.h"
#include "GUI/Atlas.h"
#include "GUI/WindowManager.h"
#include "GUI/WindowManagerDriver.h"
#include "GUI/Cursor.h"
#include "Geometry/HermiteField.h"
#include "Geometry/CubeField.h"
#include "Geometry/DebugDraw.h"
#include "Game/Camera.h"
#include "Map/Storage.h"
#include "Map/Generator.h"
#include "Map/Mutator.h"
#include "Map/Map.h"
#include "OS/WorkerPool.h"
#include "OS/IO.h"
#include "Physics/Bullet.h"
#include "Physics/Character.h"
#include "Render/Material.h"

static int gl_get_int(GLenum what)
{
	int n;
	glGetIntegerv(what, &n);
	return n;
}

static float gl_get_float(GLenum what)
{
	float f;
	glGetFloatv(what, &f);
	return f;
}

struct MyNotMeCallback : btCollisionWorld::ClosestRayResultCallback {
	MyNotMeCallback(btCollisionObject *me, const btVector3 &from, const btVector3 &to):
		btCollisionWorld::ClosestRayResultCallback(from, to), me(me) {}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) override {
		if (rayResult.m_collisionObject == me)
			return 1.0;

		return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
	}

	btCollisionObject *me;
};


static WindowManagerCursorSet load_cursor_set()
{
	WindowManagerCursorSet cs;
	cs.arrow = load_cursor_from_file("textures/arrow.cursor");
	cs.ns = load_cursor_from_file("textures/ns.cursor");
	cs.we = load_cursor_from_file("textures/we.cursor");
	cs.nwse = load_cursor_from_file("textures/nwse.cursor");
	cs.nesw = load_cursor_from_file("textures/nesw.cursor");
	cs.move = load_cursor_from_file("textures/move.cursor");
	return cs;
}

static void generate_shadow_cameras(
	Slice<ShadowCamera> out,
	const PlayerCamera &view, float near, float far, float fov, float aspect,
	const Vec3 &lightdir, float ratio)
{
	Sphere spheres[4];
	generate_frustum_split_spheres(spheres, fov, aspect, near, far, ratio);
	for (int i = 0; i < 4; i++) {
		const Sphere sphere = transform(spheres[i], view.transform);

		ShadowCamera &outcam = out[i];
		outcam.transform.orientation = Quat_LookAt(lightdir);
		const Vec2 texel = Vec2(sphere.diameter()) / Vec2(1024);
		Vec3 center = inverse(outcam.transform.orientation).rotate(sphere.center);
		center.x = std::floor(center.x / texel.x) * texel.x;
		center.y = std::floor(center.y / texel.y) * texel.y;
		center.z += sphere.radius + sphere.radius;
		outcam.transform.translation = outcam.transform.orientation.rotate(center);
		outcam.set_from_sphere(sphere, sphere.radius);
		outcam.apply_transform();
	}
}

struct GameEnvironment : EnvironmentBase {
	ENV_VAR(bool,  wire,            false);
	ENV_VAR(float, threshold,       0.0f);
	ENV_VAR(int,   seed,            0);
	ENV_VAR(bool,  update_map,      false,
		EVF_GUI, "Update Map");
	ENV_VAR(bool,  use_light,       false);

	ENV_VAR(bool, player_moving_forward,  false);
	ENV_VAR(bool, player_moving_left,     false);
	ENV_VAR(bool, player_moving_backward, false);
	ENV_VAR(bool, player_moving_right,    false);

	ENV_VAR(float, sm_factor, 1.0f,
		EVF_GUI, "SM Offset Factor", R"( {type="number", min=1, max=100, increment=0.5} )");
	ENV_VAR(float, sm_units, 100.0f,
		EVF_GUI, "SM Offset Units", R"( {type="number", min=1, max=4096, increment=0.5} )");

	ENV_VAR(bool, cascades_debug, false,
		EVF_GUI, "Cascades Debug");
	ENV_VAR(float, shadow_distance,   300.0f,
		EVF_GUI, "Shadow Distance", R"( {type="number", min=25, max=1500, increment=5} )");
	ENV_VAR(bool,  update_shadow_maps, false,
		EVF_GUI, "Update Shadow Maps");
	ENV_VAR(float, shadow_ratio, 0.2f,
		EVF_GUI, "Shadow Split Ratio", R"( {type="ratio"} )");
	ENV_VAR(int,   tool,          1,
		EVF_PERSISTENT | EVF_GUI, "Tool", R"( {type="choice", list={"Sphere", "Cube"}} )");
	ENV_VAR(int,   material,        1,
		EVF_PERSISTENT | EVF_GUI, "Material", R"( {type="number", min=1, max=7, increment=1, format="%d"} )");
	ENV_VAR(int,   action,          1,
		EVF_PERSISTENT | EVF_GUI, "Action Type", R"( {type="choice", list={"Union", "Difference", "Paint"}} )");
	ENV_VAR(Vec3,  light_color,     Vec3(1));
	ENV_VAR(float, sun_angle,       0.0f,
		EVF_PERSISTENT | EVF_GUI, "Sun Angle", R"( {type="number", min=0, max=180, increment=1} )");
	ENV_VAR(Vec3,  sun_color,       Vec3(1),
		EVF_PERSISTENT | EVF_GUI, "Sun Color", R"( {type="color"} )");
	ENV_VAR(float, sky_turbidity,   4.0f,
		EVF_PERSISTENT | EVF_GUI, "Sky Turbidity", R"( {type="number", min=1, max=10, increment=0.5} )");
	ENV_VAR(float, sky_strength,    1.0f,
		EVF_PERSISTENT | EVF_GUI, "Sky Strength", R"( {type="number", min=0, max=20, increment=0.1} )");
	ENV_VAR(float, exposure,        0.0f,
		EVF_PERSISTENT | EVF_GUI, "Exposure", R"( {type="number", min=-20, max=20, increment=0.1} )");
	ENV_VAR(float, camera_velocity, 1.0f);
};

struct Game : RTTIBase<Game> {
	bool quit = false;
	SDL_Window *win;
	SDL_GLContext ctx;
	Vec2i window_size;
	int mouse_motion_count = 0;
	SDL_MouseMotionEvent last_mouse_motion;
	VertexArray debug_vao;
	PlayerCamera camera;
	double local_time = 0;
	GameEnvironment env;
	UniquePtr<DeferredShading> ds;
	ShadowCamera shadow_cameras[4];

	Font terminus_font;
	Atlas decor_atlas;
	UniquePtr<WindowManager> wm;
	UniquePtr<WindowManagerDriver> wmd;
	UniquePtr<ScriptEventDispatcher> script_event_dispatcher;
	Texture bc1_textures;
	Texture bc4_textures;
	Texture bc5_textures;
	Map::Config map_config;
	Map::StorageConfig map_storage_config;
	UniquePtr<BulletWorld> bullet;
	UniquePtr<Map::Storage> map_storage;
	UniquePtr<Map::Generator> map_generator;
	UniquePtr<Map::Mutator> map_mutator;
	UniquePtr<Character> character;
	UniquePtr<CharacterController> character_controller;
	UniquePtr<Map::Map> map;
	WorldOffset world_offset;

	UniquePtr<MaterialBuffer> material_buf;

	Buffer per_frame0;
	Buffer per_frame1;
	Buffer *per_frame = &per_frame0;

	VertexArray sky_vao;
	VertexArray cube_vao;
	VertexArray capsule_vao;
	bool hit;
	Vec3 hit_position;
	float camera_angle = 0.0f;
	float camera_offset = 0.0f;

	Game();
	~Game();

	void sys_init();
	void main_loop();
	bool can_quit();
	void register_lua_functions();
	void update_async_systems(double delta);
	void update_all(double delta);
	void draw_all();
	template <int variant>
	void draw_layer0(const Frustum &f, bool zero_to_one);
	void on_key(const SDL_KeyboardEvent &ev);
	void on_text_input(const SDL_TextInputEvent &ev);
	void on_mouse_button(const SDL_MouseButtonEvent &ev);
	void on_mouse_motion(const SDL_MouseMotionEvent &ev);
	void on_mouse_motion_delayed();
	void on_mouse_wheel(const SDL_MouseWheelEvent &ev);
	void on_window_event(const SDL_WindowEvent &ev);
	void set_size(int w, int h);
	void set_window_size(int w, int h);
	void info_debug();
	void info();
	void doit();
};

static Game *NG_Game;

void Game::sys_init()
{
	String appdir = IO::get_application_directory();
	IO::make_directories(appdir);

	NG_LuaVM->init_game();
	env.register_all(NG_LuaVM->L);
	register_lua_functions();
	NG_LuaVM->do_file("boot.lua");
	set_window_size(1280, 720);

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		die("Failed to initialize SDL: %s", SDL_GetError());
	}

	// Intel requires specifying core profile.
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	//SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);

	win = SDL_CreateWindow("nextgame",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		window_size.x, window_size.y,
		SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!win) {
		die("Failed to create an SDL window: %s", SDL_GetError());
	}

	ctx = SDL_GL_CreateContext(win);
	if (!ctx) {
		die("Failed to initialize OpenGL 3.3 context: %s", SDL_GetError());
	}

	glewExperimental = GL_TRUE;
	const GLenum err = glewInit();
	if (GLEW_OK != err) {
		die("Failed to initialize GLEW: %s", glewGetErrorString(err));
	}

	printf("Max vertex attributes: %d\n",
		gl_get_int(GL_MAX_VERTEX_ATTRIBS));
	printf("Max varying floats: %d\n",
		gl_get_int(GL_MAX_VARYING_FLOATS));
	printf("Max array texture layers: %d\n",
		gl_get_int(GL_MAX_ARRAY_TEXTURE_LAYERS));
	printf("Max texture image units: %d\n",
		gl_get_int(GL_MAX_TEXTURE_IMAGE_UNITS));
	printf("Max texture anisotropy: %.3f\n",
		gl_get_float(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT));
	printf("Max uniform locations: %d\n",
		gl_get_int(GL_MAX_UNIFORM_LOCATIONS));
	printf("Max vertex uniform components: %d\n",
		gl_get_int(GL_MAX_VERTEX_UNIFORM_COMPONENTS));
	printf("Max geometry uniform components: %d\n",
		gl_get_int(GL_MAX_GEOMETRY_UNIFORM_COMPONENTS));
	printf("Max fragment uniform components: %d\n",
		gl_get_int(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS));
	printf("Max uniform buffer bindings: %d\n",
		gl_get_int(GL_MAX_UNIFORM_BUFFER_BINDINGS));
	printf("Max uniform block size: %d\n",
		gl_get_int(GL_MAX_UNIFORM_BLOCK_SIZE));
	printf("Max vertex uniform blocks: %d\n",
		gl_get_int(GL_MAX_VERTEX_UNIFORM_BLOCKS));
	printf("Max geometry uniform blocks: %d\n",
		gl_get_int(GL_MAX_GEOMETRY_UNIFORM_BLOCKS));
	printf("Max fragment uniform blocks: %d\n",
		gl_get_int(GL_MAX_FRAGMENT_UNIFORM_BLOCKS));

	SDL_GL_SetSwapInterval(1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	//glEnable(GL_FRAMEBUFFER_SRGB);
}

Game::Game()
{
	if (NG_Game)
		die("There can only be one instance of Game");
	NG_Game = this;

	// create window, GL context, init GLEW, prepare initial GL state
	sys_init();

	last_mouse_motion.xrel = 0;
	last_mouse_motion.yrel = 0;
	camera.transform = Transform(
		Vec3(50, 220.722473, 50),
		Quat(0.069682, 0.930201, 0.262344, -0.247074));
	camera.apply_transform();

	ds = make_unique<DeferredShading>(window_size);
	terminus_font = Font_FromFile("textures/terminus.font");
	decor_atlas = Atlas_FromFile("textures/decor.atlas");
	wm = make_unique<WindowManager>(window_size);
	wmd = make_unique<WindowManagerDriver>(wm.get(), load_cursor_set());
	wmd->set_default_cursor();

	{
		Vector<V3C3> vs;
		Vector<uint32_t> is;
		debug_cube(vs, is, Vec3(-0.5), Vec3(0.5));
		cube_vao = create_debug_mesh(vs, is);
	}

	{
		Vector<V3C3> vs;
		Vector<uint32_t> is;
		debug_capsule(vs, is, Vec3(0), 0.3, 1.4, Vec3(1));
		capsule_vao = create_debug_mesh(vs, is);
	}
	{
		sky_vao = create_sphere_mesh(3, 1, true);
	}

	BIND_SHADER(UNIFORM_BLOCKS);
	NG_ResourceCache->update_ub_info();
	dump_uniform_blocks_info(NG_ResourceCache->ub_info);

	material_buf = make_unique<MaterialBuffer>();
	per_frame0 = Buffer(GL_UNIFORM_BUFFER, GL_STREAM_DRAW);
	per_frame1 = Buffer(GL_UNIFORM_BUFFER, GL_STREAM_DRAW);

	bullet = make_unique<BulletWorld>();

	// MapConfig
	map_config.visible_range = Vec3i(9, 4, 9);

	// MapStorageConfig
	map_storage_config.directory = "testworld";
	map_storage = make_unique<Map::Storage>(&map_storage_config);

	// MapGenerator
	map_generator = make_unique<Map::Generator>();

	// MapMutator
	map_mutator = make_unique<Map::Mutator>();

	// Map
	map = make_unique<Map::Map>(&map_config, &world_offset, bullet.get());

	// Character (TODO: leak)
	character = make_unique<Character>(camera.transform.translation);

	// CharacterController
	character_controller = make_unique<CharacterController>(
		character->ghost, character->shape, 0.35f);
	bullet->bt->addCollisionObject(character->ghost,
		btBroadphaseProxy::CharacterFilter,
		btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter);
	bullet->bt->addAction(character_controller.get());
	character_controller->set_gravity(0.0f);

	NG_LuaVM->do_file("init.lua");
	NG_LuaVM->on_event = InterLua::Global(NG_LuaVM->L, "global")["OnEvent"];
	if (!NG_LuaVM->on_event) {
		die("Lua init script must set global.OnEvent with a valid funciton");
	}
	script_event_dispatcher = make_unique<ScriptEventDispatcher>(wm.get(), NG_LuaVM->on_event);

}

Game::~Game()
{
	NG_Game = nullptr;
	SDL_GL_DeleteContext(ctx);
	SDL_DestroyWindow(win);
	SDL_Quit();
}

void Game::main_loop()
{
	Timer mainloop_timer;
	while (!quit) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_WINDOWEVENT:
				on_window_event(e.window);
				break;
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_TEXTINPUT:
				on_text_input(e.text);
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				on_key(e.key);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				on_mouse_button(e.button);
				break;
			case SDL_MOUSEMOTION:
				on_mouse_motion(e.motion);
				break;
			case SDL_MOUSEWHEEL:
				on_mouse_wheel(e.wheel);
				break;
			default:
				break;
			}
		}
		on_mouse_motion_delayed();
		update_all(mainloop_timer.delta());
		draw_all();
		SDL_GL_SwapWindow(win);
		NG_WorkerPool->finalize_tasks();
	}
	map_storage->force_save = true;
	while (!can_quit()) {
		SDL_Delay(16);
		update_async_systems(mainloop_timer.delta());
		NG_WorkerPool->finalize_tasks();
	}
	script_event_dispatcher->on_quit();
}

bool Game::can_quit()
{
	return
		map_storage->can_quit() &&
	map->can_quit();
}

static void ParseMaterialFace(MaterialFace *face, InterLua::Ref val)
{
	face->base_color.x = val["base_color"][1];
	face->base_color.y = val["base_color"][2];
	face->base_color.z = val["base_color"][3];
	face->metallic = val["metallic"];
	face->roughness = val["roughness"];
	face->scale = val["scale"];
	face->bump_scale = val["bump_scale"];
	face->base_color_texture = val["textures"]["base_color"].As<int>() - 1;
	face->normal_texture = val["textures"]["normal"].As<int>() - 1;
	face->metallic_texture = val["textures"]["metallic"].As<int>() - 1;
	face->roughness_texture = val["textures"]["roughness"].As<int>() - 1;
}

static void AddMaterialPack(InterLua::Ref args)
{
	MaterialBuffer &mbuf = *NG_Game->material_buf;
	mbuf.bc1_textures.clear();
	mbuf.bc4_textures.clear();
	mbuf.bc5_textures.clear();

	InterLua::Ref name = args["name"];
	InterLua::Ref textures = args["textures"];
	for (int i = 0, n = args.Length(); i < n; i++) {
		InterLua::Ref material = args[i+1];
		Material &m = mbuf.materials[i];
		ParseMaterialFace(&m.top, material["top"]);
		ParseMaterialFace(&m.side, material["side"]);
	}
	InterLua::Ref bc1 = textures["bc1_textures"];
	for (int i = 0, n = bc1.Length(); i < n; i++)
		mbuf.bc1_textures.append(bc1[i+1].As<const char*>());
	InterLua::Ref bc4 = textures["bc4_textures"];
	for (int i = 0, n = bc4.Length(); i < n; i++)
		mbuf.bc4_textures.append(bc4[i+1].As<const char*>());
	InterLua::Ref bc5 = textures["bc5_textures"];
	for (int i = 0, n = bc5.Length(); i < n; i++)
		mbuf.bc5_textures.append(bc5[i+1].As<const char*>());

	mbuf.upload_materials();
	NG_Game->bc1_textures = load_texture_array_from_dds_files(mbuf.bc1_textures);
	NG_Game->bc4_textures = load_texture_array_from_dds_files(mbuf.bc4_textures);
	NG_Game->bc5_textures = load_texture_array_from_dds_files(mbuf.bc5_textures);
	mbuf.dump(7);
}

void Game::register_lua_functions()
{
	InterLua::GlobalNamespace(NG_LuaVM->L).Namespace("global").
		Function("AddMaterialPack", AddMaterialPack).
	End();
}

void Game::update_async_systems(double delta)
{
	map_storage->update(delta);
	map->update();
	map_mutator->update();
}

void Game::update_all(double delta)
{
	constexpr double standard_frame_time = 1.0 / 60.0;
	constexpr double max_frame_time = standard_frame_time * 10.0;
	if (delta > max_frame_time)
		delta = max_frame_time;

	if (delta > 0.018)
		printf("!!!!!!!!!!!!!!!!!!!!!! PANIC! 60FPS rule violation! (%f)\n", delta);

	script_event_dispatcher->on_environment_sync(env.sync());
	update_async_systems(delta);

	wm->update(delta);
	bullet->update(delta);
	character_controller->update(delta);

	camera.transform.translation = character_controller->interpolated_position();
	camera.apply_transform();

	/*
	Vec3 diff(0);
	if (world_offset.player_position_update(camera.transform.translation, &diff)) {
		map->move();
		camera.transform.translation = camera.transform.translation + diff;
		camera.apply_transform();
		character_controller->relative_warp(diff);
	}
	*/

	if (env.update_map) {
		map->player_position_update(world_offset.local_to_world(
			camera.transform.translation + camera.look_dir * Vec3(1, 0, 1) * Vec3(32.0f)));
	}

	unsigned state = 0;
	if (env.player_moving_forward) state |= PCS_MOVING_FORWARD;
	if (env.player_moving_backward) state |= PCS_MOVING_BACKWARD;
	if (env.player_moving_left) state |= PCS_MOVING_LEFT;
	if (env.player_moving_right) state |= PCS_MOVING_RIGHT;

	character_controller->set_walk_direction(
		get_walk_direction(state, camera.look_dir, camera.right,
			character_controller->gravity() != 0.0f) * Vec3(env.camera_velocity));

	// ray hit tmp
	const btVector3 from = to_bt(camera.transform.translation);
	const btVector3 to = to_bt(camera.transform.translation + camera.look_dir * Vec3(1000.0f));
	MyNotMeCallback cb(character->ghost, from, to);
	cb.m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;
	bullet->bt->rayTest(from, to, cb);
	if (cb.hasHit()) {
		hit = true;
		hit_position = from_bt(cb.m_hitPointWorld);
	} else {
		hit = false;
	}
}

// camera.SetPosition(Vec3(150.097626, 183.849533, -3266.634033))

void Game::draw_all()
{
	wm->redraw();
	unbind_framebuffer();

	//ds->point_lights[0].color = env.light_color;

	const Mat3 iv = to_mat3(camera.transform.orientation);
	const Mat4 vp = camera.projection *
		to_mat4(inverse(camera.transform));
	const Mat4 vprot = camera.projection * to_mat4(inverse(camera.transform.orientation));
	const float aspect = (float)window_size.x / window_size.y;
	const Vec3 sundir = normalize(Vec3(
		0.1,
		std::sin(env.sun_angle * MATH_DEG_TO_RAD),
		std::cos(env.sun_angle * MATH_DEG_TO_RAD)
	));

	static int current_shadow_pass = 0;
	current_shadow_pass = current_shadow_pass^1;

	if (env.update_shadow_maps) {
		generate_shadow_cameras(Slice<ShadowCamera>(shadow_cameras, 4), camera,
			0.25f, env.shadow_distance, 85.0f, aspect, -sundir, env.shadow_ratio);
	}

	const Mat4 shadow_bias(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f
	);
	static Mat4 light_vps[4], shadow_vps[4];
	for (int i = 0; i < 4; i++) {
		if ((i&1) != current_shadow_pass)
			continue;
		light_vps[i] =
			shadow_cameras[i].projection *
			to_mat4(inverse(shadow_cameras[i].transform));
		shadow_vps[i] = shadow_bias * light_vps[i];
	}

	per_frame = per_frame == &per_frame0 ? &per_frame1 : &per_frame0;
	MAP_UNIFORM_BLOCK(PerFrame, *per_frame, {
		SET_UNIFORM_IN_BLOCK(PF_ViewProjection, vp);
		SET_UNIFORM_IN_BLOCK(PF_InverseView, iv);
		SET_UNIFORM_IN_BLOCK(PF_HalfPlaneSize, camera.half_plane_wh);
		SET_UNIFORM_IN_BLOCK(PF_CameraPosition, camera.transform.translation);
		SET_UNIFORM_IN_BLOCK(PF_ProjectionRatio, camera.projection_ratio);
		SET_UNIFORM_IN_BLOCK(PF_AspectRatio, aspect);
		if (env.update_shadow_maps) {
			SET_UNIFORM_IN_BLOCK(PF_ShadowViewProjectionBiased[0], Slice<Mat4>(shadow_vps, 4));
			SET_UNIFORM_IN_BLOCK(PF_ShadowViewProjection[0], Slice<Mat4>(light_vps, 4));
		}
	});

	glViewport(0, 0, window_size.x, window_size.y);
	glClear(GL_DEPTH_BUFFER_BIT);

	arhosek_xyz_skymodelstate_init(&ds->sky_state, env.sky_turbidity, 0.3f, dot(sundir, Vec3_Y()));
	float config_x[9];
	float config_y[9];
	float config_z[9];
	for (int i = 0; i < 9; i++) {
		config_x[i] = ds->sky_state.configs[0][i];
		config_y[i] = ds->sky_state.configs[1][i];
		config_z[i] = ds->sky_state.configs[2][i];
	}
	Vec3 radiance(
		ds->sky_state.radiances[0],
		ds->sky_state.radiances[1],
		ds->sky_state.radiances[2]
	);

	if (env.update_shadow_maps) {
		ds->draw_shadow_map_stage([&](int layer){
			if ((layer&1) != current_shadow_pass)
				return;
			BIND_SHADER(depth_only);
			glClear(GL_DEPTH_BUFFER_BIT);
			SET_UNIFORM(ViewProjection, light_vps[layer]);
			draw_layer0<1>(shadow_cameras[layer].frustum, false);
		}, env.sm_factor, env.sm_units);
	}

	ds->draw_gbuffer_stage([&]{
		BIND_SHADER(layer0);
		draw_layer0<0>(camera.frustum, true);
	});

	ds->draw_sky_stage([&]{
		BIND_SHADER(sky_hw);
		SET_UNIFORM_BLOCK(PerFrame, *per_frame, 0);
		SET_UNIFORM_TEXTURE(DepthTexture, ds->gbuf_depth, 0);
		SET_UNIFORM(Strength, env.sky_strength);
		SET_UNIFORM(SunDirection, sundir);
		SET_UNIFORM(ConfigX, Slice<const float>(config_x));
		SET_UNIFORM(ConfigY, Slice<const float>(config_y));
		SET_UNIFORM(ConfigZ, Slice<const float>(config_z));
		SET_UNIFORM(Radiance, radiance);
		ds->draw_fullscreen_quad();
	});

	ds->mark_frustums({
		shadow_cameras[0].frustum,
		shadow_cameras[1].frustum,
		shadow_cameras[2].frustum,
		shadow_cameras[3].frustum,
	});

	BIND_SHADER(sun_shadow);
	SET_UNIFORM_BLOCK(PerFrame, *per_frame, 0);
	SET_UNIFORM_TEXTURE(DepthTexture, ds->gbuf_depth, 0);
	SET_UNIFORM_TEXTURE(ShadowMap, ds->shadow_map_rt0, 1);
	ds->draw_sun_shadows([&](int cascade) {
		SET_UNIFORM_I(Cascade, cascade);
		ds->draw_fullscreen_quad();
	});

	Vec4 sundir_view = vprot * ToVec4(sundir);
	Vec2 sunpos = (Vec2(sundir_view.x, sundir_view.y) / Vec2(sundir_view.w) + Vec2(1.0)) / Vec2(2.0);
	if (sundir_view.z >= 0.0) {
		for (int i = 0; i < 3; i++) {
			int n = i&1;
			ds->bright_pass_stage([&]{
				BIND_SHADER(zoom_blur);
				SET_UNIFORM_BLOCK(PerFrame, *per_frame, 0);
				SET_UNIFORM(LightPosition, sunpos);
				Texture &t = i == 0 ? ds->hdr_rt0 : ds->bright_pass_tex[n^1];
				SET_UNIFORM_TEXTURE(SourceTexture, t, 0);
				ds->draw_fullscreen_quad();
			}, n);
		}
	} else {
		ds->bright_pass_stage([&]{
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT);
		}, 0);
	}

	ds->draw_lighting_stage([&]{
		constexpr float dist_threshold = 0.5;

		/*
		BIND_SHADER(ds_pointlight);
		SET_UNIFORM_BLOCK(PerFrame, *per_frame, 0);
		SET_UNIFORM_TEXTURE(AlbedoTexture, ds->gbuf_rt0, 0);
		SET_UNIFORM_TEXTURE(SpecularTexture, ds->gbuf_rt1, 1);
		SET_UNIFORM_TEXTURE(NormalTexture, ds->gbuf_rt2, 2);
		SET_UNIFORM_TEXTURE(DepthTexture, ds->gbuf_depth, 3);
		ds->sphere.Bind();
		for (const PointLight &pl : ds->point_lights) {
			const float radius2 = (pl.radius+dist_threshold)*(pl.radius+dist_threshold);
			if (Distance2(camera.position, pl.position) < radius2)
				continue;
			const Mat4 model = Mat4_Translate(pl.position) * Mat4_Scale(pl.radius);
			SET_UNIFORM(Model, model);
			SET_UNIFORM(LightPosition, pl.position);
			SET_UNIFORM(LightColor, pl.color);
			SET_UNIFORM(LightRadius, pl.radius);
			glDrawElements(GL_TRIANGLES, ds->sphere.n, GL_UNSIGNED_INT, 0);
		}
		ds->inv_sphere.Bind();
		for (const PointLight &pl : ds->point_lights) {
			const float radius2 = (pl.radius+dist_threshold)*(pl.radius+dist_threshold);
			if (Distance2(camera.position, pl.position) >= radius2)
				continue;
			const Mat4 model = Mat4_Translate(pl.position) * Mat4_Scale(pl.radius);
			SET_UNIFORM(Model, model);
			SET_UNIFORM(LightPosition, pl.position);
			SET_UNIFORM(LightColor, pl.color);
			SET_UNIFORM(LightRadius, pl.radius);
			glDrawElements(GL_TRIANGLES, ds->inv_sphere.n, GL_UNSIGNED_INT, 0);
		}
		*/

		BIND_SHADER(ds_ambient);
		SET_UNIFORM_BLOCK(PerFrame, *per_frame, 0);
		SET_UNIFORM(SunDirection, sundir);
		SET_UNIFORM(SunColor, srgb_to_linear(env.sun_color));
		SET_UNIFORM_TEXTURE(BaseColorMetallicTexture, ds->gbuf_rt0, 0);
		SET_UNIFORM_TEXTURE(RoughnessTexture, ds->gbuf_rt1, 1);
		SET_UNIFORM_TEXTURE(NormalTexture, ds->gbuf_rt2, 2);
		SET_UNIFORM_TEXTURE(DepthTexture, ds->gbuf_depth, 3);
		SET_UNIFORM_TEXTURE(SunShadowTexture, ds->sun_shadow_rt0, 4);
		ds->draw_fullscreen_quad();
	});

	if (env.cascades_debug) {
		const Vec3 colors[] = {
			Vec3(0.2, 0.0, 0.0),
			Vec3(0.0, 0.2, 0.0),
			Vec3(0.0, 0.0, 0.2),
			Vec3(0.2, 0.2, 0.0),
		};
		BIND_SHADER(color);
		ds->draw_sun_shadows_debug([&](int cascade) {
			SET_UNIFORM(Color, colors[cascade]);
			ds->draw_fullscreen_quad();
		});
	}

	ds->draw_post_process_stage([&]{
		BIND_SHADER(ds_postprocess);
		SET_UNIFORM_BLOCK(PerFrame, *per_frame, 0);
		SET_UNIFORM_TEXTURE(HDRTexture, ds->hdr_rt0, 0);
		SET_UNIFORM_TEXTURE(BrightTexture, ds->bright_pass_tex[0], 1);
		SET_UNIFORM(Exposure, env.exposure);
		ds->draw_fullscreen_quad();
	});

	if (hit) {
		const Vec3d wp = world_offset.local_to_world(hit_position);
		const Map::Position pos(wp);
		const Vec3 vpos = ToVec3(pos.chunk - world_to_chunk(world_offset.offset)) * chunk_size(0) + ToVec3(pos.floor_voxel) * CUBE_SIZE;
		ds->draw_transparent_stage([&]{
			BIND_SHADER(color_alpha);
			SET_UNIFORM(Color, Vec4(1, 0, 0, 0.05));
			SET_UNIFORM_TEXTURE(DepthTexture, ds->gbuf_depth, 0);
			const Vec3 min = vpos;
			const Vec3 max = vpos + CUBE_SIZE;
			//ds->draw_box(min, max);
			SET_UNIFORM(Color, Vec4(0, 1, 0, 0.1));
			ds->draw_box(hit_position - Vec3(0.1), hit_position + Vec3(0.1));
		});
	}

	BIND_SHADER(debug);
	if (debug_vao.n > 0) {
		debug_vao.bind();
		SET_UNIFORM(ModelViewProj, vp);
		glDrawElements(GL_LINES, debug_vao.n, GL_UNSIGNED_INT, nullptr);
	}

	wm->composite();
}

template <int variant>
void Game::draw_layer0(const Frustum &frustum, bool zero_to_one)
{
	SET_UNIFORM_BLOCK(PerFrame, *per_frame, 0);
	SET_UNIFORM_BLOCK(Materials, material_buf->buffer, 1);
	SET_UNIFORM_TEXTURE(BC1Textures, bc1_textures, 0);
	SET_UNIFORM_TEXTURE(BC4Textures, bc4_textures, 1);
	SET_UNIFORM_TEXTURE(BC5Textures, bc5_textures, 2);

	map->current->vao.bind();
	int drawn = 0;
	int total = 0;

	FrustumCullingType cull_type = FCT_NORMAL;
	if (!zero_to_one)
		cull_type = FCT_NO_NEAR_PLANE;

	auto draw_lod = [&](int lod) {
		for (auto kv : map->current->geometry) {
			const Map::ChunkMesh *mesh = kv.value;
			if (mesh->indices.length() == 0)
				continue;
			if (mesh->lods[7] != lod)
				continue;

			const Vec3i chunk_offset = kv.key - world_to_chunk(world_offset.offset);
			const Vec3 offset = ToVec3(chunk_offset * CHUNK_SIZE) * CUBE_SIZE;
			const Vec3 min = offset - ToVec3(CHUNK_SIZE) * CUBE_SIZE;
			const Vec3 max = offset + ToVec3(Vec3i(lod_factor(mesh->lods[7])) * CHUNK_SIZE) * CUBE_SIZE;
			total += mesh->indices.length()/3;
			if (frustum.cull(min, max, cull_type) == FS_OUTSIDE)
				continue;
			drawn += mesh->indices.length()/3;

			const Mat4 m = Mat4_Translate(offset);
			SET_UNIFORM(Model, m);
			glMultiDrawElementsBaseVertex(GL_TRIANGLES,
				mesh->count.data(), GL_UNSIGNED_INT, mesh->base_index.data(),
				mesh->count.length(), mesh->base_vertex.data());
		}
	};

	if (zero_to_one) {
		for (int i = 0; i < LODS_N; i++)
			draw_lod(i);
	} else {
		for (int i = LODS_N-1; i >= 0; i--)
			draw_lod(i);
	}
	//printf("(%d) drawn: %d, total: %d\n", variant, drawn, total);
}

void Game::on_key(const SDL_KeyboardEvent &ev)
{
	script_event_dispatcher->on_keyboard_event(ev);
}

void Game::on_text_input(const SDL_TextInputEvent &ev)
{
	script_event_dispatcher->on_text_input_event(ev);
}

void Game::on_mouse_button(const SDL_MouseButtonEvent &ev)
{
	wmd->on_mouse_button(ev);
	script_event_dispatcher->on_mouse_button(ev);
}

void Game::on_mouse_motion(const SDL_MouseMotionEvent &ev)
{
	const Vec2i prevrel(last_mouse_motion.xrel, last_mouse_motion.yrel);
	last_mouse_motion = ev;
	last_mouse_motion.xrel = prevrel.x + ev.xrel;
	last_mouse_motion.yrel = prevrel.y + ev.yrel;
	mouse_motion_count++;
}

void Game::on_mouse_motion_delayed()
{
	if (mouse_motion_count == 0)
		return;
	mouse_motion_count = 0;

	Window *resized = nullptr;
	wmd->on_mouse_motion(last_mouse_motion, &resized);
	if (resized) {
		ScriptEventTermboxResize rev;
		rev.window = resized;
		rev.size = resized->termbox_window->termbox->size();
		NG_LuaVM->on_event(SE_TERMBOX_RESIZE, &rev);
	}

	script_event_dispatcher->on_mouse_motion(last_mouse_motion);

	last_mouse_motion.xrel = 0;
	last_mouse_motion.yrel = 0;
}

void Game::on_mouse_wheel(const SDL_MouseWheelEvent &ev)
{
	script_event_dispatcher->on_mouse_wheel(ev);
}

void Game::on_window_event(const SDL_WindowEvent &ev)
{
	switch (ev.event) {
	case SDL_WINDOWEVENT_RESIZED:
		set_size(ev.data1, ev.data2);
		break;
	}
}

void Game::set_size(int w, int h)
{
	set_window_size(w, h);
	wm->resize(window_size);
	ds->set_size(window_size);
}

void Game::set_window_size(int w, int h)
{
	window_size = {w, h};
	camera.set_perspective(85.0f, (float)w/h, 0.25f, 1500.0f);
	camera.apply_transform();
}

void Game::info_debug()
{
	if (debug_draw.vertices.length() > 0)
		debug_vao = create_debug_mesh(debug_draw.vertices, debug_draw.indices);
	debug_draw.clear();
}

void Game::info()
{
	const Mat4 v = to_mat4(inverse(camera.transform));
	const Mat4 vi = to_mat4(camera.transform);
	v.dump();
	vi.dump();
	inverse(v).dump();

	int tris_visible = 0;
	for (auto kv : map->current->geometry) {
		const Map::ChunkMesh *mesh = kv.value;
		if (mesh->indices.length() == 0)
			continue;

		const Vec3i chunk_offset = kv.key - world_to_chunk(world_offset.offset);
		const Vec3 offset = ToVec3(chunk_offset * CHUNK_SIZE) * CUBE_SIZE;
		const Vec3 min = offset - ToVec3(CHUNK_SIZE) * CUBE_SIZE;
		const Vec3 max = offset + ToVec3(Vec3i(lod_factor(mesh->lods[7])) * CHUNK_SIZE) * CUBE_SIZE;
		if (camera.frustum.cull(min, max) != FS_OUTSIDE)
			tris_visible += mesh->indices.length() / 3;
	}
	printf("Visible triangles: %d\n", tris_visible);

	const Vec3 orig = character_controller->interpolated_position();
	debug_draw.line(orig, orig+Vec3_X(5), Vec3_X());
	debug_draw.line(orig, orig+Vec3_Y(5), Vec3_Y());
	debug_draw.line(orig, orig+Vec3_Z(5), Vec3_Z());

	printf("camera pos: %f, %f, %f\n", VEC3(camera.transform.translation));
	printf("camera rot: %f, %f, %f, %f\n", VEC4(camera.transform.orientation));
	const Map::Position p(world_offset.local_to_world(camera.transform.translation));
	printf("Chunk: %d %d %d, Point: %f %f %f\n", VEC3(p.chunk), VEC3(p.point));

	Quat tmp_r(Vec3_Y(), camera_angle);
	Vec3 tmp = tmp_r.rotate(Vec3_Z(1));
	debug_draw.line(orig, orig + tmp * Vec3(5), Vec3(1));
	Vec3 tmp2 = cross(tmp, Vec3_Y());
	debug_draw.line(orig, orig + tmp2 * Vec3(5), Vec3(1));

	if (debug_draw.vertices.length() > 0) {
		printf("Debug data uploaded: %d lines\n", debug_draw.indices.length() / 2);
		debug_vao = create_debug_mesh(debug_draw.vertices, debug_draw.indices);
	}
	debug_draw.clear();

	printf("Info done\n");
}

void Game::doit()
{
	if (!hit)
		return;

	map_mutator->mutate({
		Map::Position(world_offset.local_to_world(hit_position)),
		(Map::MutatorAction)env.action,
		(Map::MutatorTool)env.tool,
		env.material});
	/*
	int n = ds->point_lights.Length() + 1;
	Vector<Vec3> random_colors(n);
	GenerateRandomColors(random_colors);
	ds->point_lights.Append({
		character_controller->InterpolatedPosition(),
		random_colors[n-1],
		4.0f,
	});
	*/
}

int main(int, char**)
{
	// GLOBALS
	CubeFieldIndices cube_field_indices;
	EventManager event_manager;
	WorkerPool worker_pool;
	LuaVM lua_vm(LUA_VM_GLOBAL);
	ResourceCache resource_cache;

	Game game;
	game.main_loop();
	//GenerateMarchingCubes();
	return 0;
}

NG_LUA_API Window *NG_Game_AddWindow(const Vec2i &pos,
	WindowAlignment alignment, bool decorate)
{
	auto tb = make_unique<Termbox>(
		&NG_Game->terminus_font, &NG_Game->terminus_font, Vec2i(80, 24));
	auto tbwin = make_unique<TermboxWindow>(
		std::move(tb), decorate ? &NG_Game->decor_atlas : nullptr);
	return NG_Game->wm->add_window(std::move(tbwin), alignment, pos);
}

NG_LUA_API void NG_Game_RemoveWindow(Window *win)
{
	NG_Game->wm->remove_window(win);
	NG_Game->script_event_dispatcher->on_window_delete(win);
}

NG_LUA_API void NG_Game_RaiseWindow(Window *win)
{
	NG_Game->wm->raise(win);
}

NG_LUA_API void NG_Game_EnableGUI(bool b)
{
	if (b) {
		NG_Game->wm->active = true;
	} else {
		NG_Game->wm->active = false;
		NG_Game->wmd->set_default_cursor();
	}
}

NG_LUA_API Vec2i NG_Game_CursorPosition()
{
	return NG_Game->script_event_dispatcher->last_mouse_position;
}

NG_LUA_API void NG_Game_Info() { NG_Game->info(); }
NG_LUA_API void NG_Game_Do() { NG_Game->doit(); }
NG_LUA_API void NG_Game_InfoDebug() { NG_Game->info_debug(); }
NG_LUA_API void NG_Game_RotateCamera(float x, float y)
{
	NG_Game->camera.transform.orientation =
		mouse_rotate(NG_Game->camera.transform.orientation, x, y, 0.25f);
	NG_Game->camera.apply_transform();
}
NG_LUA_API void NG_Game_Jump() { NG_Game->character_controller->jump(); }
NG_LUA_API void NG_Game_ToggleGravity()
{
	if (NG_Game->character_controller->gravity() == 0.0f)
		NG_Game->character_controller->set_gravity(9.8f * 3);
	else
		NG_Game->character_controller->set_gravity(0.0f);
}
