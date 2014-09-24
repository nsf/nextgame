#pragma once

#include "Render/OpenGL.h"
#include "Geometry/WorldOffset.h"
#include "Geometry/VertexFormats.h"
#include "Core/HashMap.h"
#include "OOP/EventManager.h"
#include "Map/Config.h"
#include "OS/Timer.h"
#include "Physics/Bullet.h"

namespace Map {

struct ChunkMesh {
	Vector<V3N3M1_terrain> vertices;
	Vector<uint32_t> indices;
	int ref_count = 1;
	int lods[8];

	struct {
		int voffset;
		int ioffset;
	} current, next;

	Vector<GLsizei> count; // number of indices in each mesh
	Vector<GLint> base_vertex; // offset into vertex buffer of each mesh
	Vector<const GLvoid*> base_index; // offset into index buffer of each mesh

	btTriangleIndexVertexArray *parray = nullptr;
	btBvhTriangleMeshShape *pshape = nullptr;
	btCollisionObject *pobject = nullptr;

	ChunkMesh(int lods[8]);
	~ChunkMesh();

	// swaps current and next, adjusting bases along the way
	void adjust();

	int vertices_byte_offset() const { return current.voffset * sizeof(vertices[0]); }
	int indices_byte_offset() const { return current.ioffset; }
};

struct StateVAO {
	GLVertexArray id;
	Buffer vbo;
	Buffer ibo;
	int vbo_len = 0;
	int ibo_len = 0;

	NG_DELETE_COPY_AND_MOVE(StateVAO);
	StateVAO();

	void bind_vertex_attributes();
	void resize(int vsize, int isize);
	void append_vertices(Slice<const uint8_t> data);
	void append_indices(Slice<const uint8_t> data);
	void append_existing(const StateVAO &r, ChunkMesh *mesh);
	void bind() const;
	void clear();
};

struct State {
	HashMap<Vec3i, ChunkMesh*> geometry;
	StateVAO vao;

	NG_DELETE_COPY_AND_MOVE(State);
	State() = default;
	~State();
};

struct UpdatedChunks {
	Vec3i min;
	Vec3i max;
};

struct Map : RTTIBase<Map> {
	Vector<UpdatedChunks> updated_chunks;
	const Config *config = nullptr;
	const WorldOffset *offset = nullptr;
	BulletWorld *btworld = nullptr;
	int queued_geometry = 0;
	State states[2];
	State *current = &states[0];
	State *next = &states[1];
	Vec3i last_player_chunk = Vec3i(9999999);
	Timer t_map_update = Timer(TA_DONT_START);

	NG_DELETE_COPY_AND_MOVE(Map);
	Map(const Config *config, const WorldOffset *offset, BulletWorld *btworld);
	~Map();

	bool can_quit();
	void move();
	void player_position_update(const Vec3d &wp);
	void finalize_map_update();

	void handle_chunks_updated(RTTIObject *event);
	void handle_map_storage_response(RTTIObject *event);
	void handle_map_chunk_geometry_generated(RTTIObject *event);

	void update();
};

} // namespace Map
