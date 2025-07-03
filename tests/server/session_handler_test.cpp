#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>
#include "server/session_handler.h"

using namespace collab::server;

class SessionHandlerTest : public ::testing::Test {
protected:
    boost::asio::io_context io_context;
    std::shared_ptr<boost::asio::ip::tcp::socket> createSocket() {
        return std::make_shared<boost::asio::ip::tcp::socket>(io_context);
    }
};

TEST_F(SessionHandlerTest, SocketGuardRAII) {
    auto socket = createSocket();
    {
        SocketGuard guard(socket);
        EXPECT_TRUE(guard.isValid());
    }
    EXPECT_FALSE(socket->is_open());
}

TEST_F(SessionHandlerTest, UserSessionState) {
    UserSession session("test-session", "");
    EXPECT_EQ(session.getState(), UserSession::State::CONNECTING);
    session.setState(UserSession::State::AUTHENTICATING);
    EXPECT_EQ(session.getState(), UserSession::State::AUTHENTICATING);
    session.setUsername("testuser");
    session.setState(UserSession::State::AUTHENTICATED);
    EXPECT_EQ(session.getUsername(), "testuser");
    EXPECT_EQ(session.getState(), UserSession::State::AUTHENTICATED);
}

TEST_F(SessionHandlerTest, UserSessionDocuments) {
    UserSession session("test-session", "testuser");
    EXPECT_TRUE(session.getActiveDocuments().empty());
    EXPECT_TRUE(session.addDocument("doc1"));
    EXPECT_TRUE(session.hasDocument("doc1"));
    EXPECT_EQ(session.getActiveDocuments().size(), 1);
    EXPECT_TRUE(session.addDocument("doc2"));
    EXPECT_TRUE(session.hasDocument("doc2"));
    EXPECT_EQ(session.getActiveDocuments().size(), 2);
    EXPECT_FALSE(session.addDocument("doc1"));
    EXPECT_TRUE(session.removeDocument("doc1"));
    EXPECT_FALSE(session.hasDocument("doc1"));
    EXPECT_EQ(session.getActiveDocuments().size(), 1);
    EXPECT_FALSE(session.removeDocument("doc3"));
}

TEST_F(SessionHandlerTest, SessionHandlerBasic) {
    SessionHandler handler;
    auto socket = createSocket();
    auto [sessionId, session] = handler.createSession(socket);
    EXPECT_FALSE(sessionId.empty());
    EXPECT_NE(session, nullptr);
    EXPECT_EQ(session->getState(), UserSession::State::CONNECTING);
    EXPECT_EQ(handler.getSessionCount(), 1);
    auto retrievedSession = handler.getSession(sessionId);
    EXPECT_EQ(retrievedSession, session);
    EXPECT_TRUE(handler.authenticateSession(sessionId, "testuser"));
    EXPECT_EQ(session->getUsername(), "testuser");
    EXPECT_EQ(session->getState(), UserSession::State::AUTHENTICATED);
    auto retrievedByUsername = handler.getSessionByUsername("testuser");
    EXPECT_EQ(retrievedByUsername, session);
    EXPECT_FALSE(handler.isUsernameAvailable("testuser"));
    EXPECT_TRUE(handler.closeSession(sessionId));
    EXPECT_EQ(handler.getSessionCount(), 0);
    EXPECT_EQ(handler.getSession(sessionId), nullptr);
    EXPECT_TRUE(handler.isUsernameAvailable("testuser"));
}

TEST_F(SessionHandlerTest, MultipleSessionsAndDocuments) {
    SessionHandler handler;
    auto socket1 = createSocket();
    auto socket2 = createSocket();
    auto socket3 = createSocket();
    auto [sessionId1, session1] = handler.createSession(socket1);
    auto [sessionId2, session2] = handler.createSession(socket2);
    auto [sessionId3, session3] = handler.createSession(socket3);
    EXPECT_EQ(handler.getSessionCount(), 3);
    EXPECT_TRUE(handler.authenticateSession(sessionId1, "user1"));
    EXPECT_TRUE(handler.authenticateSession(sessionId2, "user2"));
    session1->addDocument("doc1");
    session1->addDocument("doc2");
    session2->addDocument("doc1");
    auto usersOnDoc1 = handler.getUsersOnDocument("doc1");
    EXPECT_EQ(usersOnDoc1.size(), 2);
    EXPECT_TRUE(std::find(usersOnDoc1.begin(), usersOnDoc1.end(), "user1") != usersOnDoc1.end());
    EXPECT_TRUE(std::find(usersOnDoc1.begin(), usersOnDoc1.end(), "user2") != usersOnDoc1.end());
    auto usersOnDoc2 = handler.getUsersOnDocument("doc2");
    EXPECT_EQ(usersOnDoc2.size(), 1);
    EXPECT_EQ(usersOnDoc2[0], "user1");
    session3->addDocument("doc1");
    usersOnDoc1 = handler.getUsersOnDocument("doc1");
    EXPECT_EQ(usersOnDoc1.size(), 2);
    EXPECT_TRUE(handler.closeSession(sessionId2));
    EXPECT_EQ(handler.getSessionCount(), 2);
    usersOnDoc1 = handler.getUsersOnDocument("doc1");
    EXPECT_EQ(usersOnDoc1.size(), 1);
    EXPECT_EQ(usersOnDoc1[0], "user1");
}

TEST_F(SessionHandlerTest, SessionCleanup) {
    SessionHandler handler;
    auto socket1 = createSocket();
    auto socket2 = createSocket();
    auto [sessionId1, session1] = handler.createSession(socket1);
    auto [sessionId2, session2] = handler.createSession(socket2);
    EXPECT_EQ(handler.getSessionCount(), 2);
    session1->setState(UserSession::State::CONNECTING);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    session2->setState(UserSession::State::CONNECTING);
    int cleaned = handler.cleanupIdleSessions(5 / 1000);
    EXPECT_EQ(cleaned, 1);
    EXPECT_EQ(handler.getSessionCount(), 1);
    EXPECT_EQ(handler.getSession(sessionId1), nullptr);
    EXPECT_NE(handler.getSession(sessionId2), nullptr);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
