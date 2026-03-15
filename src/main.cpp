#include "mainwindow.h"

#include <QApplication>
class newwindow {
public:
    static MainWindow* w;
};
int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    
    MainWindow w;
    w.show();
    return a.exec();
}
