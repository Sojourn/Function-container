#include <string>
#include <thread>
#include <memory>
#include <cassert>

#include <zmq.hpp>

#include "function.h"

void testApi();
void testSerialization();

int main(int argc, char **argv)
{
	// Test std::function api compliance
	testApi();

	// Test moving functions between threads as raw byte buffers
	testSerialization();

#ifdef _WIN32
	std::system("pause");
#endif
	return 0;
}

void testApi()
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
}

void testSerialization()
{
	static const char *serverAddr = "inproc://server";
	
	zmq::context_t context(0);
	
	std::thread serverThread([&](){
		zmq::socket_t socket(context, ZMQ_REP);
		socket.bind(serverAddr);

		zmq::message_t msg;
		for (;;) {
			socket.recv(&msg);

			Work work(msg.data(), msg.size());
			int result = work();

			socket.send(&result, sizeof(int));
			if (result < 0)
				break;
		}
	});

	std::thread clientThread([&](){
		zmq::socket_t socket(context, ZMQ_REQ);
		socket.connect(serverAddr);
		
		auto dispatchWork = [&](Work work){
			{
				zmq::message_t msg(work.size());
				work.serialize(msg.data(), msg.size());
				socket.send(msg);
			}
			{
				zmq::message_t msg;
				socket.recv(&msg);

				int result = 0;
				std::memcpy(&result, msg.data(), std::min(sizeof(int), msg.size()));
				std::cout << result << std::endl;
			}
		};

		dispatchWork([](){ return strlen("1"); });
		dispatchWork([](){ return strlen("22"); });
		dispatchWork([](){ return strlen("333"); });
		dispatchWork([](){ return strlen("4444"); });
		dispatchWork([](){ return -1; });
	});

	clientThread.join();
	serverThread.join();
}