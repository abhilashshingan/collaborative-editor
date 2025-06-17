#include <QApplication>
#include <QLabel>
#include <iostream>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    QLabel label("Collaborative Editor (Client)");
    label.setMinimumSize(400, 200);
    label.setAlignment(Qt::AlignCenter);
    label.show();
    
    std::cout << "Collaborative Editor Client started\n";
    
    return app.exec();
}