#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QTimer>
#include <QUrl>
#include <QInputDialog>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QRegularExpression>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QDate>
#include <QDateEdit>
#include <QDateEdit>
#include <QFrame>
#include <QPixmap>
#include <QScrollArea>
#include <QAbstractItemView>
#include <QVector>
#include <QStringList>
#include <QTableWidgetItem>
#include <functional>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <vector>
#include <list>
#include <queue>
#include <string>

using namespace std;

struct Train { int number=0,totalSeats=0,availableSeats=0; string name,source,destination; };
struct Passenger { int id=0,age=0; string name,gender; };
struct Booking { int id=0,passengerId=0,trainNumber=0; string date; bool active=true; };
struct Waiting { int passengerId=0,trainNumber=0; };

vector<Train> trains;
list<Passenger> passengers;
vector<Booking> bookings;
queue<Waiting> waitingList;
int nextPassengerId=1,nextBookingId=1;

QString qs(const string &s){ return QString::fromStdString(s); }
string ss(const QString &s){ return s.toStdString(); }

int findTrain(int n){
    for(int i=0;i<(int)trains.size();++i) if(trains[i].number==n) return i;
    return -1;
}
bool passengerExists(int id){
    for(const auto&p:passengers) if(p.id==id) return true;
    return false;
}

void saveData(){
    ofstream f("trains.txt");
    for(const auto&t:trains) f<<t.number<<' '<<quoted(t.name)<<' '<<quoted(t.source)<<' '<<quoted(t.destination)<<' '<<t.totalSeats<<' '<<t.availableSeats<<'\n';
    ofstream p("passengers.txt");
    for(const auto&x:passengers) p<<x.id<<' '<<x.age<<' '<<quoted(x.name)<<' '<<quoted(x.gender)<<'\n';
    ofstream b("bookings.txt");
    for(const auto&x:bookings) b<<x.id<<' '<<x.passengerId<<' '<<x.trainNumber<<' '<<quoted(x.date)<<' '<<x.active<<'\n';
    ofstream w("waiting.txt");
    auto q=waitingList;
    while(!q.empty()){ auto x=q.front();q.pop();w<<x.passengerId<<' '<<x.trainNumber<<'\n'; }
}
void loadData(){
    ifstream f("trains.txt"); Train t;
    while(f>>t.number>>quoted(t.name)>>quoted(t.source)>>quoted(t.destination)>>t.totalSeats>>t.availableSeats) trains.push_back(t);
    ifstream p("passengers.txt"); Passenger x;
    while(p>>x.id>>x.age>>quoted(x.name)>>quoted(x.gender)){ passengers.push_back(x);nextPassengerId=max(nextPassengerId,x.id+1); }
    ifstream b("bookings.txt"); Booking y;
    while(b>>y.id>>y.passengerId>>y.trainNumber>>quoted(y.date)>>y.active){ bookings.push_back(y);nextBookingId=max(nextBookingId,y.id+1); }
    ifstream w("waiting.txt"); Waiting z;
    while(w>>z.passengerId>>z.trainNumber) waitingList.push(z);
}

class Window: public QWidget {    QNetworkAccessManager *network = new QNetworkAccessManager(this);
    QString apiKey;

    QDialog *liveDialog = nullptr;
    QLabel *liveDetails = nullptr;
    QTimer *liveTimer = nullptr;
    QString activeLiveTrain;

    QString toIST(const QString &value) {
        if (value.isEmpty()) return "Not available";
        QDateTime dt = QDateTime::fromString(value, Qt::ISODateWithMs);
        if (!dt.isValid()) dt = QDateTime::fromString(value, Qt::ISODate);
        if (!dt.isValid()) return value;
        if (dt.timeSpec() == Qt::LocalTime && !value.endsWith('Z') &&
            !value.contains(QRegularExpression("[+-][0-9]{2}:[0-9]{2}$"))) {
            dt.setTimeSpec(Qt::UTC);
        }
        return dt.toUTC().addSecs(19800).toString("dd MMM yyyy, hh:mm:ss") + " IST";
    }

