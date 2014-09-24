#pragma once

#include "OOP/EventManager.h"
#include "Math/Vec.h"
#include "Geometry/HermiteField.h"
#include "Map/Map.h"
#include "Map/Position.h"

namespace Map {

enum MutatorAction {
	MA_UNION = 1,
	MA_DIFFERENCE = 2,
	MA_PAINT = 3,
};

enum MutatorTool {
	MT_SPHERE = 1,
	MT_CUBE = 2,
};

struct MutatorBatch {
	Position position;
	MutatorAction action;
	MutatorTool tool;
	int material;
};

struct Mutator : RTTIBase<Mutator>
{
	EventManager *event_manager;

	Vector<MutatorBatch> queue;
	bool target_change_valid = false;
	Vec3i target_change_offset;
	HermiteField target_change;
	MutatorBatch target_batch;

	NG_DELETE_COPY_AND_MOVE(Mutator);
	Mutator();
	~Mutator();

	void handle_map_storage_response(RTTIObject *event);

	void mutate(const MutatorBatch &batch);

	void update();
};

} // namespace Map
