#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("Railway Reservation System");
    window.resize(1100, 700);

    QWidget *central = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(central);

    QLabel *title = new QLabel("Railway Reservation System");
    title->setStyleSheet("font-size: 28px; font-weight: bold;");

    QLabel *message = new QLabel("Welcome! Your railway application is starting.");

    layout->addWidget(title);
    layout->addWidget(message);
    layout->addStretch();

    window.setCentralWidget(central);
    window.show();

    return app.exec();
}