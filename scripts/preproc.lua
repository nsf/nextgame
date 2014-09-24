local M = {}

local utils = require "utils"

local GLSL_VERSION_STRING = "#version 330"

local EXPR_PATTERNS = {
	"^%$([a-zA-Z0-9.:_]+%b())",
	"^%$([a-zA-Z0-9.:_]+%b[])",
	"^%$([a-zA-Z0-9.:_]+)",
	"^%$(%b{})",
}

-- Tokenizes input into the following stream of tokens:
-- 1. Plain text.
-- 2. One of the following:
--     - variable: %$[a-zA-Z0-9.:_]+ or %$%b{}
--     - table: %$[a-zA-Z0-9.:_]+%b[]
--     - function: %$[a-zA-Z0-9.:_]+%b()
--
-- possible return values:
-- {
--    type = "text",
--    argument = <text>,
--    line_offset = <line offset>,
-- }
-- {
--    type = "expr",
--    argument = <lua expr>,
--    line_offset = <line offset>,
-- }
function M.TokenizeText(text)
	local tokens = {}
	local p = 1
	while true do
		local b, e = text:find("$", p, true)
		if not b then
			-- no $ sign, means no need to expand anything, we're done here
			tokens[#tokens+1] = {
				type = "text",
				argument = text:sub(p),
				line_offset = utils.LineOffset(text, p),
			}
			break
		end
		if b ~= p then
			-- found the $ sign, but not at the beginning of the string, means
			-- there is text in-between, submit it
			tokens[#tokens+1] = {
				type = "text",
				argument = text:sub(p, b-1),
				line_offset = utils.LineOffset(text, p),
			}
		end
		local expr
		for _, pat in ipairs(EXPR_PATTERNS) do
			expr = text:match(pat, b)
			if expr then
				break
			end
		end
		if not expr then
			error("unrecognized dollar-sign ($) substitution form")
		end
		p = b + #expr + 1
		if expr:byte(1) == string.byte("{") then
			-- in case if it's a ${variable} form, remove {}
			expr = expr:sub(2, -2)
		end
		tokens[#tokens+1] = {
			type = "expr",
			argument = expr,
			line_offset = utils.LineOffset(text, b),
		}
	end
	return tokens
end

local function PreprocessLocal(data, attributes)
	local sys = {
		files = {},
		line_offset = attributes.line_offset,
		fileno = attributes.fileno or 0,
		context = attributes.context,
		load_include = attributes.load_include,
	}
	local tokens = M.Tokenize(data)
	local nodes = M.Parse(tokens)
	local out = M.Interpret(nodes, sys)
	return out, sys.files
end

local QUOTES = {
	{string.byte('"'), string.byte('"')},
	{string.byte('<'), string.byte('>')},
	{string.byte("'"), string.byte("'")},
	{string.byte("`"), string.byte("`")},
}

local function CleanString(str)
	for _, q in ipairs(QUOTES) do
		if str:byte(1) == q[1] and str:byte(-1) == q[2] then
			return str:sub(2, -2)
		end
	end
	return str
end

