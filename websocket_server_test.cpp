#include "websocket_server.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

class WebSocketServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        server = std::make_unique<WebSocketServer>("localhost", "8080");
    }
    
    void TearDown() override {
        server->stop();
    }
    
    std::unique_ptr<WebSocketServer> server;
};

TEST_F(WebSocketServerTest, ServerInitialization) {
    EXPECT_NO_THROW(server->start());
}

TEST_F(WebSocketServerTest, ServerStop) {
    server->start();
    EXPECT_NO_THROW(server->stop());
}

TEST_F(WebSocketServerTest, MessageBroadcasting) {
    server->start();
    
    json test_message = {
        {"type", "test"},
        {"data", "test_data"}
    };
    
    EXPECT_NO_THROW(server->broadcast(test_message));
}

TEST_F(WebSocketServerTest, MultipleStarts) {
    EXPECT_NO_THROW(server->start());
    EXPECT_NO_THROW(server->start()); // Should handle multiple starts gracefully
}

TEST_F(WebSocketServerTest, MultipleStops) {
    server->start();
    EXPECT_NO_THROW(server->stop());
    EXPECT_NO_THROW(server->stop()); // Should handle multiple stops gracefully
}

TEST_F(WebSocketServerTest, StressTest) {
    server->start();
    
    const int num_messages = 1000;
    for (int i = 0; i < num_messages; ++i) {
        json message = {
            {"id", i},
            {"data", "test_data_" + std::to_string(i)}
        };
        EXPECT_NO_THROW(server->broadcast(message));
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 