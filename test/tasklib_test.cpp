#include <tasklib.h>

using namespace std;

template<typename... Args>
void log(Args... args) {
	(cout << ... << args) << endl;
}

void test_task(size_t thread_id) {
	// log("test_task\n");
	this_thread::sleep_for(chrono::microseconds(50 + rand() % 1000));
}

TaskSet make_randomized_test_set() {
	auto MAX_TASKS = 50;
	auto MIN_TASKS = 10;
	auto num_tasks = MIN_TASKS + rand() % (MAX_TASKS - MIN_TASKS);

	// make random names
	auto names = vector<string> {};
	for (int i = 0; i < num_tasks; i++) {
		names.emplace_back("task." + to_string(i));
	}

	auto builder = TaskSetBuilder();
	for (int i = 0; i < num_tasks; i++) {
		// can depend on previous tasks
		auto deps = unordered_set<string> {};
		for (int j = 0; j < i - 1; j++) {
			if (rand() % 100 < 50) {
				deps.insert(names[j]);
			}
		}

		builder.add(names[i], deps, test_task);
	}

	return builder.build();
}

// deliberately non-atomic test counter for linear test
int linear_counter = 0;
void linear_init_task(size_t thread_id) {
	linear_counter = 0;
	log("Linear task: ", linear_counter);
}
void linear_test_task(size_t thread_id) {
	linear_counter += 1;
	log("Linear task: ", linear_counter);
}

TaskSet make_linear_test_set() {
	auto names = vector<string>{};
	auto num_tasks = 100;
	for (auto i = 0; i < num_tasks; i++) {
		names.emplace_back("task." + to_string(i));
	}

	auto builder = TaskSetBuilder{};
	builder.add("init", {}, linear_init_task);
	for (auto i = 0; i < num_tasks; i++) {
		builder.add(
			"task."+to_string(i),
			{ (i == 0) ? "init"s : "task." + to_string(i-1) },
			linear_test_task
		);
	}

	return builder.build();
}

void test_randomized() {
	auto engine = TaskEngine(7);

	for (int i = 0; i < 100; i++) {
		auto set = make_randomized_test_set();
		log("running test", i);
		engine.run(set);
		log("done");
	}
}

void test_linear() {
	auto engine = TaskEngine(7);
	for (auto i = 0; i < 100; i++) {
		auto linear_set = make_linear_test_set();
		log("running linear test");
		engine.run(linear_set);
		log("done");
	}
}

void test_tree() {
	auto engine = TaskEngine(7);
	for (auto i = 0; i < 100; i++) {
		auto tree = make_tree_test_set();
		log("running tree-shaped test");
		engine.run(tree);
		log("done");
	}
}

int main() {
	test_randomized();
	test_linear();
	test_tree();

	log("press enter to finish");
	getchar();

	return 0;
}
