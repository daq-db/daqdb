#include <FabricNode.h>
#include <stdio.h>
#include <vector>
#include <iostream>

using namespace Fabric;

int main(int argc, char *argv[])
{
	try {
	if (argc < 4 || (argc - 1) % 3 != 0) {
		throw std::string("usage: " + std::string(argv[0]) + "{<node> <addr> <serv> [<connect <addr> <serv>...]}...");
	}

	FabricAttributes attr;
	attr.setProvider("sockets");
	attr.setBufferSize(4096);


	std::vector<FabricNode *> nodes;
	std::vector<FabricConnection *> connections;
	int cnt = 0;

	FabricNode *curNode = nullptr;
	for (int i = 1; i < argc; i += 3) {
		std::string op = argv[i];
		std::string addr = argv[i + 1];
		std::string serv = argv[i + 2];

		if (op == "node") {
			curNode = new FabricNode(attr, addr, serv);

			curNode->onConnectionRequest([&] (FabricConnection &conn) -> void {
				std::cout << "Connection request " << conn.getNameStr() << " -> " << conn.getPeerStr() << std::endl;
				conn.onRecv([&] (const std::vector<uint8_t> &msg) -> void {
					std::string str(msg.begin(), msg.end());
					std::cout << cnt++ << " Received " << str << std::endl;
				});
			});

			curNode->onConnected([&] (FabricConnection &conn) -> void {
				std::cout << "Connected          " << conn.getNameStr() << " -> " << conn.getPeerStr() << std::endl;
				for (int i = 0; i < 100; i++) {
					conn.send("Hello!");
				}
			});

			curNode->onDisconnected([&] (FabricConnection &conn) -> void {
				std::cout << "Disconnected       " << conn.getNameStr() << " -> " << conn.getPeerStr() << std::endl;
			});

			curNode->listen();

			nodes.push_back(curNode);
		} else if (op == "connect") {
			if (curNode == nullptr)
				throw std::string("please create new node first");

			FabricConnection &conn = curNode->connect(addr, serv);

			connections.push_back(&conn);
		} else if (op == "connectAsync") {
			if (curNode == nullptr)
				throw std::string("please create new node first");

			FabricConnection &conn = curNode->connectAsync(addr, serv);

			connections.push_back(&conn);
		} else {
			throw std::string("invalid operation");
		}
	}

	std::cout << "Press any button to stop..." << std::endl;
	std::getchar();

	} catch (std::string &str) {
		std::cerr << "ERROR: " << str << std::endl;
	}


	return 0;
}