    void showLiveDialog() {
        if (liveDialog) {
            liveDialog->show();
            liveDialog->raise();
            return;
        }
        liveDialog = new QDialog(this);
        liveDialog->setAttribute(Qt::WA_DeleteOnClose);
        liveDialog->setWindowTitle("Live Train Status — IST");
        liveDialog->resize(520, 330);
        liveDialog->setStyleSheet("QDialog{background:#f4f7fb;color:#17365d;} QLabel{background:white;border:1px solid #dbe4f0;border-radius:10px;padding:14px;font-size:11pt;color:#17365d;}");
        auto *layout = new QVBoxLayout(liveDialog);
        auto *heading = new QLabel("🚆 LIVE RUNNING STATUS");
        heading->setStyleSheet("background:transparent;border:0;font-size:18pt;font-weight:900;color:#17365d;padding:4px;");
        liveDetails = new QLabel("Connecting to RailRadar...");
        liveDetails->setWordWrap(true);
        liveDetails->setMinimumHeight(180);
        auto *close = new QPushButton("Stop auto-refresh and close");
        close->setMinimumHeight(42);
        layout->addWidget(heading);
        layout->addWidget(liveDetails);
        layout->addWidget(close);
        connect(close, &QPushButton::clicked, liveDialog, &QDialog::close);
        connect(liveDialog, &QObject::destroyed, this, [this]() {
            liveDialog = nullptr;
            liveDetails = nullptr;
            activeLiveTrain.clear();
            if (liveTimer) liveTimer->stop();
        });
        liveDialog->show();
    }

    void checkLiveTrain(const QString &number) {
        if (apiKey.isEmpty()) {
            bool ok = false;
            QString entered = QInputDialog::getText(this, "RailRadar API",
                "Enter your RailRadar API key (stored only for this app session):",
                QLineEdit::Password, "", &ok);
            if (!ok || entered.trimmed().isEmpty()) return;
            apiKey = entered.trimmed();
        }

        activeLiveTrain = number;
        showLiveDialog();
        if (liveDetails) liveDetails->setText(
            QString("Train %1\nFetching latest report...\nTimezone: Indian Standard Time (IST, UTC+05:30)")
                .arg(number));

        if (!liveTimer) {
            liveTimer = new QTimer(this);
            liveTimer->setInterval(30000);
            connect(liveTimer, &QTimer::timeout, this, [this]() {
                if (!activeLiveTrain.isEmpty() && liveDialog)
                    requestLiveStatus(activeLiveTrain);
            });
        }
        liveTimer->start();
        requestLiveStatus(number);
    }

    void requestLiveStatus(const QString &number) {
        QUrl url("https://api.railradar.in/v1/trains/" + number + "/live");
        QNetworkRequest request(url);
        request.setRawHeader("Authorization", QByteArray("Bearer ") + apiKey.toUtf8());
        request.setRawHeader("Accept", "application/json");
        auto *reply = network->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, number]() {
            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QByteArray body = reply->readAll();
            auto error = reply->error();
            reply->deleteLater();

            if (number != activeLiveTrain || !liveDetails) return;
            if (error != QNetworkReply::NoError) {
                if (status == 401) apiKey.clear();
                liveDetails->setText(QString("Train %1\nLive status could not be loaded.\nHTTP: %2\n%3\n\nAuto-refresh: every 30 seconds\nTimezone: IST (UTC+05:30)")
                    .arg(number).arg(status).arg(QString::fromUtf8(body).left(500)));
                return;
            }

            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
            if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                liveDetails->setText("The API response was not valid JSON.");
                return;
            }

            QJsonObject root = doc.object();
            if (root.contains("success") && !root.value("success").toBool()) {
                QString message = root.value("error").toObject().value("message").toString("Live status unavailable.");
                liveDetails->setText("Train " + number + "\n" + message + "\n\nAuto-refresh: every 30 seconds\nTimezone: IST (UTC+05:30)");
                return;
            }

            QJsonObject data = root.value("data").toObject();
            QJsonObject train = data.value("train").toObject();
            QJsonObject location = data.value("currentLocation").toObject();
            QJsonObject next = data.value("nextHalt").toObject();
            QString name = data.value("trainName").toString(train.value("name").toString("Not available"));
            QString trainNo = data.value("trainNumber").toVariant().toString();
            if (trainNo.isEmpty()) trainNo = number;
            QString station = location.value("stationName").toString();
            if (station.isEmpty()) station = location.value("stationCode").toString("Not available");
            QString nextStation = next.value("stationName").toString("Not available");
            QString delay = data.contains("delayMinutes")
                ? QString::number(data.value("delayMinutes").toInt()) + " minutes"
                : "Not available";
            QString statusText = data.value("status").toString("Not available");
            QString updated = toIST(data.value("lastUpdatedAt").toString());

