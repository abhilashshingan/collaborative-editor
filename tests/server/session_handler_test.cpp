// tests/server/session_handler_test.cpp
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>
#include "server/session_handler.h"

using namespace collab::server;

class SessionHandlerTest : public ::testing::Test {
protected:
    boost::asio::io_context io_context;
    
    // Helper to create a socket
    std::shared_ptr<boost::asio::ip::tcp::socket> createSocket() {
        return std::make_shared<boost::asio::ip::tcp::socket>(io_context);
    }
};

// Test the SocketGuard RAII behavior
TEST_F(SessionHandlerTest, SocketGuardRAII) {
    auto socket = createSocket();
    
    // Create a scope for the SocketGuard
    {
        SocketGuard guard(socket);
        EXPECT_TRUE(guard.isValid());
    }
    
    // After the scope ends, the socket should be closed
    EXPECT_FALSE(socket->is_open());
}

// Test user session state tracking
TEST_F(SessionHandlerTest, UserSessionState) {
    UserSession session("test-session", "");
    
    // Initial state should be CONNECTING
    EXPECT_EQ(session.getState(), UserSession::State::CONNECTING);
    
    // Set state to AUTHENTICATING
    session.setState(UserSession::State::AUTHENTICATING);
    EXPECT_EQ(session.getState(), UserSession::State::AUTHENTICATING);
    
    // Set username and state to AUTHENTICATED
    session.setUsername("testuser");
    session.setState(UserSession::State::AUTHENTICATED);
    
    EXPECT_EQ(session.getUsername(), "testuser");
    EXPECT_EQ(session.getState(), UserSession::State::AUTHENTICATED);
}

// Test user session document tracking
TEST_F(SessionHandlerTest, UserSessionDocuments) {
    UserSession session("test-session", "testuser");
    
    // Initially, no documents
    EXPECT_TRUE(session.getActiveDocuments().empty());
    
    // Add a document
    EXPECT_TRUE(session.addDocument("doc1"));
    EXPECT_TRUE(session.hasDocument("doc1"));
    EXPECT_EQ(session.getActiveDocuments().size(), 1);
    
    // Add another document
    EXPECT_TRUE(session.addDocument("doc2"));
    EXPECT_TRUE(session.hasDocument("doc2"));
    EXPECT_EQ(session.getActiveDocuments().size(), 2);
    
    // Adding the same document again should fail
    EXPECT_FALSE(session.addDocument("doc1"));
    
    // Remove a document
    EXPECT_TRUE(session.removeDocument("doc1"));
    EXPECT_FALSE(session.hasDocument("doc1"));
    EXPECT_EQ(session.getActiveDocuments().size(), 1);
    
    // Removing a non-existent document should fail
    EXPECT_FALSE(session.removeDocument("doc3"));
}

// Test session handler basic operations
TEST_F(SessionHandlerTest, SessionHandlerBasic) {
    SessionHandler handler;
    
    // Create a session
    auto socket = createSocket();
    auto [sessionId, session] = handler.createSession(socket);
    
    EXPECT_FALSE(sessionId.empty());
    EXPECT_NE(session, nullptr);
    EXPECT_EQ(session->getState(), UserSession::State::CONNECTING);
    EXPECT_EQ(handler.getSessionCount(), 1);
    
    // Get the session by ID
    auto retrievedSession = handler.getSession(sessionId);
    EXPECT_EQ(retrievedSession, session);
    
    // Authenticate the session
    EXPECT_TRUE(handler.authenticateSession(sessionId, "testuser"));
    EXPECT_EQ(session->getUsername(), "testuser");
    EXPECT_EQ(session->getState(), UserSession::State::AUTHENTICATED);
    
    // Get session by username
    auto retrievedByUsername = handler.getSessionByUsername("testuser");
    EXPECT_EQ(retrievedByUsername, session);
    
    // Username should not be available anymore
    EXPECT_FALSE(handler.isUsernameAvailable("testuser"));
    
    // Close the session
    EXPECT_TRUE(handler.closeSession(sessionId));
    EXPECT_EQ(handler.getSessionCount(), 0);
    
    // Trying to get a closed session should return nullptr
    EXPECT_EQ(handler.getSession(sessionId), nullptr);
    
    // Username should be available again
    EXPECT_TRUE(handler.isUsernameAvailable("testuser"));
}

// Test multiple session management
TEST_F(SessionHandlerTest, MultipleSessionsAndDocuments) {
    SessionHandler handler;
    
    // Create three sessions
    auto socket1 = createSocket();
    auto socket2 = createSocket();
    auto socket3 = createSocket();
    
    auto [sessionId1, session1] = handler.createSession(socket1);
    auto [sessionId2, session2] = handler.createSession(socket2);
    auto [sessionId3, session3] = handler.createSession(socket3);
    
    EXPECT_EQ(handler.getSessionCount(), 3);
    
    // Authenticate two sessions
    EXPECT_TRUE(handler.authenticateSession(sessionId1, "user1"));
    EXPECT_TRUE(handler.authenticateSession(sessionId2, "user2"));
    
    // Open documents
    session1->addDocument("doc1");
    session1->addDocument("doc2");
    
    session2->addDocument("doc1");
    
    // Check users on documents
    auto usersOnDoc1 = handler.getUsersOnDocument("doc1");
    EXPECT_EQ(usersOnDoc1.size(), 2);
    EXPECT_TRUE(std::find(usersOnDoc1.begin(), usersOnDoc1.end(), "user1") != usersOnDoc1.end());
    EXPECT_TRUE(std::find(usersOnDoc1.begin(), usersOnDoc1.end(), "user2") != usersOnDoc1.end());
    
    auto usersOnDoc2 = handler.getUsersOnDocument("doc2");
    EXPECT_EQ(usersOnDoc2.size(), 1);
    EXPECT_EQ(usersOnDoc2[0], "user1");
    
    // Session 3 is not authenticated, so it should not be counted
    session3->addDocument("doc1");
    usersOnDoc1 = handler.getUsersOnDocument("doc1");
    EXPECT_EQ(usersOnDoc1.size(), 2); // Still 2
    
    // Close one session
    EXPECT_TRUE(handler.closeSession(sessionId2));
    EXPECT_EQ(handler.getSessionCount(), 2);
    
    // Check users on documents again
    usersOnDoc1 = handler.getUsersOnDocument("doc1");
    EXPECT_EQ(usersOnDoc1.size(), 1);
    EXPECT_EQ(usersOnDoc1[0], "user1");
}

// Test session cleanup
TEST_F(SessionHandlerTest, SessionCleanup) {
    SessionHandler handler;
    
    // Create two sessions
    auto socket1 = createSocket();
    auto socket2 = createSocket();
    
    auto [sessionId1, session1] = handler.createSession(socket1);
    auto [sessionId2, session2] = handler.createSession(socket2);
    
    EXPECT_EQ(handler.getSessionCount(), 2);
    
    // Set a very old last activity on session1
    session1->setState(UserSession::State::CONNECTING); // This updates last activity
    
    // Sleep to ensure session1 is idle
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Update activity on session2
    session2->setState(UserSession::State::CONNECTING);
    
    // Clean up sessions with idle time > 5ms
    int cleaned = handler.cleanupIdleSessions(5 / 1000); // 5ms in seconds
    EXPECT_EQ(cleaned, 1);
    EXPECT_EQ(handler.getSessionCount(), 1);
    
    // session1 should be gone, session2 should remain
    EXPECT_EQ(handler.getSession(sessionId1), nullptr);
    EXPECT_NE(handler.getSession(sessionId2), nullptr);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
