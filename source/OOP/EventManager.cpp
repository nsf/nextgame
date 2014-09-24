#include "OOP/EventManager.h"

EventHandler::EventHandler(EventHandler &&r):
	receiver(r.receiver), own_data(r.own_data), on_event(r.on_event)
{
	r.receiver = nullptr;
	r.own_data = false;
	r.on_event = nullptr;
}

EventHandler &EventHandler::operator=(EventHandler &&r)
{
	if (own_data)
		delete receiver;
	receiver = r.receiver;
	own_data = r.own_data;
	on_event = r.on_event;
	r.receiver = nullptr;
	r.own_data = false;
	r.on_event = nullptr;
	return *this;
}

EventHandler::~EventHandler()
{
	if (own_data)
		delete receiver;
}

EventManager::EventManager()
{
	if (NG_EventManager)
		die("There can only be one EventManager");
	NG_EventManager = this;
}

EventManager::~EventManager()
{
	NG_EventManager = nullptr;
}

void EventManager::register_handler(EventID event_id,
	void (*on_event)(RTTIObject *, RTTIObject *), RTTIObject *receiver, bool own)
{
	EventHandler eh;
	eh.receiver = receiver;
	eh.on_event = on_event;
	eh.own_data = own;

	if (handlers.length() <= event_id)
		handlers.resize(event_id+1);

	Vector<EventHandler> &v = handlers[event_id];
	for (EventHandler &h : v) {
		if (h.receiver == receiver) {
			h = std::move(eh);
			return;
		}
	}
	v.append(std::move(eh));
}

void EventManager::unregister_handler(EventID event_id, RTTIObject *receiver)
{
	NG_ASSERT(event_id < handlers.length());
	if (receiver == nullptr) {
		handlers[event_id].clear();
	} else {
		Vector<EventHandler> &v = handlers[event_id];
		for (int i = 0, n = v.length(); i < n; i++) {
			if (v[i].receiver == receiver) {
				v.quick_remove(i);
				break;
			}
		}
	}
}

void EventManager::unregister_handlers(RTTIObject *receiver)
{
	for (Vector<EventHandler> &v : handlers) {
		for (int i = 0; i < v.length(); ) {
			if (v[i].receiver == receiver) {
				v.quick_remove(i);
				continue;
			}
			i++;
		}
	}
}

void EventManager::fire(EventID event_id, RTTIObject *event, RTTIObject *receiver)
{
	NG_ASSERT(event_id < handlers.length());
	for (EventHandler &h : handlers[event_id]) {
		if (receiver == nullptr or h.receiver == receiver)
			(*h.on_event)(event, h.receiver);
	}
}

EventManager *NG_EventManager = nullptr;