            liveDetails->setText(
                QString("Train: %1 (%2)\nRunning status: %3\nCurrent location: %4\nNext station: %5\nDelay: %6\nLast updated: %7\n\nAuto-refresh: every 30 seconds\nTrain times shown in IST (UTC+05:30)")
                .arg(name, trainNo, statusText, station, nextStation, delay, updated));
        });
    }
    QLabel *stats;
    QLineEdit *fromSearch,*toSearch,*trainNumberSearch;
    QDateEdit *journeyDate;

    QLabel* makeLabel(const QString &text,const QString &style=""){
        auto *l=new QLabel(text);
        if(!style.isEmpty()) l->setStyleSheet(style);
        return l;
    }

    QPushButton* makeButton(const QString &text,const QString &kind="primary"){
        auto *b=new QPushButton(text);
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(44);
        if(kind=="accent") b->setStyleSheet("QPushButton{background:#ff6d38;color:white;border:0;border-radius:9px;padding:10px 18px;font-weight:700;} QPushButton:hover{background:#e95a28;}");
        else if(kind=="light") b->setStyleSheet("QPushButton{background:white;color:#183b70;border:1px solid #dbe4f0;border-radius:9px;padding:10px 16px;font-weight:650;} QPushButton:hover{background:#edf4ff;}");
        else b->setStyleSheet("QPushButton{background:#1769aa;color:white;border:0;border-radius:9px;padding:10px 16px;font-weight:700;} QPushButton:hover{background:#10558c;}");
        return b;
    }

    QFrame* card(){
        auto *f=new QFrame;
        f->setObjectName("card");
        f->setStyleSheet("QFrame#card{background:white;border:1px solid #e5ebf3;border-radius:14px;}");
        return f;
    }

    void table(QString title,QStringList headers,QVector<QStringList> rows){
        QDialog d(this);
        d.setWindowTitle(title); d.resize(900,500);
        d.setStyleSheet("QDialog{background:#f4f7fb;color:#1e293b;} QLabel{color:#17365d;}");
        auto *l=new QVBoxLayout(&d); l->setContentsMargins(18,18,18,18);
        auto *heading=makeLabel(title,"font-size:20px;font-weight:800;color:#17365d;padding:4px;");
        l->addWidget(heading);
        auto *t=new QTableWidget(rows.size(),headers.size());
        t->setHorizontalHeaderLabels(headers);
        t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        t->setEditTriggers(QAbstractItemView::NoEditTriggers);
        t->setSelectionBehavior(QAbstractItemView::SelectRows);
        t->setAlternatingRowColors(true);
        t->setStyleSheet("QTableWidget{background:white;alternate-background-color:#f2f7fd;border:1px solid #dce5f0;border-radius:8px;gridline-color:#e5ebf3;} QHeaderView::section{background:#17365d;color:white;padding:10px;border:0;font-weight:700;}");
        for(int r=0;r<rows.size();++r) for(int c=0;c<headers.size();++c) t->setItem(r,c,new QTableWidgetItem(rows[r][c]));
        l->addWidget(t);
        auto *close=makeButton("Close","light"); l->addWidget(close);
        connect(close,&QPushButton::clicked,&d,&QDialog::accept);
        d.exec();
    }

    void refresh(){
        int active=0,seats=0;
        for(auto b:bookings) if(b.active) ++active;
        for(auto t:trains) seats+=t.availableSeats;
        stats->setText(QString("%1 trains     •     %2 passengers     •     %3 active bookings     •     %4 seats available")
                       .arg(trains.size()).arg(passengers.size()).arg(active).arg(seats));
    }

    void addTrain(){
        QDialog d(this); d.setWindowTitle("Add Train"); d.setStyleSheet("QDialog{background:#f4f7fb;} QLabel{color:#17365d;font-weight:600;}");
        QFormLayout f(&d); f.setContentsMargins(22,22,22,22); f.setSpacing(12);
        QSpinBox no,seats; no.setRange(1,999999); seats.setRange(1,100000);
        QLineEdit name,src,dst;
        f.addRow("Train number",&no); f.addRow("Train name",&name); f.addRow("Source station",&src); f.addRow("Destination station",&dst); f.addRow("Total seats",&seats);
        QDialogButtonBox box(QDialogButtonBox::Ok|QDialogButtonBox::Cancel); f.addRow(&box);
        connect(&box,&QDialogButtonBox::accepted,&d,&QDialog::accept); connect(&box,&QDialogButtonBox::rejected,&d,&QDialog::reject);
        if(d.exec()!=QDialog::Accepted) return;
        if(name.text().trimmed().isEmpty()||src.text().trimmed().isEmpty()||dst.text().trimmed().isEmpty()){ QMessageBox::warning(this,"Missing details","Fill all fields."); return; }
        if(findTrain(no.value())!=-1){ QMessageBox::warning(this,"Duplicate","Train number already exists."); return; }
        trains.push_back({no.value(),seats.value(),seats.value(),ss(name.text().trimmed()),ss(src.text().trimmed()),ss(dst.text().trimmed())});
        saveData(); refresh(); QMessageBox::information(this,"Success","Train added successfully.");
    }

    void searchTrain(){
        QString from=fromSearch?fromSearch->text().trimmed():QString();
        QString to=toSearch?toSearch->text().trimmed():QString();
        QVector<QStringList> rows;
        for(const auto&t:trains){
            bool matchesFrom=from.isEmpty()||qs(t.source).contains(from,Qt::CaseInsensitive);
            bool matchesTo=to.isEmpty()||qs(t.destination).contains(to,Qt::CaseInsensitive);
            if(matchesFrom&&matchesTo) rows.push_back({QString::number(t.number),qs(t.name),qs(t.source),qs(t.destination),QString::number(t.availableSeats),QString::number(t.totalSeats)});
        }
        if(rows.isEmpty()){ QMessageBox::information(this,"No trains found","No matching trains found. Try another station or add a train first."); return; }
        table("Available Trains",{"Train No.","Train Name","From","To","Seats Left","Total Seats"},rows);
    }

    void registerPassenger(){
        QDialog d(this); d.setWindowTitle("Register Passenger"); d.setStyleSheet("QDialog{background:#f4f7fb;} QLabel{color:#17365d;font-weight:600;}");
        QFormLayout f(&d); f.setContentsMargins(22,22,22,22); f.setSpacing(12);
        QLineEdit name; QSpinBox age; age.setRange(1,120); QComboBox gender; gender.addItems({"Male","Female","Other"});
        f.addRow("Full name",&name); f.addRow("Age",&age); f.addRow("Gender",&gender);
        QDialogButtonBox box(QDialogButtonBox::Ok|QDialogButtonBox::Cancel); f.addRow(&box);
        connect(&box,&QDialogButtonBox::accepted,&d,&QDialog::accept); connect(&box,&QDialogButtonBox::rejected,&d,&QDialog::reject);
        if(d.exec()!=QDialog::Accepted) return;
        if(name.text().trimmed().isEmpty()){ QMessageBox::warning(this,"Missing name","Enter passenger name."); return; }
        Passenger p{nextPassengerId++,age.value(),ss(name.text().trimmed()),ss(gender.currentText())};
        passengers.push_back(p); saveData(); refresh();
        QMessageBox::information(this,"Registered",QString("Passenger registered.\nPassenger ID: %1").arg(p.id));
    }

    void viewPassengers(){
        QVector<QStringList> r;
        for(auto p:passengers) r.push_back({QString::number(p.id),qs(p.name),QString::number(p.age),qs(p.gender)});
        table("Registered Passengers",{"ID","Name","Age","Gender"},r);
    }

    void book(){
        if(passengers.empty()||trains.empty()){ QMessageBox::warning(this,"Cannot book","Add a train and register a passenger first."); return; }
        QDialog d(this); d.setWindowTitle("Book Train Ticket"); d.setStyleSheet("QDialog{background:#f4f7fb;} QLabel{color:#17365d;font-weight:600;}");
        QFormLayout f(&d); f.setContentsMargins(22,22,22,22); f.setSpacing(12);
        QComboBox pb,tb;
        for(auto p:passengers) pb.addItem(QString("%1 - %2").arg(p.id).arg(qs(p.name)),p.id);
        for(auto t:trains) tb.addItem(QString("%1 - %2 | %3 to %4 | %5 seats")
            .arg(t.number).arg(qs(t.name),qs(t.source),qs(t.destination)).arg(t.availableSeats),t.number);
        f.addRow("Passenger",&pb); f.addRow("Train",&tb);
        QDialogButtonBox box(QDialogButtonBox::Ok|QDialogButtonBox::Cancel); f.addRow(&box);
        connect(&box,&QDialogButtonBox::accepted,&d,&QDialog::accept); connect(&box,&QDialogButtonBox::rejected,&d,&QDialog::reject);
        if(d.exec()!=QDialog::Accepted) return;
        int pid=pb.currentData().toInt(),tn=tb.currentData().toInt(),i=findTrain(tn);
        if(i<0) return;
        if(trains[i].availableSeats<=0){ waitingList.push({pid,tn}); saveData(); QMessageBox::information(this,"Waiting list","No seats available. Passenger added to waiting list."); return; }
        Booking b{nextBookingId++,pid,tn,ss(QDate::currentDate().toString("yyyy-MM-dd")),true};
        bookings.push_back(b); --trains[i].availableSeats; saveData(); refresh();
        QMessageBox::information(this,"Booking Confirmed",QString("Your ticket is confirmed!\n\nBooking ID: %1\nPassenger ID: %2\nTrain: %3\nJourney date: %4")
            .arg(b.id).arg(pid).arg(tn).arg(qs(b.date)));
    }

    void viewBookings(){
        QVector<QStringList> r;
        for(auto b:bookings) r.push_back({QString::number(b.id),QString::number(b.passengerId),QString::number(b.trainNumber),qs(b.date),b.active?"Confirmed":"Cancelled"});
        table("Bookings",{"Booking ID","Passenger ID","Train","Date","Status"},r);
    }

    void cancel(){
        bool ok; int id=QInputDialog::getInt(this,"Cancel Ticket","Booking ID:",1,1,999999999,1,&ok);
        if(!ok) return;
        for(auto &b:bookings) if(b.id==id&&b.active){
            b.active=false; int i=findTrain(b.trainNumber); bool assigned=false;
            if(i>=0){
                queue<Waiting> rest;
                while(!waitingList.empty()){
                    auto w=waitingList.front(); waitingList.pop();
                    if(!assigned&&w.trainNumber==b.trainNumber&&passengerExists(w.passengerId)){
                        Booking nb{nextBookingId++,w.passengerId,w.trainNumber,ss(QDate::currentDate().toString("yyyy-MM-dd")),true};
                        bookings.push_back(nb); assigned=true;
                    } else rest.push(w);
                }
                waitingList=rest;
                if(!assigned) ++trains[i].availableSeats;
            }
            saveData(); refresh();
            QMessageBox::information(this,"Ticket Cancelled",assigned?"Ticket cancelled. A waiting-list passenger received the seat.":"Ticket cancelled successfully.");
            return;
        }
        QMessageBox::warning(this,"Not found","Active booking ID not found.");
    }

    void viewWaiting(){
        QVector<QStringList> r; auto q=waitingList; int pos=1;
        while(!q.empty()){ auto w=q.front(); q.pop(); r.push_back({QString::number(pos++),QString::number(w.passengerId),QString::number(w.trainNumber)}); }
        table("Waiting List",{"Position","Passenger ID","Train No."},r);
    }

    void reports(){
        QVector<QStringList> r;
        for(auto t:trains){
            int occ=t.totalSeats-t.availableSeats;
            double pc=t.totalSeats?100.0*occ/t.totalSeats:0;
            r.push_back({qs(t.name),QString::number(t.number),QString::number(occ),QString::number(t.availableSeats),QString::number(pc,'f',1)+"%"});
        }
        table("Seat Occupancy Report",{"Train","Number","Occupied","Available","Occupancy"},r);
        bool ok;
        QString date=QInputDialog::getText(this,"Daily Reservations","Date (YYYY-MM-DD):",QLineEdit::Normal,QDate::currentDate().toString("yyyy-MM-dd"),&ok);
        if(ok){ int n=0; for(auto b:bookings) if(b.active&&qs(b.date)==date) ++n; QMessageBox::information(this,"Daily Report",QString("Confirmed reservations on %1: %2").arg(date).arg(n)); }
    }

    void quickSearch(){
        QString from=fromSearch->text().trimmed(),to=toSearch->text().trimmed();
        if(from.isEmpty()||to.isEmpty()){
            QMessageBox::information(this,"Enter stations","Enter both From and To stations to search. You can also use the Search Trains button to browse all trains.");
            return;
        }
        searchTrain();
    }

