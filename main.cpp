#include <iostream>
#include <vector>
#include <list>
#include <queue>
#include <fstream>
#include <iomanip>
#include <string>
#include <limits>
#include <ctime>
using namespace std;

class Train {
public:
int number, totalSeats, availableSeats;
string name, source, destination;
};

class Passenger {
public:
int id, age;
string name, gender;
};

class Booking {
public:
int id, passengerId, trainNumber;
string date;
bool active;
};

class Waiting {
public:
int passengerId, trainNumber;
};

vector<Train> trains;
list<Passenger> passengers;
vector<Booking> bookings;
queue<Waiting> waitingList;

int nextPassengerId = 1;
int nextBookingId = 1;

void saveData() {
ofstream f("trains.txt");
for (Train t : trains)
f << t.number << ' ' << quoted(t.name) << ' '
<< quoted(t.source) << ' ' << quoted(t.destination) << ' '
<< t.totalSeats << ' ' << t.availableSeats << '\n';
f.close();
ofstream p("passengers.txt");
for (Passenger x : passengers)
    p << x.id << ' ' << x.age << ' '
      << quoted(x.name) << ' ' << quoted(x.gender) << '\n';
p.close();

ofstream b("bookings.txt");
for (Booking x : bookings)
    b << x.id << ' ' << x.passengerId << ' '
      << x.trainNumber << ' ' << quoted(x.date) << ' '
      << x.active << '\n';
b.close();

ofstream w("waiting.txt");
queue<Waiting> q = waitingList;
while (!q.empty()) {
    Waiting x = q.front();
    q.pop();
    w << x.passengerId << ' ' << x.trainNumber << '\n';
}
w.close();


}

void loadData() {
ifstream f("trains.txt");
Train t;
while (f >> t.number >> quoted(t.name)
>> quoted(t.source) >> quoted(t.destination)
>> t.totalSeats >> t.availableSeats)
trains.push_back(t);
f.close();

ifstream p("passengers.txt");
Passenger x;
while (p >> x.id >> x.age >> quoted(x.name) >> quoted(x.gender)) {
    passengers.push_back(x);
    if (x.id >= nextPassengerId)
        nextPassengerId = x.id + 1;
}
p.close();

ifstream b("bookings.txt");
Booking y;
while (b >> y.id >> y.passengerId >> y.trainNumber
         >> quoted(y.date) >> y.active) {
    bookings.push_back(y);
    if (y.id >= nextBookingId)
        nextBookingId = y.id + 1;
}
b.close();

ifstream w("waiting.txt");
Waiting z;
while (w >> z.passengerId >> z.trainNumber)
    waitingList.push(z);
w.close();


}

int findTrain(int number) {
for (int i = 0; i < (int)trains.size(); i++)
if (trains[i].number == number)
return i;
return -1;
}

bool passengerExists(int id) {
for (Passenger p : passengers)
if (p.id == id)
return true;
return false;
}

string currentDate() {
time_t now = time(nullptr);
tm *local = localtime(&now);
char date[11];
strftime(date, sizeof(date), "%Y-%m-%d", local);
return date;
}

void addTrain() {
Train t;
cout << "Train Number: ";
cin >> t.number;

if (findTrain(t.number) != -1) {
    cout << "Train already exists!\n";
    return;
}

cin.ignore(numeric_limits<streamsize>::max(), '\n');

cout << "Train Name: ";
getline(cin, t.name);

cout << "Source: ";
getline(cin, t.source);

cout << "Destination: ";
getline(cin, t.destination);

cout << "Total Seats: ";
cin >> t.totalSeats;

if (t.number <= 0 || t.totalSeats <= 0) {
    cout << "Invalid train number or seat count!\n";
    return;
}

t.availableSeats = t.totalSeats;
trains.push_back(t);
saveData();

cout << "Train added successfully!\n";


}

void searchTrain() {
int number;
cout << "Enter Train Number: ";
cin >> number;


int i = findTrain(number);

if (i == -1) {
    cout << "Train not found!\n";
    return;
}

Train t = trains[i];

cout << "\nTrain Number: " << t.number
     << "\nTrain Name: " << t.name
     << "\nSource: " << t.source
     << "\nDestination: " << t.destination
     << "\nTotal Seats: " << t.totalSeats
     << "\nAvailable Seats: " << t.availableSeats << '\n';


}

void checkSeats() {
int number;
cout << "Enter Train Number: ";
cin >> number;


int i = findTrain(number);

if (i == -1) {
    cout << "Train not found!\n";
    return;
}

cout << "Available Seats: " << trains[i].availableSeats << '\n';


}

void registerPassenger() {
Passenger p;
p.id = nextPassengerId++;


cin.ignore(numeric_limits<streamsize>::max(), '\n');

cout << "Passenger Name: ";
getline(cin, p.name);

cout << "Age: ";
cin >> p.age;

if (p.age <= 0 || p.age > 120) {
    cout << "Invalid age!\n";
    nextPassengerId--;
    return;
}

cin.ignore(numeric_limits<streamsize>::max(), '\n');

cout << "Gender: ";
getline(cin, p.gender);

passengers.push_back(p);
saveData();

cout << "Passenger registered successfully!\n";
cout << "Passenger ID: " << p.id << '\n';


}

void viewPassengers() {
if (passengers.empty()) {
cout << "No passengers registered!\n";
return;
}


for (Passenger p : passengers)
    cout << "\nID: " << p.id
         << "\nName: " << p.name
         << "\nAge: " << p.age
         << "\nGender: " << p.gender << '\n';


}