local INTERPRETERS = {
	_if = function(node, sys)
		local expr = "return "..node.argument
		local ok = load(expr, expr, "t", sys.context)()
		if ok then
			return M.Interpret(node.body_nodes, sys)
		else
			return M.Interpret(node.else_nodes, sys)
		end
	end,
	_do = function(node, sys)
		local expr = node.argument
		load(expr, expr, "t", sys.context)()
		return ""
	end,
	_text = function(node, sys)
		local tokens = M.TokenizeText(node.argument)
		local base_line = node.token.line + sys.line_offset
		local out = ""
		for _, tok in ipairs(tokens) do
			local line = base_line + tok.line_offset
			out = out.."#line "..line.." "..sys.fileno.."\n"
			if tok.type == "text" then
				out = out..tok.argument.."\n"
			elseif tok.type == "expr" then
				local expr = "return "..tok.argument
				out = out..load(expr, expr, "t", sys.context)().."\n"
			end
		end
		return out
	end,
	_include = function(node, sys)
		if not sys.load_include then
			return ""
		end
		local filename = CleanString(node.argument)
		local data = sys.load_include(filename)

		-- disable more than 1 level of includes
		sys.files[#sys.files+1] = filename
		return PreprocessLocal(data, {
			line_offset = 0,
			context = sys.context,
			fileno = sys.fileno+1,
		})
	end,
}

function M.Interpret(nodes, sys)
	local out = ""
	for _, node in ipairs(nodes) do
		local interp = INTERPRETERS["_"..node.type]
		if not interp then
			error("unknown node type: '"..node.type.."'")
		end
		if #out > 0 then
			out = out.."\n"
		end
		out = out..interp(node, sys)
	end
	return out
end

local ParseNode, ParseIfNode

local function IsIfEndToken(tok)
	if tok.type ~= "directive" then
		return false
	end
	return tok.key == "elseif" or tok.key == "else" or tok.key == "end"
end

local function IsIfElseEndToken(tok)
	if tok.type ~= "directive" then
		return false
	end
	if tok.key == "elseif" or tok.key == "else" then
		error("unexpected '"..tok.key.."' token")
	end
	return tok.key == "end"
end

ParseIfNode = function(tokens, i)
	local tok = tokens[i]
	local if_node = {
		type = "if",
		argument = tok.value,
		body_nodes = {},
		else_nodes = {},
	}

	i = i + 1
	local current_tfun = IsIfEndToken -- termination function
	local current_table = if_node.body_nodes
	while i <= #tokens do
		if current_tfun(tokens[i]) then
			local tok = tokens[i]
			if tok.key == "elseif" then
				local node, ni = ParseIfNode(tokens, i)
				if_node.else_nodes[1] = node
				return if_node, ni
			elseif tok.key == "else" then
				current_tfun = IsIfElseEndToken
				current_table = if_node.else_nodes
				i = i + 1
			else
				return if_node, i+1
			end
		else
			local node, ni = ParseNode(tokens, i)
			current_table[#current_table+1], i = node, ni
		end
	end
	error("no matching end statement for an if statement")
end

-- Takes a list of tokens and a pointer into it, returns a parsed node and a new
-- pointer. In many cases new pointer is i+1, but if the node is a block node
-- (such as an if statement, then it's > 1).
ParseNode = function(tokens, i)
	local tok = tokens[i]
	local key, value
	if tok.type == "text" then
		return {
			type = "text",
			argument = tok.text,
			token = tok,
		}, i+1
	elseif tok.type == "directive" then
		key = tok.key
		value = tok.value
	else
		error("unknown token type: "..tok.type)
	end

	if key == "if" then
		return ParseIfNode(tokens, i)
	elseif key == "do" then
		return {
			type = "do",
			argument = value,
			token = tok,
		}, i+1
	elseif key == "include" then
		return {
			type = "include",
			argument = value,
			token = tok,
		}, i+1
	else -- "elseif", "else", "end"
		-- All of these tokens are parts of the block nodes and should not be
		-- seen here. When they are standalone, it's an error.
		error("erroneous standalone directive: '"..key.."'")
	end
end

-- Converts tokens into an AST. AST understands the following directives
-- (directives not in the list are converted to text):
-- 1. #if <lua expr>
-- 2. #elseif <lua expr>
-- 3. #else
-- 4. #do <lua expr>
-- 5. #end
-- 6. #include <string>
--
-- Obviously every #if must end with an #end. Yes, it's not a C preprocessor
-- syntax, keywords are taken from lua.
--
-- It returns an array of AST nodes, all node types are listed here:
-- {
--   type = "if"
--   argument = <lua expr>,
--   body_nodes = {<ast node>, <ast node>, ...},
--   else_nodes = {<ast node>, <ast node>, ...},
--   token = <token>,
-- }
-- {
--   type = "do",
--   argument = <lua expr>,
--   token = <token>,
-- }
-- {
--   type = "text",
--   argument = <text>,
--   token = <token>,
-- }
-- {
--   type = "include",
--   argument = <filename>,
--   token = <token>,
-- }
--
-- Note: all nodes include their source token for error reporting purposes.
function M.Parse(tokens)
	local i = 1
	local nodes = {}
	while i <= #tokens do
		local node, ni = ParseNode(tokens, i)
		nodes[#nodes+1] = node
		i = ni
	end
	return nodes
end

-- Tokenizes input into a sequence of tokens:
-- 1. If line matches '%s*#%s*(%a+)', it's a `directive`.
-- 2. Otherwise it's a `text`.
-- 3. Each token is a table with the following keys:
--    * type ("directive" or "text")
--    * line (line where it was found, relative to input)
--    * text (if type == "text")
--    * key (if type == "directive" then directive key, e.g. #if -> key == "if")
--    * value (if type == "directive" the rest of the line after key)
function M.Tokenize(data)
	local tokens = {}
	local deferred_text = nil
	for line_n, line in utils.Lines(data) do
		local b, e, key = line:find("^%s*#%s*(%a+)")
		if key then -- `directive` token
			if deferred_text then -- commit accumulated `text` token
				tokens[#tokens+1] = deferred_text
				deferred_text = nil
			end

			local value = line:sub(e+1):match("^%s*(.-)%s*$")
			tokens[#tokens+1] = {
				type = "directive",
				line = line_n,
				key = key,
				value = value,
			}
		else
			-- we defer adding `text` tokens, so that all the text lines are
			-- accumulated properly
			if not deferred_text then
				deferred_text = {
					type = "text",
					line = line_n,
				}
			end
			if not deferred_text.text then
				deferred_text.text = line
			else
				deferred_text.text = deferred_text.text.."\n"..line
			end
		end
	end
	if deferred_text then -- commit accumulated `text` token
		tokens[#tokens+1] = deferred_text
		deferred_text = nil
	end
	return tokens
end

function M.Preprocess(data, attributes)
	local text, files = PreprocessLocal(data, attributes)
	return GLSL_VERSION_STRING.."\n"..text, files
end

return M