public:
    Window(){
        setWindowTitle("🚆 Indian Railway Reservation System");
        resize(1180,780);
        setMinimumSize(980,680);
        setStyleSheet(
            "QWidget{background:#f4f7fb;color:#172b4d;font-family:'Segoe UI';font-size:10pt;}"
            "QLineEdit,QComboBox,QSpinBox,QDateEdit{background:white;border:1px solid #d7e0ed;border-radius:8px;padding:11px;color:#172b4d;}"
            "QLineEdit:focus,QComboBox:focus,QDateEdit:focus{border:1px solid #1769aa;}"
            "QLabel#muted{color:#6b7b93;}");

        auto *root=new QVBoxLayout(this);
        root->setContentsMargins(0,0,0,0); root->setSpacing(0);

        auto *top=new QWidget;
        top->setStyleSheet("background:#17365d;");
        auto *topLayout=new QHBoxLayout(top);
        topLayout->setContentsMargins(28,14,28,14);
        auto *brand=new QLabel("🚆  INDIAN RAILWAYS");
        brand->setStyleSheet("color:white;font-size:23px;font-weight:900;letter-spacing:2px;");
        auto *brandSub=new QLabel("SAFE JOURNEY  •  SIMPLE BOOKING  •  BETTER TRAVEL");
        brandSub->setStyleSheet("color:#b9d8f5;font-size:9px;font-weight:700;letter-spacing:2px;");
        auto *brandBox=new QVBoxLayout; brandBox->setSpacing(0); brandBox->addWidget(brand); brandBox->addWidget(brandSub);
        topLayout->addLayout(brandBox); topLayout->addStretch();
        auto *topTag=new QLabel("🚉  PLAN YOUR JOURNEY");
        topTag->setStyleSheet("color:white;font-weight:700;letter-spacing:1px;");
        topLayout->addWidget(topTag);
        root->addWidget(top);

        auto *body=new QWidget;
        auto *layout=new QVBoxLayout(body);
        layout->setContentsMargins(34,24,34,24); layout->setSpacing(18);

        auto *hero=new QFrame;
        hero->setStyleSheet("QFrame{background:#dcecff;border:0;border-radius:16px;}");
        auto *heroLayout=new QHBoxLayout(hero); heroLayout->setContentsMargins(26,18,22,18); heroLayout->setSpacing(18);
        auto *heroText=new QVBoxLayout; heroText->setSpacing(7);
        auto *welcome=new QLabel("INDIAN RAILWAYS");
        welcome->setStyleSheet("font-size:13px;font-weight:800;color:#1769aa;background:transparent;");
        auto *welcomeTitle=new QLabel("Book Train Tickets");
        welcomeTitle->setWordWrap(true);
        welcomeTitle->setStyleSheet("font-size:26px;font-weight:850;color:#17365d;background:transparent;");
        auto *welcomeSub=new QLabel("Travel across India with comfort and convenience.");
        welcomeSub->setWordWrap(true);
        welcomeSub->setStyleSheet("color:#526b8b;background:transparent;font-size:11pt;");
        heroText->addWidget(welcome); heroText->addWidget(welcomeTitle); heroText->addWidget(welcomeSub); heroText->addStretch();
        heroLayout->addLayout(heroText,1);
        auto *trainVisual=new QLabel("🚆\n━━━━━━━━━━━━━━━━━━━━\n🚉  ────────────────");
        trainVisual->setAlignment(Qt::AlignCenter);
        trainVisual->setMinimumWidth(250);
        trainVisual->setStyleSheet("font-size:28px;font-weight:800;color:#1769aa;background:#c4e3ff;border:1px solid #a6d2f5;border-radius:12px;padding:18px;");
        heroLayout->addWidget(trainVisual,0,Qt::AlignVCenter);
        auto *modiImage=new QLabel;
        modiImage->setFixedSize(94,120);
        modiImage->setAlignment(Qt::AlignCenter);
        modiImage->setStyleSheet("background:white;border:2px solid white;border-radius:12px;color:#64748b;font-size:9pt;");
        QPixmap modiPixmap("modi.jpg");
        if(!modiPixmap.isNull()) modiImage->setPixmap(modiPixmap.scaled(modiImage->size(),Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation));
        else modiImage->setText("Add modi.jpg");
        modiImage->setToolTip("Portrait source: Government of India / PMO, via Wikimedia Commons. This does not imply endorsement.");
        heroLayout->addWidget(modiImage,0,Qt::AlignVCenter);
        layout->addWidget(hero);

        auto *searchCard=card();
        auto *searchLayout=new QVBoxLayout(searchCard); searchLayout->setContentsMargins(22,18,22,20); searchLayout->setSpacing(12);
        auto *searchTitle=new QLabel("SEARCH TRAINS BY ROUTE OR TRAIN NUMBER");
        searchTitle->setStyleSheet("font-size:12px;font-weight:850;color:#17365d;letter-spacing:1px;");
        searchLayout->addWidget(searchTitle);
        auto *searchGrid=new QGridLayout; searchGrid->setHorizontalSpacing(12); searchGrid->setVerticalSpacing(7);
        searchGrid->addWidget(makeLabel("FROM","font-size:9pt;font-weight:700;color:#64748b;"),0,0);
        searchGrid->addWidget(makeLabel("TO","font-size:9pt;font-weight:700;color:#64748b;"),0,1);
        searchGrid->addWidget(makeLabel("JOURNEY DATE","font-size:9pt;font-weight:700;color:#64748b;"),0,2);
        searchGrid->addWidget(makeLabel("TRAIN NUMBER (ANY INDIAN TRAIN)","font-size:9pt;font-weight:700;color:#64748b;"),0,3);
        fromSearch=new QLineEdit; fromSearch->setPlaceholderText("e.g. Bhopal");
        toSearch=new QLineEdit; toSearch->setPlaceholderText("e.g. Delhi");
        journeyDate=new QDateEdit(QDate::currentDate()); journeyDate->setCalendarPopup(true); journeyDate->setDisplayFormat("dd MMM yyyy");
        trainNumberSearch=new QLineEdit; trainNumberSearch->setPlaceholderText("e.g. 12919");
        searchGrid->addWidget(fromSearch,1,0); searchGrid->addWidget(toSearch,1,1); searchGrid->addWidget(journeyDate,1,2);
        searchGrid->addWidget(trainNumberSearch,1,3);
        auto *searchBtn=makeButton("SEARCH ROUTE","accent");
        searchGrid->addWidget(searchBtn,1,4);
        searchGrid->setColumnStretch(0,1); searchGrid->setColumnStretch(1,1); searchGrid->setColumnStretch(2,1); searchGrid->setColumnStretch(3,1);
        searchLayout->addLayout(searchGrid);
        layout->addWidget(searchCard);
        connect(searchBtn,&QPushButton::clicked,this,[this]{quickSearch();});
        auto *numberBtn=makeButton("CHECK TRAIN STATUS","primary");
        searchGrid->addWidget(numberBtn,2,3,1,2);
        connect(numberBtn,&QPushButton::clicked,this,[this]{ QString number=trainNumberSearch->text().trimmed(); if(number.isEmpty() || number.size()<4 || number.size()>6 || !number.contains(QRegularExpression("^\\d+$"))){ QMessageBox::warning(this,"Train number required","Enter a valid Indian train number (4 to 6 digits)."); return; } checkLiveTrain(number); });

        auto *statsCard=card();
        auto *statsLayout=new QVBoxLayout(statsCard); statsLayout->setContentsMargins(20,14,20,14);
        auto *statsTitle=new QLabel("YOUR RAILWAY OVERVIEW");
        statsTitle->setStyleSheet("font-size:11px;font-weight:850;color:#64748b;letter-spacing:1px;");
        stats=new QLabel;
        stats->setStyleSheet("font-size:12px;font-weight:700;color:#17365d;padding:4px 0;");
        statsLayout->addWidget(statsTitle); statsLayout->addWidget(stats);
        layout->addWidget(statsCard);

        auto *actionsTitle=new QLabel("Explore Railway Services");
        actionsTitle->setStyleSheet("font-size:19px;font-weight:850;color:#17365d;padding-top:2px;");
        layout->addWidget(actionsTitle);

        auto *grid=new QGridLayout; grid->setSpacing(13);
        struct Action { QString title,desc,kind; std::function<void()> fn; };
        vector<Action> actions={
            {"🚉  Add Train","Add a route and seat capacity","primary",[this]{addTrain();}},
            {"👥  Register Passenger","Create a passenger profile","light",[this]{registerPassenger();}},
            {"🎟️  Book Ticket","Reserve a seat on a train","accent",[this]{book();}},
            {"📋  View Bookings","See confirmed and cancelled tickets","light",[this]{viewBookings();}},
            {"🔍  Search Trains","Browse trains and available seats","primary",[this]{searchTrain();}},
            {"🚆  Live Train Status","Live status with IST and auto-refresh","accent",[this]{
                bool ok=false;
                QString number=QInputDialog::getText(this,"Live Train Status","Enter train number (example: 12919):",QLineEdit::Normal,"",&ok).trimmed();
                if(!ok) return;
                if(number.isEmpty() || number.size()<4 || number.size()>6 || !number.contains(QRegularExpression("^\\d+$"))){QMessageBox::warning(this,"Invalid train number","Enter a valid 4 to 6 digit train number.");return;}
                checkLiveTrain(number);
            }},
            {"❌  Cancel Ticket","Cancel an active reservation","light",[this]{cancel();}},
            {"👥  Passengers","View registered passengers","light",[this]{viewPassengers();}},
            {"⏳  Waiting List","Check passengers waiting for seats","light",[this]{viewWaiting();}},
            {"📊  Reports","Review seat occupancy and daily totals","primary",[this]{reports();}},
        };
        for(int i=0;i<(int)actions.size();++i){
            auto *tile=card();
            auto *tileLayout=new QVBoxLayout(tile); tileLayout->setContentsMargins(15,14,15,14); tileLayout->setSpacing(7);
            auto *title=makeLabel(actions[i].title,"font-size:13pt;font-weight:800;color:#17365d;background:transparent;");
            auto *desc=makeLabel(actions[i].desc,"font-size:9pt;color:#718096;background:transparent;");
            auto *btn=makeButton("Open",actions[i].kind);
            tileLayout->addWidget(title); tileLayout->addWidget(desc); tileLayout->addStretch(); tileLayout->addWidget(btn);
            grid->addWidget(tile,i/3,i%3);
            connect(btn,&QPushButton::clicked,this,actions[i].fn);
        }
        layout->addLayout(grid);

        auto *footer=makeLabel("🚆 INDIAN RAILWAYS  •  Your data remains in the existing .txt files","font-size:9pt;color:#8190a5;padding:8px;");
        footer->setAlignment(Qt::AlignCenter); layout->addWidget(footer);

        auto *scroll=new QScrollArea; scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame); scroll->setWidget(body);
        root->addWidget(scroll);
        refresh();
    }
};

