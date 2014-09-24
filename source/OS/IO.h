#pragma once

#include "Core/Vector.h"
#include "Core/String.h"
#include "Core/Error.h"

namespace IO {

//----------------------------------------------------------------------
// ReadFile and WriteFile
//----------------------------------------------------------------------

// Loads the file from a given location. On failure returns an empty vector.
Vector<uint8_t> read_file(const char *filename, Error *err = &DefaultError);
void write_file(const char *filename, Slice<const uint8_t> data,
	Error *err = &DefaultError);

//----------------------------------------------------------------------
// Path-related Utils
//----------------------------------------------------------------------

String clean_path(Slice<const char> str);

// It will use path's contents to split the string in-place and create all the
// directories recursively.
void make_directories(String &path, Error *err = &DefaultError);

String get_application_directory();
String get_current_directory();
String get_environment(const char *what);
bool fnmatch(const char *pattern, const char *name);

//----------------------------------------------------------------------
// Directories
//----------------------------------------------------------------------

struct Directory;

Directory *open_directory(const char *path, Error *err = &DefaultError);
void close_directory(Directory *dir);
String next_file(Directory *dir);
String next_directory(Directory *dir);

} // namespace FS
