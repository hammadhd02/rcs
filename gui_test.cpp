#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("Railway Reservation System");
    window.resize(800, 500);

    auto *layout = new QVBoxLayout(&window);
    auto *title = new QLabel("RAILWAY RESERVATION SYSTEM");
    auto *search = new QPushButton("Search Trains");
    auto *book = new QPushButton("Book Ticket");
    auto *view = new QPushButton("View Bookings");
    auto *cancel = new QPushButton("Cancel Ticket");

    title->setStyleSheet("font-size: 26px; font-weight: bold;");
    layout->addWidget(title);
    layout->addWidget(search);
    layout->addWidget(book);
    layout->addWidget(view);
    layout->addWidget(cancel);

    window.show();
    return app.exec();
}