int main(int argc,char *argv[]){
    QApplication app(argc,argv);
    app.setApplicationName("Railway Reservation System");

    QDialog login;
    login.setWindowTitle("Login | Railway Reservation");
    login.setFixedSize(430,390);
    login.setStyleSheet(
        "QDialog{background:#f4f7fb;color:#172b4d;font-family:'Segoe UI';}"
        "QLabel{color:#17365d;}"
        "QLineEdit{background:white;border:1px solid #d7e0ed;border-radius:9px;padding:12px;font-size:11pt;}"
        "QPushButton{background:#1769aa;color:white;border:0;border-radius:9px;padding:12px;font-weight:800;}"
        "QPushButton:hover{background:#10558c;}");
    auto *loginLayout=new QVBoxLayout(&login);
    loginLayout->setContentsMargins(30,26,30,26);
    loginLayout->setSpacing(12);
    auto *logo=new QLabel("RAILWAY");
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet("font-size:27px;font-weight:900;letter-spacing:3px;color:#17365d;");
    auto *tagline=new QLabel("RESERVATION SYSTEM");
    tagline->setAlignment(Qt::AlignCenter);
    tagline->setStyleSheet("font-size:10px;font-weight:800;letter-spacing:2px;color:#1769aa;");
    auto *welcome=new QLabel("Welcome back, Lnctian");
    welcome->setAlignment(Qt::AlignCenter);
    welcome->setStyleSheet("font-size:17px;font-weight:800;padding-top:6px;");
    auto *hint=new QLabel("Sign in to manage your railway dashboard");
    hint->setAlignment(Qt::AlignCenter);
    hint->setStyleSheet("color:#718096;font-size:10pt;");
    auto *username=new QLineEdit;
    username->setPlaceholderText("Username");
    username->setText("Ajmal");
    auto *password=new QLineEdit;
    password->setPlaceholderText("Password");
    password->setEchoMode(QLineEdit::Password);
    password->setText("");
    auto *loginButton=new QPushButton("SIGN IN");
    auto *loginHint=new QLabel("Login: Ajmal  |  Password: 1234");
    loginHint->setAlignment(Qt::AlignCenter);
    loginHint->setStyleSheet("font-size:9pt;color:#718096;");
    loginLayout->addWidget(logo);
    loginLayout->addWidget(tagline);
    loginLayout->addSpacing(4);
    loginLayout->addWidget(welcome);
    loginLayout->addWidget(hint);
    loginLayout->addWidget(username);
    loginLayout->addWidget(password);
    loginLayout->addWidget(loginButton);
    loginLayout->addWidget(loginHint);
    QObject::connect(loginButton,&QPushButton::clicked,&login,[&](){
        if(username->text().trimmed().compare("Ajmal",Qt::CaseInsensitive)==0 && password->text()=="1234")
            login.accept();
        else
            QMessageBox::warning(&login,"Login failed","Username or password is incorrect.");
    });
    QObject::connect(password,&QLineEdit::returnPressed,loginButton,&QPushButton::click);
    if(login.exec()!=QDialog::Accepted) return 0;

    loadData();
    Window w;
    w.show();
    return app.exec();
}
