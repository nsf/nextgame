#pragma once

#include "Math/Vec.h"
#include "Serialize/MsgPack.h"

SD_DEFINE_METHOD_FOR_TYPE(MsgPack, Vec3, {
	SD_FIELD(x);
	SD_FIELD(y);
	SD_FIELD(z);
});

SD_DEFINE_METHOD_FOR_TYPE(MsgPack, Vec3i, {
	SD_FIELD(x);
	SD_FIELD(y);
	SD_FIELD(z);
});
