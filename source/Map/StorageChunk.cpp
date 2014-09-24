#include "Map/StorageChunk.h"
#include "Math/Noise.h"
#include "OS/IO.h"

namespace Map {

void StorageChunk::save(const StorageConfig &config, Error *err) const
{
	ByteWriter w;
	ByteWriter tmp;
	String filename = String::format("%d_%d_%d.ngc", VEC3(location));
	String fullpath = config.directory + "/" + filename;
	w.write_string("NGSC");
	w.write_int32(CHUNK_SIZE.x);
	w.write_int32(CHUNK_SIZE.y);
	w.write_int32(CHUNK_SIZE.z);
	w.write_int32(STORAGE_CHUNK_SIZE.x);
	w.write_int32(STORAGE_CHUNK_SIZE.y);
	w.write_int32(STORAGE_CHUNK_SIZE.z);

	for (const auto &c : chunks) {
		const HermiteRLEField &f = c.lods[0];
		f.serialize(&tmp);
	}
	w.write_compressed(tmp.sub());
	IO::write_file(fullpath.c_str(), w.sub(), err);
}

StorageChunk::StorageChunk(const Vec3i &location):
	chunks(volume(STORAGE_CHUNK_SIZE)), location(location)
{
}

StorageChunk StorageChunk::new_from_file(const Vec3i &location,
	const StorageConfig &config, Error *err)
{
	String filename = String::format("%d_%d_%d.ngc", VEC3(location));
	String fullpath = config.directory + "/" + filename;

	auto contents = IO::read_file(fullpath.c_str(), err);
	if (*err)
		return StorageChunk(location);

	return new_from_buffer(location, contents, err);
}

StorageChunk StorageChunk::new_from_buffer(const Vec3i &location,
	const Vector<uint8_t> &contents, Error *err)
{
	if (contents.length() < 4 || slice_cast<const char>(contents.sub(0, 4)) != "NGSC") {
		err->set("Bad magic, NGSC expected");
		return StorageChunk(location);
	}

	ByteReader br(contents.sub(4));
	Vec3i chunk_size, storage_chunk_size;

	chunk_size.x = br.read_int32(err);
	chunk_size.y = br.read_int32(err);
	chunk_size.z = br.read_int32(err);
	if (*err)
		return StorageChunk(location);

	if (chunk_size != CHUNK_SIZE) {
		err->set("Mismatching chunk sizes, file: (%d %d %d), expected: (%d %d %d)",
			VEC3(chunk_size), VEC3(CHUNK_SIZE));
		return StorageChunk(location);
	}

	storage_chunk_size.x = br.read_int32(err);
	storage_chunk_size.y = br.read_int32(err);
	storage_chunk_size.z = br.read_int32(err);
	if (*err)
		return StorageChunk(location);

	if (storage_chunk_size != STORAGE_CHUNK_SIZE) {
		err->set("Mismatching storage chunk sizes, file: (%d %d %d), expected: (%d %d %d)",
			VEC3(storage_chunk_size), VEC3(STORAGE_CHUNK_SIZE));
		return StorageChunk(location);
	}

	auto tmp = br.read_compressed(err);
	if (*err)
		return StorageChunk(location);

	StorageChunk msc(location);
	br = ByteReader(tmp);
	for (int i = 0, n = volume(storage_chunk_size); i < n; i++) {
		Chunk &c = msc.chunks[i];
		c.lods[0].deserialize(&br, CHUNK_SIZE + Vec3i(1), err);
		if (*err)
			return StorageChunk(location);
	}

	return msc;
}

} // namespace Map

