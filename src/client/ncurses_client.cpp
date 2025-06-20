// src/client/ncurses_client.cpp - A terminal-based client using ncurses
#include <ncurses.h>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <csignal>
#include <clocale>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <boost/asio.hpp>

namespace collab {
namespace client {

/**
 * TextBuffer class manages the text content with support for cursor movement
 * and text manipulation
 */
class TextBuffer {
public:
    TextBuffer() {
        // Initialize with one empty line
        lines_.push_back("");
    }

    // Add character at current cursor position
    void insertChar(char c) {
        if (c == '\n' || c == '\r') {
            // Create a new line, splitting the current line
            std::string current_line = lines_[cursor_y_];
            std::string before = current_line.substr(0, cursor_x_);
            std::string after = current_line.substr(cursor_x_);

            lines_[cursor_y_] = before;
            lines_.insert(lines_.begin() + cursor_y_ + 1, after);
            
            // Track modified lines
            modified_lines_.insert(cursor_y_);
            modified_lines_.insert(cursor_y_ + 1);
            
            cursor_y_++;
            cursor_x_ = 0;
        } else {
            lines_[cursor_y_].insert(cursor_x_, 1, c);
            cursor_x_++;
            
            // Track modified line
            modified_lines_.insert(cursor_y_);
        }
    }

    // Delete character at current cursor position
    void deleteChar() {
        if (cursor_x_ > 0) {
            // Delete character before cursor
            lines_[cursor_y_].erase(cursor_x_ - 1, 1);
            cursor_x_--;
            
            // Track modified line
            modified_lines_.insert(cursor_y_);
        } else if (cursor_y_ > 0) {
            // At start of line, merge with previous line
            int prev_line_length = lines_[cursor_y_ - 1].length();
            lines_[cursor_y_ - 1] += lines_[cursor_y_];
            lines_.erase(lines_.begin() + cursor_y_);
            cursor_y_--;
            cursor_x_ = prev_line_length;
            
            // Track modified lines and removed line
            modified_lines_.insert(cursor_y_);
            // Note: lines after the deleted one will be renumbered, so we flag for full redraw
            need_full_redraw_ = true;
        }
    }

    // Move cursor with bounds checking
    void moveCursor(int dx, int dy) {
        // Move vertically
        if (dy != 0) {
            int new_y = cursor_y_ + dy;
            if (new_y >= 0 && new_y < (int)lines_.size()) {
                cursor_y_ = new_y;
                // Adjust x position if beyond line length
                if (cursor_x_ > (int)lines_[cursor_y_].length()) {
                    cursor_x_ = lines_[cursor_y_].length();
                }
            }
        }

        // Move horizontally
        if (dx != 0) {
            int new_x = cursor_x_ + dx;
            if (new_x >= 0 && new_x <= (int)lines_[cursor_y_].length()) {
                cursor_x_ = new_x;
            }
        }
    }

    // Update buffer content from external source (server)
    void updateContent(const std::string& content) {
        // Parse content into lines
        std::vector<std::string> new_lines;
        std::istringstream stream(content);
        std::string line;
        
        while (std::getline(stream, line)) {
            new_lines.push_back(line);
        }
        
        // If last char was newline, add empty line
        if (!content.empty() && content.back() == '\n') {
            new_lines.push_back("");
        }
        
        // If empty content, ensure at least one empty line
        if (new_lines.empty()) {
            new_lines.push_back("");
        }
        
        // Find changed lines
        size_t min_size = std::min(lines_.size(), new_lines.size());
        
        // Check common lines for changes
        for (size_t i = 0; i < min_size; i++) {
            if (lines_[i] != new_lines[i]) {
                modified_lines_.insert(i);
                lines_[i] = new_lines[i];
            }
        }
        
        // If line count changed, we need special handling
        if (lines_.size() != new_lines.size()) {
            // If lines were added
            if (lines_.size() < new_lines.size()) {
                for (size_t i = lines_.size(); i < new_lines.size(); i++) {
                    lines_.push_back(new_lines[i]);
                    modified_lines_.insert(i);
                }
            }
            // If lines were removed
            else {
                lines_.resize(new_lines.size());
                need_full_redraw_ = true; // Line numbers shifted, need full redraw
            }
        }
        
        // Adjust cursor if it's now beyond the document
        if (cursor_y_ >= (int)lines_.size()) {
            cursor_y_ = lines_.size() - 1;
        }
        if (cursor_x_ > (int)lines_[cursor_y_].length()) {
            cursor_x_ = lines_[cursor_y_].length();
        }
    }

