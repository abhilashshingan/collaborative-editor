// include/common/network/connection.h
#pragma once

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <memory>
#include <string>
#include <functional>

namespace collab {
namespace network {

/**
 * A Connection class that handles a single TCP connection.
 * This class is designed to be managed by a shared_ptr.
 */
class Connection : public std::enable_shared_from_this<Connection> {
public:
    /**
     * Factory method to create a new connection.
     *
     * @param io_context The Boost.Asio io_context to use
     * @return A new Connection instance wrapped in a shared_ptr
     */
    static std::shared_ptr<Connection> create(boost::asio::io_context& io_context);

    /**
     * Destructor that ensures clean shutdown.
     */
    ~Connection();

    /**
     * Gets the underlying socket.
     *
     * @return Reference to the socket
     */
    boost::asio::ip::tcp::socket& socket();

    /**
     * Starts the connection, which begins reading data.
     */
    void start();

    /**
     * Closes the connection gracefully.
     */
    void close();

    /**
     * Sends data over the connection asynchronously.
     *
     * @param data The data to send
     */
    void send(const std::string& data);

    /**
     * Callback type definition for data reception events.
     */
    using DataReceivedCallback = std::function<void(std::shared_ptr<Connection>, const std::string&)>;

    /**
     * Sets the callback to invoke when data is received.
     *
     * @param callback Function to call when data is received
     */
    void setDataReceivedCallback(DataReceivedCallback callback);

    /**
     * Callback type definition for connection closed events.
     */
    using ConnectionClosedCallback = std::function<void(std::shared_ptr<Connection>)>;

    /**
     * Sets the callback to invoke when the connection is closed.
     *
     * @param callback Function to call when connection closes
     */
    void setConnectionClosedCallback(ConnectionClosedCallback callback);

private:
    /**
     * Constructor is private, use create() factory method instead.
     *
     * @param io_context The Boost.Asio io_context to use
     */
    explicit Connection(boost::asio::io_context& io_context);

    /**
     * Asynchronously reads data from the socket.
     */
    void readData();

    /**
     * Callback for handling received data.
     *
     * @param error Error code if read failed
     * @param bytes_transferred Number of bytes received
     */
    void handleRead(const boost::system::error_code& error, std::size_t bytes_transferred);

    /**
     * Callback for handling sent data.
     *
     * @param error Error code if write failed
     * @param bytes_transferred Number of bytes sent
     */
    void handleWrite(const boost::system::error_code& error, std::size_t bytes_transferred);

    boost::asio::ip::tcp::socket socket_;
    enum { max_buffer_size = 8192 };
    char read_buffer_[max_buffer_size];
    std::string write_buffer_;
    DataReceivedCallback data_callback_;
    ConnectionClosedCallback closed_callback_;
    bool is_open_;
};

}  // namespace network
}  // namespace collab