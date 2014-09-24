#include "Map/Chunk.h"

namespace Map {

void Chunk::generate_lod_fields()
{
	switch (lods[0].data.length()) {
	case 0:
		return;
	case 1:
		for (int i = 1; i < LODS_N; i++) {
			lods[i].data.append(lods[0].data[0]);
			lods[i].seqs.pappend(0, 0, true);
			lods[i].finalize(CHUNK_SIZE / Vec3i(lod_factor(i)) + Vec3i(1));
		}
		break;
	default:
		HermiteField tmp_fields[LODS_N];
		for (int i = 0; i < LODS_N; i++) {
			tmp_fields[i] = HermiteField(CHUNK_SIZE / Vec3i(lod_factor(i)) + Vec3i(1));
		}
		lods[0].decompress(tmp_fields[0].data);
		for (int i = 1; i < LODS_N; i++) {
			reduce_field(&tmp_fields[i], tmp_fields[i-1]);
		}
		for (int i = 1; i < LODS_N; i++) {
			lods[i] = HermiteRLEField(tmp_fields[i]);
		}
		break;
	}
}

} // namespace Map
