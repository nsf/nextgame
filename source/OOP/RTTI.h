#pragma once

#include "Core/Error.h"

struct RTTIType {
	// TODO: add something useful here? :D
};

template <typename T>
RTTIType *type_id()
{
	static RTTIType type;
	return &type;
}

struct RTTIObject {
	virtual ~RTTIObject();
	virtual RTTIType *type() const = 0;
};


template <typename T>
struct RTTIBase : RTTIObject {
	RTTIType *type() const override
	{
		return type_id<T>();
	}
	static RTTIType *static_type()
	{
		return type_id<T>();
	}
	static T *cast(RTTIObject *p, Error *err = &DefaultError)
	{
		if (p->type() == static_type())
			return static_cast<T*>(p);
		err->set("Invalid dynamic cast");
		return nullptr;
	}
	static const T *cast(const RTTIObject *p, Error *err = &DefaultError)
	{
		if (p->type() == static_type())
			return static_cast<const T*>(p);
		err->set("Invalid dynamic cast");
		return nullptr;
	}
};

template <typename T>
bool is_a(RTTIObject *p) { return T::static_type() == p->type(); }
