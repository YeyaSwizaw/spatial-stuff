#include <iostream>
#include <chrono>
#include <thread>
#include <unordered_map>

#include "improbable/worker.h"
#include "generated/position.h"

template <typename Pred>
void handle_until_pred(worker::Connection& connection, worker::Dispatcher& dispatcher, Pred until) {
    static const unsigned int fps = 60;
    auto t = std::chrono::high_resolution_clock::now();

    while(!until()) {
        auto op_list = connection.GetOpList(0);
        dispatcher.Process(op_list);

        t += std::chrono::microseconds(1000000 / fps);
        std::this_thread::sleep_until(t);
    }
}

auto begin_connection() {
    worker::ConnectionParameters params;
    params.WorkerType = "server";
    params.WorkerId = "server";
    params.Network.ConnectionType = worker::NetworkConnectionType::kTcp;
    params.Network.UseExternalIp = false;
    
    std::string hostname = "localhost";
    return worker::Connection::ConnectAsync(hostname, 7777, params);
}

int main() {
    auto connection_attempt = begin_connection();

    worker::Dispatcher dispatcher;

    dispatcher.OnLogMessage([](auto msg) {
        std::cout << msg.Message << std::endl;
    });

    bool running = true;
    dispatcher.OnDisconnect([&](auto...) {
        running = false;
    });

    auto entity_id_requests = std::unordered_map<worker::RequestId<worker::ReserveEntityIdRequest>, std::function<void(worker::Option<worker::EntityId>)>>{};
    auto entity_requests = std::unordered_map<worker::RequestId<worker::CreateEntityRequest>, std::function<void()>>{};

    dispatcher.OnReserveEntityIdResponse([&](const worker::ReserveEntityIdResponseOp& op) {
        if(op.StatusCode == worker::StatusCode::kSuccess) {
            auto it = entity_id_requests.find(op.RequestId);
            if(it != entity_id_requests.end()) {
                (it->second)(op.EntityId);
            }

            entity_id_requests.erase(it);
        }
    });

    dispatcher.OnCreateEntityResponse([&](const worker::CreateEntityResponseOp& op) {
        if(op.StatusCode == worker::StatusCode::kSuccess) {
            auto it = entity_requests.find(op.RequestId);
            if(it != entity_requests.end()) {
                (it->second)();
            }

            entity_requests.erase(it);
        }
    });

    auto connection = connection_attempt.Get();

    auto request_id = connection.SendReserveEntityIdRequest(500);
    entity_id_requests[request_id] = [&](auto id) {
        std::cout << "ID Reserved!" << std::endl;

        worker::Entity entity;
        entity.Add<position::Position>({{50, 0, 0}});
        auto request_id = connection.SendCreateEntityRequest(entity, {"world_chunk"}, id, 500);
        entity_requests[request_id] = [&]() {
            std::cout << "Entity Created!" << std::endl;
        };
    };

    handle_until_pred(connection, dispatcher, [&](){ 
        return !running; 
    });
}