    // Getters for cursor position and text content
    int getCursorX() const { return cursor_x_; }
    int getCursorY() const { return cursor_y_; }
    const std::vector<std::string>& getLines() const { return lines_; }
    
    // Get text content as a single string
    std::string getContent() const {
        std::string content;
        for (size_t i = 0; i < lines_.size(); i++) {
            content += lines_[i];
            if (i < lines_.size() - 1) {
                content += "\n";
            }
        }
        return content;
    }
    
    // Get modified lines and clear the tracking
    std::unordered_set<int> getAndClearModifiedLines() {
        std::unordered_set<int> result = std::move(modified_lines_);
        modified_lines_ = std::unordered_set<int>();
        return result;
    }
    
    // Check if a full redraw is needed and reset the flag
    bool needFullRedrawAndReset() {
        bool result = need_full_redraw_;
        need_full_redraw_ = false;
        return result;
    }

private:
    std::vector<std::string> lines_;
    int cursor_x_ = 0;
    int cursor_y_ = 0;
    std::unordered_set<int> modified_lines_; // Tracks which lines have been modified
    bool need_full_redraw_ = true; // Initially true to draw everything
};

/**
 * NetworkClient handles WebSocket communication with server
 */
class NetworkClient {
public:
    NetworkClient(const std::string& host, unsigned short port)
        : io_context_(),
          socket_(io_context_),
          connected_(false),
          host_(host),
          port_(port),
          running_(true) {
    }

    ~NetworkClient() {
        stop();
    }

    bool connect() {
        try {
            boost::asio::ip::tcp::resolver resolver(io_context_);
            boost::asio::ip::tcp::resolver::results_type endpoints = 
                resolver.resolve(host_, std::to_string(port_));

            boost::asio::connect(socket_, endpoints);
            connected_ = true;

            // Start network thread
            network_thread_ = std::thread([this]() {
                try {
                    // Start reading from server
                    startRead();
                    io_context_.run();
                } catch (const std::exception& e) {
                    std::cerr << "Network error: " << e.what() << std::endl;
                }
            });

            return true;
        } catch (const std::exception& e) {
            std::cerr << "Connection error: " << e.what() << std::endl;
            return false;
        }
    }

    void sendTextUpdate(const std::string& content) {
        if (!connected_) return;
        
        auto message = "UPDATE " + content + "\n";
        
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(message),
            [this](const boost::system::error_code& error, std::size_t /*bytes_transferred*/) {
                if (error) {
                    connected_ = false;
                    std::cerr << "Error sending update: " << error.message() << std::endl;
                }
            });
    }

    bool isConnected() const {
        return connected_;
    }

    void stop() {
        if (!running_.exchange(false)) {
            return; // Already stopped
        }

        io_context_.stop();
        
        if (network_thread_.joinable()) {
            network_thread_.join();
        }

        if (socket_.is_open()) {
            boost::system::error_code ec;
            socket_.close(ec);
        }
    }
    
    // Register callback for receiving text updates from server
    void setTextUpdateCallback(std::function<void(const std::string&)> callback) {
        update_callback_ = callback;
    }

private:
    // Start reading from the server
    void startRead() {
        if (!running_ || !connected_) return;
        
        boost::asio::async_read_until(
            socket_,
            receive_buffer_,
            '\n',
            [this](const boost::system::error_code& error, std::size_t bytes_transferred) {
                if (!error) {
                    // Process received data
                    std::istream is(&receive_buffer_);
                    std::string line;
                    std::getline(is, line);
                    
                    // Process the line
                    processReceivedLine(line);
                    
                    // Continue reading
                    startRead();
                } else {
                    connected_ = false;
                    
                    if (error != boost::asio::error::operation_aborted) {
                        std::cerr << "Read error: " << error.message() << std::endl;
                    }
                }
            });
    }
    
