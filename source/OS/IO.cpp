#include "OS/IO.h"
#include "Core/Defer.h"

#include <sys/stat.h>
#include <cstdio>
#include <linux/limits.h>
#include <unistd.h>
#include <fnmatch.h>
#include <dirent.h>
#include <SDL2/SDL.h>

namespace IO {

//----------------------------------------------------------------------
// ReadFile and WriteFile
//----------------------------------------------------------------------

Vector<uint8_t> read_file(const char *filename, Error *err)
{
	Vector<uint8_t> data;
	FILE *f = fopen(filename, "rb");
	if (!f) {
		err->set("failed to open file: %s", filename);
		return {};
	}
	DEFER { fclose(f); };

	struct stat st;
	if (-1 == fstat(fileno(f), &st)) {
		err->set("failed to get the size of the file: %s", filename);
		return {};
	}

	data.resize(st.st_size);
	if ((size_t)st.st_size != fread(data.data(), 1, st.st_size, f)) {
		err->set("failed to read the file: %s", filename);
		return {};
	}

	return data;
}

void write_file(const char *filename, Slice<const uint8_t> data, Error *err)
{
	FILE *f = fopen(filename, "wb");
	if (!f) {
		err->set("failed to open file: %s", filename);
		return;
	}
	DEFER { fclose(f); };
	if (data.length != (int)fwrite(data.data, 1, data.length, f)) {
		err->set("failed to write the file: %s", filename);
		return;
	}
}

//----------------------------------------------------------------------
// Path-related Utils
//----------------------------------------------------------------------

static bool is_dir(const char *path)
{
	struct stat st;
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static void mkdir(char *path, Error *err)
{
	if (::mkdir(path, 0777) < 0) {
		err->set("Failed creating directory: '%s' (errno: %d)\n",
			path, errno);
	}
}

static void mkdir_recursive(char *path, Error *err)
{
	if (is_dir(path))
		return;

	char *q = strrchr(path, '/');
	if (q != nullptr) {
		*q = '\0';
		mkdir_recursive(path, err);
		*q = '/';
	}
	if (*err)
		return;
	mkdir(path, err);
}

void make_directories(String &path, Error *err)
{
	char *cpath = path.data();
	mkdir_recursive(cpath, err);
}

String get_application_directory()
{
	String xdghome = get_environment("XDG_CONFIG_HOME");
	if (xdghome == "") {
		xdghome = get_environment("HOME") + "/.config";
	}
	return clean_path(xdghome) + "/nextgame";
}

String get_current_directory()
{
	char tmp[PATH_MAX];
	char *result = getcwd(tmp, sizeof(tmp));
	if (result == nullptr) {
		die("Path is way too long, longer than %d", PATH_MAX);
		return {};
	}

	// Cast is required here, because otherwise it'll trigger C array ctor
	// of the Slice and we need the Slice<const char> ctor which uses strlen.
	return (const char*)tmp;
}

String get_environment(const char *what)
{
	return getenv(what);
}

bool fnmatch(const char *pattern, const char *name)
{
	return ::fnmatch(pattern, name, FNM_PATHNAME | FNM_NOESCAPE) == 0;
}

//----------------------------------------------------------------------
// Directories
//----------------------------------------------------------------------

struct Directory {
	DIR *dir;
};

Directory *open_directory(const char *path, Error *err)
{
	DIR *d = opendir(path);
	if (!d) {
		err->set("Failed to open directory '%s'", path);
		return nullptr;
	}

	Directory *dir = new (OrDie) Directory;
	dir->dir = d;
	return dir;
}

void close_directory(Directory *dir)
{
	closedir(dir->dir);
	delete dir;
}

String next_file(Directory *dir)
{
	while (1) {
		struct dirent *de = readdir(dir->dir);
		if (!de)
			return {};
		if (de->d_type != DT_REG)
			continue;
		return (const char*)de->d_name;
	}
	return {};
}

String next_directory(Directory *dir)
{
	while (1) {
		struct dirent *de = readdir(dir->dir);
		if (!de)
			return {};
		if (de->d_type != DT_DIR)
			continue;
		if (de->d_name[0] == '.')
			continue;
		return (const char*)de->d_name;
	}
	return {};
}

String clean_path(Slice<const char> str)
{
	if (str == "")
		return ".";

	String s = str;
	const auto is_sep = [&](int idx) {
		return idx == s.length() || s[idx] == '/';
	};
	const bool rooted = s[0] == '/';
	const int n = s.length();

	int dst = 0;
	int r = 0, dotdot = 0;
	if (rooted) {
		dst = r = dotdot = 1;
	}

	while (r < n) {
		if (s[r] == '/') {
			r++;
		} else if (s[r] == '.' && is_sep(r+1)) {
			r++;
		} else if (s[r] == '.' && s[r+1] == '.' && is_sep(r+2)) {
			r += 2;
			if (dst > dotdot) {
				dst--;
				while (dst > dotdot && s[dst] != '/') {
					dst--;
				}
			} else if (!rooted) {
				if (dst > 0) {
					s[dst++] = '/';
				}
				s[dst++] = '.';
				s[dst++] = '.';
				dotdot = dst;
			}
		} else {
			if ((rooted && dst != 1) || (!rooted && dst != 0)) {
				s[dst++] = '/';
			}
			while (r < n && s[r] != '/') {
				s[dst++] = s[r++];
			}
		}
	}
	if (dst == 0)
		s[dst++] = '.';
	s.resize(dst);
	return s;
}

} // namespace FS

//----------------------------------------------------------------------
// Lua bindings
//----------------------------------------------------------------------

NG_LUA_API IO::Directory *NG_FS_OpenDirectory(const char *path)
{
	Error err(EV_QUIET);
	return IO::open_directory(path, &err);
}

NG_LUA_API void NG_FS_CloseDirectory(IO::Directory *dir)
{
	IO::close_directory(dir);
}

NG_LUA_API char *NG_FS_NextFile(IO::Directory *dir)
{
	String s = IO::next_file(dir);
	return s.length() > 0 ? s.release() : nullptr;
}

NG_LUA_API char *NG_FS_NextDirectory(IO::Directory *dir)
{
	String s = IO::next_directory(dir);
	return s.length() > 0 ? s.release() : nullptr;
}

NG_LUA_API void NG_FreeString(char *ptr)
{
	delete [] ptr;
}

NG_LUA_API bool NG_FS_FNMatch(const char *pattern, const char *name)
{
	return IO::fnmatch(pattern, name);
}