void bookTicket() {
int passengerId, trainNumber;


cout << "Passenger ID: ";
cin >> passengerId;

if (!passengerExists(passengerId)) {
    cout << "Passenger not found!\n";
    return;
}

cout << "Train Number: ";
cin >> trainNumber;

int i = findTrain(trainNumber);

if (i == -1) {
    cout << "Train not found!\n";
    return;
}

if (trains[i].availableSeats == 0) {
    Waiting w;
    w.passengerId = passengerId;
    w.trainNumber = trainNumber;
    waitingList.push(w);
    saveData();
    cout << "No seats available. Added to waiting list.\n";
    return;
}

Booking b;
b.id = nextBookingId++;
b.passengerId = passengerId;
b.trainNumber = trainNumber;
b.date = currentDate();
b.active = true;

bookings.push_back(b);
trains[i].availableSeats--;

saveData();

cout << "\nTicket booked successfully!"
     << "\nBooking ID: " << b.id
     << "\nPassenger ID: " << b.passengerId
     << "\nTrain Number: " << b.trainNumber
 << "\nDate: " << b.date << '\n';
}

void cancelTicket() {
int id;
cout << "Booking ID: ";
cin >> id;


for (Booking &b : bookings) {
    if (b.id == id && b.active) {
        b.active = false;

        int i = findTrain(b.trainNumber);

        if (i != -1) {
            queue<Waiting> remaining;
            bool assigned = false;

            while (!waitingList.empty()) {
                Waiting w = waitingList.front();
                waitingList.pop();

                if (w.trainNumber == b.trainNumber && !assigned) {
                    Booking newBooking;
                    newBooking.id = nextBookingId++;
                    newBooking.passengerId = w.passengerId;
                    newBooking.trainNumber = w.trainNumber;
                    newBooking.date = currentDate();
                    newBooking.active = true;

                    bookings.push_back(newBooking);
                    assigned = true;

                    cout << "Seat assigned to waiting passenger "
                         << w.passengerId << ". New Booking ID: "
                         << newBooking.id << '\n';
                } else {
                    remaining.push(w);
                }
            }

            waitingList = remaining;

            if (!assigned)
                trains[i].availableSeats++;
        }

        saveData();
        cout << "Ticket cancelled successfully!\n";
        return;
    }
}

cout << "Active booking not found!\n";


}

void viewBookings() {
if (bookings.empty()) {
cout << "No bookings found!\n";
return;
}


for (Booking b : bookings)
    cout << "\nBooking ID: " << b.id
         << "\nPassenger ID: " << b.passengerId
         << "\nTrain Number: " << b.trainNumber
         << "\nDate: " << b.date
         << "\nStatus: " << (b.active ? "Confirmed" : "Cancelled")
         << '\n';


}

void viewWaitingList() {
if (waitingList.empty()) {
cout << "Waiting list is empty!\n";
return;
}


queue<Waiting> q = waitingList;

cout << "\nWaiting List\n";

while (!q.empty()) {
    Waiting w = q.front();
    q.pop();

    cout << "Passenger ID: " << w.passengerId
         << " | Train Number: " << w.trainNumber << '\n';
}


}

void reports() {
int choice;

cout << "\n1. Daily Reservations\n";
cout << "2. Waiting List Report\n";
cout << "3. Seat Occupancy Report\n";
cout << "Enter choice: ";
cin >> choice;

if (choice == 1) {
    string date;
    cout << "Enter Date (YYYY-MM-DD): ";
    cin >> date;

    int count = 0;

    for (Booking b : bookings)
        if (b.date == date && b.active)
            count++;

    cout << "Confirmed Reservations on " << date
         << ": " << count << '\n';
} else if (choice == 2) {
    viewWaitingList();
} else if (choice == 3) {
    if (trains.empty()) {
        cout << "No trains available!\n";
        return;
    }

    for (Train t : trains) {
        int occupied = t.totalSeats - t.availableSeats;
        double percentage = 100.0 * occupied / t.totalSeats;

        cout << "\nTrain: " << t.name
             << "\nTrain Number: " << t.number
             << "\nOccupied Seats: " << occupied
             << "\nAvailable Seats: " << t.availableSeats
             << "\nOccupancy: " << fixed << setprecision(1)
             << percentage << "%\n";
    }
} else {
    cout << "Invalid choice!\n";
}


}

int main() {
loadData();


int choice;

do {
    cout << "\n===== Railway Reservation System =====\n";
    cout << "1. Add Train\n";
    cout << "2. Search Train\n";
    cout << "3. Check Seat Availability\n";
    cout << "4. Register Passenger\n";
    cout << "5. View Passengers\n";
    cout << "6. Book Ticket\n";
    cout << "7. Cancel Ticket\n";
    cout << "8. Booking History\n";
    cout << "9. Waiting List\n";
    cout << "10. Reports\n";
    cout << "0. Exit\n";
    cout << "Enter choice: ";

    if (!(cin >> choice)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input!\n";
        continue;
    }

    switch (choice) {
        case 1: addTrain(); break;
        case 2: searchTrain(); break;
        case 3: checkSeats(); break;
        case 4: registerPassenger(); break;
        case 5: viewPassengers(); break;
        case 6: bookTicket(); break;
        case 7: cancelTicket(); break;
        case 8: viewBookings(); break;
        case 9: viewWaitingList(); break;
        case 10: reports(); break;
        case 0: saveData(); cout << "Exiting...\n"; break;
        default: cout << "Invalid choice!\n";
    }
} while (choice != 0);

return 0;

}
