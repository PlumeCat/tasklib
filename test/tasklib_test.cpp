#include <tasklib.h>

#include <array>
#include <random>

using namespace std;

template<typename... Args>
void log(Args... args) {
	#ifndef NDEBUG
	(cout << ... << args) << endl;
	#endif
}

void test_task() {
	// log("test_task\n");
	// this_thread::sleep_for(chrono::microseconds(50 + rand() % 1000));
	constexpr int array_size = 500'000;
	auto arr = vector<int>();
	arr.resize(array_size);

	// create a random array
	auto n = rand() % 311;
	for (auto i = 0; i < array_size; i++) {
		arr[i] = n;
		n = n + 1000 % 237;
	}

	// sort the array
	sort(arr.begin(), arr.end());
}

TaskSet make_randomized_test_set(int seed) {
	const auto MAX_TASKS = 50;
	const auto MIN_TASKS = 10;

	auto rng = mt19937{};
	rng.seed(seed);
	auto num_tasks_dist = uniform_int_distribution(MIN_TASKS, MAX_TASKS);
	auto num_tasks = num_tasks_dist(rng);
	auto coinflip = uniform_int_distribution(0, 100);

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
			if (coinflip(rng) % 100 < 50) {
				deps.insert(names[j]);
			}
		}

		builder.add(names[i], deps, test_task);
	}

	return builder.build();
}

// deliberately non-atomic test counter for linear test
int linear_counter = 0;
void linear_init_task() {
	linear_counter = 0;
	log("Linear task (init): ", linear_counter);
}
void linear_test_task() {
	linear_counter += 1;
	log("Linear task: ", linear_counter);
}

TaskSet make_linear_test_set() {
	auto num_tasks = 100;
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

TaskSet make_tree_test_set() {
	auto builder = TaskSetBuilder{};
	return builder.build();
}

void test_randomized() {
	srand(213); // important!
	auto engine = TaskEngine(7);

	for (int i = 0; i < 100; i++) {
		auto set = make_randomized_test_set(i + 23452);
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
	// test_linear();
	// test_tree();
	return 0;
}
