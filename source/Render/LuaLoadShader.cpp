#include "Render/LuaLoadShader.h"
#include "Script/InterLua.h"

Vector<ShaderSection> load_shader_sections(lua_State *L, const char *filename,
	Error*)
{
	Vector<ShaderSection> out;
	auto load_shader = InterLua::Global(L, "global")["LoadShader"];
	auto result = load_shader(filename);
	for (int i = 1, n = result.Length(); i <= n; i++) {
		auto section = result[i];
		auto files_lua = section[4];
		Vector<String> files;
		files.append(section[3].As<const char*>());
		for (int i = 1, n = files_lua.Length(); i <= n; i++)
			files.append(files_lua[i].As<const char*>());
		out.append({
			String(section[1].As<const char*>()),
			String(section[2].As<const char*>()),
			std::move(files),
		});
	}
	return out;
}
