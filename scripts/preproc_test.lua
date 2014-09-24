local preproc = require "preproc"
local utils = require "utils"

local test_tokenize = false
local test_parse = false
local test_tokenize_text = false

local TEST_TEXT = [[

#include "ub_perframe.glsl"

#do x = 10; y = 5

#if x == 10
#if y == 5
void main() {
	dothis();
}
#end
#else

// comment here #oops

void main() {
	dowhat();

	dothat();
}
#end]]

if test_tokenize then
	local tokens = preproc.Tokenize(TEST_TEXT)
	for _, v in ipairs(tokens) do
		print("------------------------------------------------------------")
		print("type: "..v.type)
		print("line: "..v.line)
		if v.type == "directive" then
			print("key: [["..v.key.."]]")
			print("value: [["..v.value.."]]")
		elseif v.type == "text" then
			print("text: [["..v.text.."]]")
		else
			error("unknown type!")
		end
	end
	print("------------------------------------------------------------")
end

local function print_indent(indent)
	for i = 1, indent do
		io.write("----")
	end
end

if test_parse then
	local nodes = preproc.Parse(preproc.Tokenize(TEST_TEXT))
	local print_nodes
	local print_node = function(node, indent)
		print_indent(indent)
		if node.type == "text" then
			print(" type: '"..node.type.."', argument: '...'")
		else
			print(" type: '"..node.type.."', argument: '"..node.argument.."'")
		end
		if node.type == "if" then
			if #node.body_nodes > 0 then
				print_indent(indent)
				print(" body:")
				print_nodes(node.body_nodes, indent+1)
			end
			if #node.else_nodes > 0 then
				print_indent(indent)
				print(" else:")
				print_nodes(node.else_nodes, indent+1)
			end
		end
	end
	print_nodes = function(nodes, indent)
		for _, node in ipairs(nodes) do
			print_node(node, indent)
		end
	end
	print_nodes(nodes, 1)
end

local TEST_TEXT2 = [[

Hello, ${name}. Your age is: $ages[name].

uniform mat4 Projection;
$Builder:AutoUniform(
	"fog_intensity", "Fog Intensity", "world",
	"float", {min=0, max=1})
uniform vec3 CameraPosition;

]]

if test_tokenize_text then
	local tokens = preproc.TokenizeText(TEST_TEXT2)
	for _, tok in ipairs(tokens) do
		print("------------------------------------------------------------")
		print("type: "..tok.type)
		print("line offset: "..tok.line_offset)
		print("argument: [["..tok.argument.."]]")
	end
	print("------------------------------------------------------------")
end
