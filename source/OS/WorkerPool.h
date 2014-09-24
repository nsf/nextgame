#pragma once

#include "Core/Vector.h"
#include "Core/String.h"
#include "OS/WorkerTask.h"
#include "OS/AsyncQueue.h"

struct SDL_Thread;

struct WorkerTaskInternal {
	RTTIObject *data = nullptr;
	void (*execute)(RTTIObject *data) = nullptr;
	void (*finalize)(RTTIObject *data) = nullptr;
};

struct Worker {
	String name;
	AsyncQueue<WorkerTaskInternal> *incoming;
	AsyncQueue<WorkerTaskInternal> *outgoing;
	SDL_Thread *thread;
	int n;
};

struct WorkerPoolImpl {
	Worker io_worker;
	Vector<Worker> cpu_workers;
	AsyncQueue<WorkerTaskInternal> to_io_worker;
	AsyncQueue<WorkerTaskInternal> to_cpu_workers;
	AsyncQueue<WorkerTaskInternal> from_workers;
};

struct WorkerPool :	RTTIBase<WorkerPool>
{
	Vector<WorkerTaskInternal> to_finalize;

	// using a pointer to impl here, because thread holds a pointer to each
	// worker and also pointers to queues, too many pointers, let's just keep
	// all that crap within single location, so that pointers stay valid
	WorkerPoolImpl *m_impl = nullptr;
	void finalize_tasks();

	// ncpu == 0 means auto detect
	WorkerPool(): WorkerPool(0) {}
	explicit WorkerPool(int ncpu);
	~WorkerPool();

	void handle_queue_cpu_task(RTTIObject *event);
	void handle_queue_io_task(RTTIObject *event);
};

extern WorkerPool *NG_WorkerPool;
