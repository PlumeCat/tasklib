#include <tasklib.h>
#include <sstream>

template<typename... Args>
void log(Args... args) {
	(cout << ... << args) << endl;
}


#define tasklib_error(s) { log(s); throw runtime_error(s); }

Task::Task(const string& name, vector<size_t>&& deps, const TaskFunction& tf):
	name(name),
	dependencies(deps),
	func(tf) {}
TaskSet::TaskSet(vector<Task>&& tasks):
	tasks(tasks) {}

TaskSetBuilder& TaskSetBuilder::add(const string& name, const unordered_set<string>& deps, const TaskFunction& func) {
	if (tasks.find(name) != tasks.end()) {
		tasklib_error("task already exists: " + name);
	}

	// insert into the tasks map
	tasks.emplace(pair(name, func));

	// insert into the dependencies
	forward_deps.emplace(name, deps);

	// insert into reverse dependencies
	for (const auto& d : deps) {
		reverse_deps[d].emplace(name);
	}

	return *this;
}

TaskSet TaskSetBuilder::build() const {
	// topological sort from wikipedia
	// https://en.wikipedia.org/wiki/Topological_sorting

	/*
	L - Empty list that will contain the sorted elements
	S - Set of all nodes with no dependencies

	while S is not empty do
		remove a node n from S
		add n to L
		for each node m that depends on n
			remove dependency on n from m
			if m has no other dependencies then
				insert m into S

	if graph has edges then
		return error   (graph has at least one cycle)
	else
		return L   (a topologically sorted order)
	*/

	// check reverse deps for missing tasks
	for (const auto& r : reverse_deps) {
		if (tasks.find(r.first) == tasks.end()) {
			auto ss = stringstream {};
			ss << "unknown task: '" << r.first << "'";
			if (r.second.size()) {
				auto iter = r.second.begin();
				ss << " dependency of '" << *(iter++);
				for (; iter != r.second.end(); iter++) {
					ss << "', '" << *iter;
				}
				ss << "'";
			}
			tasklib_error(ss.str());
		}
	}

	auto final_index = unordered_map<string, size_t> {};
	final_index.reserve(tasks.size()); // name to final index
	
	auto fd = forward_deps; // a copy that can be modified
	auto sorted = vector<Task> {}; // the final sorted list (will be moved-from)
	sorted.reserve(tasks.size());
	
	auto roots = vector<pair<string, TaskFunction>> {}; roots.reserve(tasks.size());
	auto processed_count = 0;

	// construct the initial roots list
	for (const auto& [n, f] : tasks) {
		if (fd[n].empty()) {
			roots.emplace_back(n, f);
		}
	}

	while (processed_count < tasks.size()) {
		// remove node from S
		if (processed_count >= roots.size()) {
			tasklib_error("Cycle detected!");
		}
		const auto& node = roots[processed_count++];

		// find the dependencies
		const auto& node_fd = forward_deps.find(node.first)->second; // can't use [] because non-const
		auto deps = vector<size_t> {};
		deps.reserve(node_fd.size());
		for (const auto& d : node_fd) {
			deps.emplace_back(final_index[d]);
		}

		// add task to sorted list
		final_index[node.first] = sorted.size();
		sorted.emplace_back(node.first, move(deps), node.second);

		// for each node m that depends on n (if any)
		const auto& rd = reverse_deps.find(node.first);
		if (rd != reverse_deps.end()) {
			for (const auto& m : rd->second) {
				// remove dependency on n from m
				auto& m_fd = fd[m];
				m_fd.erase(node.first);
				// if m has no other dependencies, insert into roots
				if (m_fd.size() == 0) {
					roots.emplace_back(m, tasks.at(m));
				}
			}
		}
	}

	return TaskSet(move(sorted));
}


TaskEngine::TaskEngine(size_t num_threads):
	has_tasks(flag_init) {
	for (auto i = 0; i < num_threads; i++) {
		threads.emplace_back(&TaskEngine::thread_main, this, i+1);
	}
}
TaskEngine::~TaskEngine() {
	should_exit = true;
	
	// clear the queue and signal worker threads
	task_queue.clear();
	flag_set(has_tasks);

	// wait for worker threads to exit (no need to wait for exit task completion, it's implicit)
	for (auto& t: threads) {
		t.join();
	}
}

void TaskEngine::add_tasks(const TaskSet& task_set) {
	// TODO: mutex and cvar / flag creation is expensive
	// reuse/pool rather than continually destroying and recreating
	task_queue.clear();
	for (const auto& t : task_set.tasks) {
		task_queue.emplace_back(t.func, t.dependencies);
	}

	queue_pos.store(0);
	flag_set(has_tasks);
}

void TaskEngine::run(const TaskSet& task_set) {
	add_tasks(task_set);
	
	// run some tasks on this thread
	while (true) {
		auto task_index = queue_pos.fetch_add(1);
		if (task_index < task_queue.size()) {
			do_task(task_index);
		} else {
			// only the main thread clears the has_tasks flag
			flag_clear(has_tasks);
			break;
		}
	}

	// wait for all tasks to complete
	// ensures we don't queue new tasks until current set is complete
	for (auto i = 0; i < task_queue.size(); i++) {
		flag_wait(task_queue[i].is_complete);
	}
}

void TaskEngine::thread_main(size_t thread_id) {
	while (!should_exit) {
		// wait for tasks to appear
		flag_wait(has_tasks);
		auto task_index = queue_pos.fetch_add(1);
		if (task_index < task_queue.size()) {
			do_task(task_index);
		}
	}
}

void TaskEngine::do_task(size_t task_index) {
	auto& task = task_queue[task_index];

	// wait for dependencies
	for (auto s : task.dependencies) {
		flag_wait(task_queue[s].is_complete);
	}

	// run task
	if (task.func) {
		task.func();
	}

	// notify threads that depend on this task
	flag_set(task.is_complete);
}

TaskEngine::QueueTask::QueueTask(const TaskFunction& func, const vector<size_t>& deps):
	func(func),
	dependencies(deps),
	is_complete(flag_init) {}

TaskEngine::QueueTask::QueueTask(TaskEngine::QueueTask&& t) noexcept:
	is_complete(flag_init) {}
