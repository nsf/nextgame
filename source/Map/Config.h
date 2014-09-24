#pragma once

#include "Math/Vec.h"
#include "Core/String.h"

namespace Map {

struct Config {
	Vec3i visible_range;
};

struct StorageConfig {
	const Config *map_config = nullptr;
	String directory;
};

} // namespace Map
