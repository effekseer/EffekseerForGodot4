#pragma once

#include <godot_cpp/godot.hpp>
#include <godot_cpp/core/class_db.hpp>

#define GDBIND_METHOD(class_name, method_name, ...) \
	ClassDB::bind_method(D_METHOD(#method_name, __VA_ARGS__), &class_name::method_name)

#define GDBIND_PROPERTY_SET_GET(class_name, property_name, type) \
	ClassDB::bind_method(D_METHOD("set_"#property_name, #property_name), &class_name::set_##property_name); \
	ClassDB::bind_method(D_METHOD("get_"#property_name), &class_name::get_##property_name); \
	ADD_PROPERTY(PropertyInfo(type, #property_name), "set_"#property_name, "get_"#property_name)

#define GDBIND_PROPERTY_SET_IS(class_name, property_name, type) \
	ClassDB::bind_method(D_METHOD("set_"#property_name, #property_name), &class_name::set_##property_name); \
	ClassDB::bind_method(D_METHOD("is_"#property_name), &class_name::is_##property_name); \
	ADD_PROPERTY(PropertyInfo(type, #property_name), "set_"#property_name, "is_"#property_name)

#define GDBIND_SIGNAL(class_name, signal_name, ...) \
	ClassDB::add_signal(#class_name, MethodInfo(#signal_name, __VA_ARGS__))

#define GDBIND_SIGNAL_ARG(arg_name, type) \
	PropertyInfo(type, #arg_name)
