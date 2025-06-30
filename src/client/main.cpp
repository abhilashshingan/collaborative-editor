// src/client/main.cpp
#include <iostream>
#ifdef QT_VERSION
#include <QApplication>
#include <QLabel>
#endif

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