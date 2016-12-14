#include <iostream>
#include <memory>
#include <chrono>
#include <thread>

#include "improbable/worker.h"

int main() {
    worker::ConnectionParameters params;
    params.WorkerType = "server";
    params.WorkerId = "server";
    params.Network.ConnectionType = worker::NetworkConnectionType::kTcp;
    params.Network.UseExternalIp = false;
    
    std::string hostname = "localhost";

    std::cout << "Parameters Initialised" << std::endl;

    std::unique_ptr<worker::Connection> connection;
    connection.reset(new worker::Connection{hostname, 7777, params});

    std::cout << "Connected to Spatial" << std::endl;

    worker::Dispatcher dispatcher;

    bool running = true;

    dispatcher.OnLogMessage([](auto msg) {
        std::cout << msg.Message << std::endl;
    });

    dispatcher.OnDisconnect([&](auto...) {
        running = false;
    });

    static const unsigned int fps = 60;
    auto t = std::chrono::high_resolution_clock::now();

    while(running) {
        auto op_list = connection->GetOpList(0);
        dispatcher.Process(op_list);

        t += std::chrono::microseconds(1000000 / fps);
        std::this_thread::sleep_until(t);
    }
}
