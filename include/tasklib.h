// tasklib.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include <string>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
using namespace std;

// copy and move shorthand
#define copy_type(cls, type) cls(const cls&)=type; cls& operator=(const cls&)=type;
#define move_type(cls, type) cls(cls&&)=type; cls& operator=(cls&&)=type;
#define copy_disable(cls) copy_type(cls, delete);
#define move_disable(cls) move_type(cls, delete);
#define copy_default(cls) copy_type(cls, default);
#define move_default(cls) move_type(cls, default);


// configuration
// #define TASKLIB_FLAG_TYPE_SIMPLE
#define TASKLIB_FLAG_TYPE_ATOMIC_FLAG
#include "simple_flag.h"
#include "config_flag.h"

// using TaskFunction = function<void()>;
using TaskFunction = void(*)(size_t);

// the Task storage type
// not much use for this class on its own outside of a TaskSet
// but is externally defined as TaskEngine and TaskSetBuilder use it
struct Task {
	const string name;
	const vector<size_t> dependencies; // an index into TaskSet::tasks
	const TaskFunction func;

	copy_default(Task);
	move_default(Task);
	Task() = default;
	Task(const string& name, vector<size_t>&& deps, const TaskFunction& tf);
	~Task() = default;
};

class TaskSet {
public:
	copy_default(TaskSet);
	move_default(TaskSet);
	TaskSet(vector<Task>&& tasks);
	const vector<Task> tasks;
private:
};

class TaskSetBuilder {
public:
	copy_default(TaskSetBuilder);
	move_default(TaskSetBuilder);
	TaskSetBuilder() = default;
	~TaskSetBuilder() = default;
	// add a task of given name with list of dependencies and function
	TaskSetBuilder& add(const string& name, const unordered_set<string>& deps, const TaskFunction& func);
	TaskSet build() const;

private:
	unordered_map<string, TaskFunction> tasks;
	unordered_map<string, unordered_set<string>> forward_deps;
	unordered_map<string, unordered_set<string>> reverse_deps;
};

/*
* Despite the fact that TaskEngine uses threads, it should not be considered thread-safe in and of itself.
* run() will block the calling thread until the task set is complete - tasks are also executed in the calling thread.
* - Do NOT call run() from another thread while tasks are in flight.
* - Do NOT destruct a TaskEngine instance from one thread while tasks are in flight on another thread
*/


class TaskEngine {
public:
	// num_threads is the number of background threads
	TaskEngine(size_t num_threads);
	~TaskEngine();

	// run the given workflow on the current thread and background threads
	// blocks until all tasks have been run
	void run(const TaskSet& task_set);

private:
	copy_disable(TaskEngine);
	move_disable(TaskEngine);

	void add_tasks(const TaskSet& task_set);
	
	// stuff for thread workers
	void thread_main(size_t thread_id);
	void do_task(size_t task_index);

	struct QueueTask {
		TaskFunction func;
		vector<size_t> dependencies;
		TasklibFlag is_complete;
		
		copy_disable(QueueTask);
		QueueTask(QueueTask&&) noexcept;
		QueueTask& operator=(QueueTask&&) noexcept;

		QueueTask() = delete;
		QueueTask(const TaskFunction& func, const vector<size_t>& deps);
		~QueueTask() = default;
	};
	vector<QueueTask> task_queue;
	atomic_uint queue_pos;
	TasklibFlag has_tasks;

	// worker threads
	vector<thread> threads;
	atomic_bool should_exit;
};
