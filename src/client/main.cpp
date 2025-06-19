// src/client/main.cpp
#include <iostream>
#ifdef QT_VERSION
#include <QApplication>
#include <QLabel>
#endif

int main(int argc, char *argv[]) {
#ifdef QT_VERSION
    QApplication app(argc, argv);
    
    QLabel label("Collaborative Editor (Client)");
    label.setMinimumSize(400, 200);
    label.setAlignment(Qt::AlignCenter);
    label.show();
    
    std::cout << "Collaborative Editor Client started\n";
    
    return app.exec();
#else
    std::cout << "Collaborative Editor Client (console)\n";
    std::cout << "Qt not available, running in console mode\n";
    
    return 0;
#endif
}