    // Process a line received from the server
    void processReceivedLine(const std::string& line) {
        if (line.substr(0, 7) == "CONTENT") {
            // Content update from server
            std::string content = line.substr(8); // Skip "CONTENT "
            
            // Call the callback if registered
            if (update_callback_) {
                update_callback_(content);
            }
        }
    }

    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::streambuf receive_buffer_;
    std::atomic<bool> connected_;
    std::string host_;
    unsigned short port_;
    std::atomic<bool> running_;
    std::thread network_thread_;
    std::function<void(const std::string&)> update_callback_;
};

/**
 * NcursesClient class implements a terminal-based text editor using ncurses
 */
class NcursesClient {
public:
    NcursesClient(const std::string& server_host = "localhost", unsigned short server_port = 8080)
        : buffer_(),
          network_(server_host, server_port),
          running_(true),
          status_message_("Collaborative Editor - Press F1 for help"),
          redraw_needed_(true) {
        
        // Initialize ncurses
        setlocale(LC_ALL, "");  // For wide character support
        initscr();              // Initialize ncurses
        raw();                  // Disable line buffering
        keypad(stdscr, TRUE);   // Enable special keys
        noecho();               // Don't echo keypresses
        curs_set(1);            // Show cursor
        
        // Enable colors if supported
        if (has_colors()) {
            start_color();
            init_pair(1, COLOR_WHITE, COLOR_BLUE);    // Status bar
            init_pair(2, COLOR_GREEN, COLOR_BLACK);   // Connected status
            init_pair(3, COLOR_RED, COLOR_BLACK);     // Disconnected status
            init_pair(4, COLOR_BLACK, COLOR_GREEN);   // Highlighted line (server edit)
        }
        
        // Register callback for server updates
        network_.setTextUpdateCallback([this](const std::string& content) {
            {
                std::lock_guard<std::mutex> lock(update_mutex_);
                // Save current cursor position
                cursor_before_update_x_ = buffer_.getCursorX();
                cursor_before_update_y_ = buffer_.getCursorY();
                
                // Update the buffer
                buffer_.updateContent(content);
                
                redraw_needed_ = true;
            }
        });

        // Try to connect to the server
        if (network_.connect()) {
            setStatusMessage("Connected to server");
        } else {
            setStatusMessage("Failed to connect to server - working offline", 3);
        }

        // Set up signal handler for resize events
        signal(SIGWINCH, [](int) {
            endwin();
            refresh();
        });
        
        // Initialize window size
        getmaxyx(stdscr, max_y_, max_x_);
    }

    ~NcursesClient() {
        endwin();  // Clean up ncurses
    }

    void run() {
        while (running_) {
            if (redraw_needed_) {
                render();
                redraw_needed_ = false;
            }
            
            // Set timeout for getch to check for updates periodically
            timeout(100); // 100ms timeout
            int ch = getch();
            
            if (ch != ERR) { // ERR is returned on timeout
                handleKeypress(ch);
            }
        }
    }

    void stop() {
        running_ = false;
        network_.stop();
    }

private:
    // Handle keyboard input
    void handleKeypress(int key) {
        std::lock_guard<std::mutex> lock(update_mutex_);
        
        switch(key) {
            case KEY_F(1):
                showHelp();
                break;
                
            case KEY_F(2):
                if (network_.isConnected()) {
                    setStatusMessage("Already connected");
                } else if (network_.connect()) {
                    setStatusMessage("Connected to server");
                    // Send current text to sync
                    network_.sendTextUpdate(buffer_.getContent());
                } else {
                    setStatusMessage("Connection failed", 3);
                }
                break;
                
            case KEY_F(10):
                setStatusMessage("Exiting...");
                running_ = false;
                break;
                
            case KEY_UP:
                buffer_.moveCursor(0, -1);
                break;
                
            case KEY_DOWN:
                buffer_.moveCursor(0, 1);
                break;
                
            case KEY_LEFT:
                buffer_.moveCursor(-1, 0);
                break;
                
            case KEY_RIGHT:
                buffer_.moveCursor(1, 0);
                break;
                
            case KEY_BACKSPACE:
            case 127:
                buffer_.deleteChar();
                redraw_needed_ = true;
                break;
                
            case KEY_ENTER:
            case '\n':
            case '\r':
                buffer_.insertChar('\n');
                redraw_needed_ = true;
                break;
                
            default:
                if (key >= 32 && key <= 126) {  // Printable ASCII
                    buffer_.insertChar(key);
                    redraw_needed_ = true;
                }
                break;
        }
        
        // Send updates to server
        if (network_.isConnected() && 
            (key == KEY_BACKSPACE || key == 127 || key == KEY_ENTER || key == '\n' || key == '\r' || 
             (key >= 32 && key <= 126))) {
            network_.sendTextUpdate(buffer_.getContent());
        }
    }

