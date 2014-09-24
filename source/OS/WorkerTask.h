#pragma once

#include "Core/Memory.h"
#include "OOP/EventManager.h"

struct EWorkerTask : RTTIBase<EWorkerTask>
{
	RTTIObject *data = nullptr;
	void (*execute)(RTTIObject *data) = nullptr;
	void (*finalize)(RTTIObject *data) = nullptr;
};

template <EventID EID>
void fire_and_delete_finalizer(RTTIObject *data)
{
	NG_EventManager->fire(EID, data);
	delete data;
}

template <EventID EID>
void fire_finalizer(RTTIObject *data)
{
	NG_EventManager->fire(EID, data);
}
