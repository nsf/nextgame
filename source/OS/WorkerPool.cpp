#include "OS/WorkerPool.h"
#include "OS/IO.h"
#include "Script/Lua.h"
#include <SDL2/SDL.h>

static int worker_thread(void *data)
{
	Worker *w = (Worker*)data;
	printf("%s on duty\n", w->name.c_str());

	// init worker lua vm
	LuaVM *vm = NG_WorkerLuaWM.get();
	vm->init_worker();
	vm->do_file("boot.lua");
	vm->on_event = InterLua::Global(vm->L, "global")["OnEvent"];

	while (true) {
		WorkerTaskInternal task = w->incoming->pop();
		if (task.execute == nullptr)
			break;
		(*task.execute)(task.data);
		w->outgoing->push(task);
	}

	printf("%s shutting down\n", w->name.c_str());
	return 0;
}

void WorkerPool::finalize_tasks()
{
	AsyncQueue<WorkerTaskInternal> &q = m_impl->from_workers;

	SDL_LockMutex(q.mutex);
	to_finalize.resize(q.queue.length());
	copy(to_finalize.sub(), q.queue.sub());
	q.queue.clear();
	SDL_UnlockMutex(q.mutex);

	for (WorkerTaskInternal &wt : to_finalize)
		if (wt.finalize)
			(*wt.finalize)(wt.data);
	to_finalize.clear();
}

WorkerPool::WorkerPool(int ncpu)
{
	if (NG_WorkerPool)
		die("There can only be one WorkerPool");
	NG_WorkerPool = this;

	if (ncpu == 0)
		ncpu = SDL_GetCPUCount();
	if (IO::get_environment("NEXTGAME_CPU_N") == "1")
		ncpu = 1;

	m_impl = new (OrDie) WorkerPoolImpl;

	WorkerPoolImpl *wpi = m_impl;
	wpi->cpu_workers.resize(ncpu);

	int i = 0;
	for (Worker &w : wpi->cpu_workers) {
		w.name = String::format("NG CPU Worker #%d", i++);
		w.incoming = &wpi->to_cpu_workers;
		w.outgoing = &wpi->from_workers;
		w.thread = SDL_CreateThread(worker_thread, w.name.c_str(), &w);
	}

	wpi->io_worker.name = String::format("NG I/O Worker");
	wpi->io_worker.incoming = &wpi->to_io_worker;
	wpi->io_worker.outgoing = &wpi->from_workers;
	wpi->io_worker.thread = SDL_CreateThread(worker_thread,
		wpi->io_worker.name.c_str(), &wpi->io_worker);

	NG_EventManager->register_handler(EID_QUEUE_CPU_TASK,
		PASS_TO_METHOD(WorkerPool, handle_queue_cpu_task),
		this, false);
	NG_EventManager->register_handler(EID_QUEUE_IO_TASK,
		PASS_TO_METHOD(WorkerPool, handle_queue_io_task),
		this, false);
}

WorkerPool::~WorkerPool()
{
	for (int i = 0; i < m_impl->cpu_workers.length(); i++)
		m_impl->to_cpu_workers.push(WorkerTaskInternal());
	m_impl->to_io_worker.push(WorkerTaskInternal());

	for (Worker &w : m_impl->cpu_workers)
		SDL_WaitThread(w.thread, nullptr);
	SDL_WaitThread(m_impl->io_worker.thread, nullptr);
	delete m_impl;

	NG_EventManager->unregister_handlers(this);
	NG_WorkerPool = nullptr;
}

void WorkerPool::handle_queue_cpu_task(RTTIObject *event)
{
	EWorkerTask *wt = EWorkerTask::cast(event);

	WorkerTaskInternal wti;
	wti.data = wt->data;
	wti.execute = wt->execute;
	wti.finalize = wt->finalize;
	m_impl->to_cpu_workers.push(wti);
}

void WorkerPool::handle_queue_io_task(RTTIObject *event)
{
	EWorkerTask *wt = EWorkerTask::cast(event);

	WorkerTaskInternal wti;
	wti.data = wt->data;
	wti.execute = wt->execute;
	wti.finalize = wt->finalize;
	m_impl->to_io_worker.push(wti);
}

WorkerPool *NG_WorkerPool = nullptr;