    // Render the current state
    void render() {
        std::lock_guard<std::mutex> lock(update_mutex_);
        
        // Get window size (in case of resize)
        getmaxyx(stdscr, max_y_, max_x_);
        
        // Calculate viewable area
        int editor_height = max_y_ - 2;  // Reserve bottom 2 lines for status
        
        // Check if we need a full redraw
        bool full_redraw = buffer_.needFullRedrawAndReset();
        
        if (full_redraw) {
            // Clear screen for full redraw
            clear();
        }
        
        // Get modified lines
        auto modified_lines = buffer_.getAndClearModifiedLines();
        
        // Render content - either all lines or just modified ones
        const auto& lines = buffer_.getLines();
        if (full_redraw) {
            // Draw all lines
            for (int y = 0; y < editor_height && y < (int)lines.size(); y++) {
                mvprintw(y, 0, "%s", lines[y].c_str());
                // Clear to end of line to ensure old text is removed
                clrtoeol();
            }
        } else {
            // Draw only modified lines
            for (auto y : modified_lines) {
                if (y >= 0 && y < editor_height && y < (int)lines.size()) {
                    // Highlight line if it was updated from server
                    if (y != cursor_before_update_y_) {
                        attron(COLOR_PAIR(4));
                        mvprintw(y, 0, "%s", lines[y].c_str());
                        attroff(COLOR_PAIR(4));
                    } else {
                        mvprintw(y, 0, "%s", lines[y].c_str());
                    }
                    // Clear to end of line to ensure old text is removed
                    clrtoeol();
                }
            }
        }
        
        // Render status bar
        attron(COLOR_PAIR(1));
        mvhline(max_y_ - 2, 0, ' ', max_x_);
        mvprintw(max_y_ - 2, 0, " %s", status_message_.c_str());
        
        // Show connection status
        int status_color = network_.isConnected() ? 2 : 3;
        std::string status = network_.isConnected() ? "CONNECTED" : "OFFLINE";
        mvprintw(max_y_ - 2, max_x_ - status.length() - 2, "%s ", status.c_str());
        attroff(COLOR_PAIR(1));
        
        // Render cursor position information
        mvprintw(max_y_ - 1, 0, "Line: %d Col: %d", buffer_.getCursorY() + 1, buffer_.getCursorX() + 1);
        
        // Position cursor
        move(buffer_.getCursorY(), buffer_.getCursorX());
        
        refresh();
    }

    // Display help screen
    void showHelp() {
        clear();
        
        mvprintw(0, 0, "Collaborative Text Editor - Help");
        mvprintw(2, 0, "F1      - Show this help");
        mvprintw(3, 0, "F2      - Connect to server");
        mvprintw(4, 0, "F10     - Exit");
        mvprintw(5, 0, "Arrows  - Move cursor");
        mvprintw(6, 0, "");
        mvprintw(7, 0, "Collaborative Features:");
        mvprintw(8, 0, "- Server edits are highlighted in green");
        mvprintw(9, 0, "- Real-time updates from other users");
        
        mvprintw(11, 0, "Press any key to return to editor");
        
        refresh();
        getch();
        redraw_needed_ = true;
    }

    // Set status message with optional color
    void setStatusMessage(const std::string& message, int color_pair = 0) {
        status_message_ = message;
    }

    TextBuffer buffer_;
    NetworkClient network_;
    bool running_;
    std::string status_message_;
    std::atomic<bool> redraw_needed_; // Flag to indicate redraw is needed
    std::mutex update_mutex_; // Mutex for synchronizing buffer updates
    int cursor_before_update_x_ = 0; // Cursor X position before server update
    int cursor_before_update_y_ = 0; // Cursor Y position before server update
    int max_y_ = 0; // Terminal height
    int max_x_ = 0; // Terminal width
};

} // namespace client
} // namespace collab

// Main function
int main(int argc, char* argv[]) {
    try {
        std::string server_host = "localhost";
        unsigned short server_port = 8080;
        
        // Parse command line arguments if provided
        if (argc >= 2) {
            server_host = argv[1];
        }
        
        if (argc >= 3) {
            server_port = std::stoi(argv[2]);
        }
        
        collab::client::NcursesClient client(server_host, server_port);
        client.run();
        
    } catch (const std::exception& e) {
        endwin(); // Clean up ncurses before showing error
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}