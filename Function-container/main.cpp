#include <string>
#include <memory>
#include <cassert>

#include "function.h"

int main(int argc, char **argv)
{
	// Lamba capture and copying
	{
		Function<void(void)> function, function2;

		std::shared_ptr<std::string> str = std::make_shared<std::string>("");
		assert(str.use_count() == 1);

		function = [=](){};
		assert(str.use_count() == 1);

		function = [&](){};
		assert(str.use_count() == 1);

		function = [&]() {
			std::shared_ptr<std::string> str2 = str;
			assert(str.use_count() == 2);
		};
		assert(str.use_count() == 1);
		function();

		function = [=]() {
			std::shared_ptr<std::string> str2 = str;
			assert(str.use_count() == 3);
		};
		assert(str.use_count() == 2);
		function();

		function2 = function;
		assert(str.use_count() == 3);

		function2 = nullptr;
		assert(str.use_count() == 2);
	}

	// Test mutable lambdas
	{
		int count = 0;
		Function<int(void)> counter = [=]() mutable {
			return count++;
		};

		assert(counter() == 0);
		assert(counter() == 1);
		assert(counter() == 2);
		assert(count == 0);

		Function<int(void)> counter2 = counter;
		assert(counter() == 3);
		assert(counter2() == 3);
		assert(count == 0);
	}

#ifdef _WIN32
	std::system("pause");
#endif
	return 0;
}
