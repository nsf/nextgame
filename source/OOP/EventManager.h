#pragma once

#include "OOP/RTTI.h"
#include "Core/Vector.h"

enum EventID {
	// add custom event ids here
	EID_NONE,

	EID_TERMBOX_IMAGE_GENERATED,

	EID_MAP_STORAGE_REQUEST,
	EID_MAP_STORAGE_RESPONSE,

	EID_GENERATE_MAP_CHUNK_REQUEST,
	EID_MAP_CHUNK_GENERATED_INTERNAL,
	EID_MAP_CHUNK_GENERATED,

	EID_MAP_STORAGE_CHUNK_SAVED,
	EID_MAP_STORAGE_CHUNK_LOADED,
	EID_MAP_STORAGE_CHUNK_PRELOADED,
	EID_MAP_CHUNK_GEOMETRY_GENERATED,

	EID_CHUNKS_UPDATED,

	EID_QUEUE_CPU_TASK,
	EID_QUEUE_IO_TASK,
};

struct EventHandler {
	RTTIObject *receiver = nullptr;
	bool own_data = false;
	void (*on_event)(RTTIObject *event, RTTIObject *receiver) = nullptr;

	NG_DELETE_COPY(EventHandler);
	EventHandler() = default;
	EventHandler(EventHandler &&r);
	EventHandler &operator=(EventHandler &&r);
	~EventHandler();
};

struct EventManager {
	Vector<Vector<EventHandler>> handlers;

	NG_DELETE_COPY_AND_MOVE(EventManager);
	EventManager();
	~EventManager();

	void register_handler(EventID event_id,
		void (*on_event)(RTTIObject*, RTTIObject*),
		RTTIObject *data, bool own = true);
	void unregister_handler(EventID event_id, RTTIObject *receiver = nullptr);
	void unregister_handlers(RTTIObject *receiver);
	void fire(EventID event_id, RTTIObject *event, RTTIObject *receiver = nullptr);
};

extern EventManager *NG_EventManager;

template <typename T, void (T::*Method)(RTTIObject*)>
void pass_to_method(RTTIObject *event, RTTIObject *data)
{
	T *obj = T::cast(data);
	(obj->*Method)(event);
}

#define PASS_TO_METHOD(T, Method) pass_to_method<T, &T::Method>
