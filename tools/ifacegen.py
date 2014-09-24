#!/usr/bin/env python3

import re, string, sys
from argparse import ArgumentParser

templ = string.Template("""
// ### Generated by ifacegen.py, don't touch ###
class $name {
	struct InterfaceBase {
		virtual ~InterfaceBase() = default;
		$virtual_methods
	};

	template <typename T>
	struct InterfaceImplMeth : InterfaceBase {
		T *ptr;
		InterfaceImplMeth(T *ptr): ptr(ptr) {}
		$method_calls
	};

	template <typename T>
	struct InterfaceImplFunc : InterfaceBase {
		T *ptr;
		InterfaceImplFunc(T *ptr): ptr(ptr) {}
		$function_calls
	};

	std::unique_ptr<InterfaceBase> impl;

public:
	$name() = default;
	template <typename T>
	$name(FromMethodsT, T *ptr): impl(new (or_die) InterfaceImplMeth<T>{ptr}) {}
	template <typename T>
	$name(FromFunctionsT, T *ptr): impl(new (or_die) InterfaceImplFunc<T>{ptr}) {}

	template <typename T>
	void FromMethods(T *ptr) { impl.reset(new (or_die) InterfaceImplMeth<T>{ptr}); }
	template <typename T>
	void FromFunctions(T *ptr) { impl.reset(new (or_die) InterfaceImplFunc<T>{ptr}); }

	explicit operator bool() const { return static_cast<bool>(impl); }

	$redirections
};
// ######
""".strip())

data = sys.stdin.read()

args_re = re.compile("\s*,\s*")
func_re = re.compile("([^()]+)\(([^()]*)\);")
type_name_re = re.compile("(.+?)(\w+)$")

def virtual_method(f):
	return "virtual {} = 0;".format(f.ToProto())

def redirection(f):
	optret = "return " if f.ReturnsValue() else ""
	return "{0} {{ {optret}impl->{1}; }}".format(f.ToProto(), f.ToCall(), optret=optret)

def method_call(f):
	optret = "return " if f.ReturnsValue() else ""
	return "{0} override {{ {optret}ptr->{1}; }}".format(f.ToProto(), f.ToCall(), optret=optret)

def function_call(f):
	optret = "return " if f.ReturnsValue() else ""
	return "{0} override {{ {optret}{1}; }}".format(f.ToProto(), f.ToFuncCall("*ptr"), optret=optret)

class TypeName(object):
	def __init__(self, ty, name):
		self.type = ty
		self.name = name

	def Type(self):
		return self.type

	def Name(self):
		return self.name

class Function(object):
	def __init__(self, f, args):
		self.f = f
		self.args = args

	def ReturnType(self):
		return self.f.Type()
	def Name(self):
		return self.f.Name()
	def Args(self):
		return self.args
	def ReturnsValue(self):
		return self.ReturnType().strip() != "void"

	def ToProto(self):
		s = self.ReturnType() + self.Name() + "("
		for i, a in enumerate(self.Args()):
			if i != 0:
				s += ", "
			s += a.Type() + a.Name()
		s += ")"
		return s

	def ToCall(self):
		s = self.Name() + "("
		for i, a in enumerate(self.Args()):
			if i != 0:
				s += ", "
			s += a.Name()
		s += ")"
		return s

	def ToFuncCall(self, arg0):
		s = self.Name() + "(" + arg0
		for a in self.Args():
			s += ", " + a.Name()
		s += ")"
		return s

	def __str__(self):
		return self.ToProto() + ";"


def parse_type_and_name(tn):
	m = type_name_re.match(tn)
	if not m:
		return None
	return TypeName(*m.groups())

def parse_function(line):
	m = func_re.match(line)
	if not m:
		return None
	retname, argstrs = m.groups()

	f = parse_type_and_name(retname)
	if not f:
		return None

	args = []
	for a in args_re.split(argstrs):
		if not a:
			continue
		arg = parse_type_and_name(a)
		if not arg:
			continue
		args.append(arg)

	return Function(f, args)

funcs = []
for line in data.splitlines():
	line = line.strip()
	if line == "":
		continue
	f = parse_function(line)
	if f:
		funcs.append(f)

virtual_methods = []
method_calls = []
function_calls = []
redirections = []
for f in funcs:
	virtual_methods.append(virtual_method(f))
	method_calls.append(method_call(f))
	function_calls.append(function_call(f))
	redirections.append(redirection(f))

parser = ArgumentParser(description="Generate an implicit C++ interface")
parser.add_argument("name",
	help="name of the interface")

p = parser.parse_args()

print(templ.substitute(
	virtual_methods = "\n\t\t".join(virtual_methods),
	method_calls = "\n\t\t".join(method_calls),
	function_calls = "\n\t\t".join(function_calls),
	redirections = "\n\t".join(redirections),
	name = p.name,
))