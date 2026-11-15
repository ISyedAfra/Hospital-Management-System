#include "hospital.h"
#include <iostream>
using namespace std;
string ask(const string &p)
{
    cout << p;
    string s;
    getline(cin, s);
    return s;
}
int askInt(const string &p)
{
    while (true)
    {
        try
        {
            string s = ask(p);
            size_t n;
            int x = stoi(s, &n);
            if (n == s.size())
                return x;
        }
        catch (...)
        {
        }
        cout << "Enter a valid number.\n";
    }
}
void patients(const HospitalSystem &h)
{
    for (const auto p : h.getPatients())
        cout << p.id << " " << p.name << " " << p.age << " " << p.gender << " " << p.phone << "\n";
}
void doctors(const HospitalSystem &h)
{
    for (const auto d : h.getDoctors())
        cout << d.id << " " << d.name << " " << d.specialization << " " << d.phone << "\n";
}
void appointments(const HospitalSystem &h)
{
    for (const auto a : h.getAppointments())
        cout << a.id << " " << a.patientName << " " << a.doctorName << " " << a.dateTime << " " << a.status << "\n";
}
void addPatient(HospitalSystem &h)
{
    string n = ask("Patient name: ");
    int a = askInt("Age: ");
    string g = ask("Gender: "), p = ask("Phone: ");
    cout << (h.addPatient(n, a, g, p) ? "Patient added.\n" : "Invalid details.\n");
}
void addDoctor(HospitalSystem &h)
{
    string n = ask("Doctor name: "), s = ask("Specialization: "), p = ask("Phone: ");
    cout << (h.addDoctor(n, s, p) ? "Doctor added.\n" : "Could not add doctor.\n");
}
void newAppointment(HospitalSystem &h)
{
    int p = askInt("Patient ID: "), d = askInt("Doctor ID: ");
    string date = ask("Date/time: ");
    cout << (h.addAppointment(p, d, date) ? "Appointment created.\n" : "Could not create appointment.\n");
}
void admin(HospitalSystem &h)
{
    while (true)
    {
        cout << "\nADMIN\n1 Add staff\n2 List staff\n3 Add patient\n4 List patients\n5 Update patient\n6 Delete patient\n7 Add doctor\n8 List doctors\n9 Create appointment\n10 List appointments\n11 Approve/Reject appointment\n0 Logout\n";
        int c = askInt("Choice: ");
        if (c == 0)
            return;
        if (c == 1)
        {
            string u = ask("Username: "), p = ask("Password (8+ chars): "), r = ask("Role (receptionist/approver): ");
            cout << (h.addStaff(u, p, r) ? "Staff added.\n" : "Could not add staff.\n");
        }
        else if (c == 2)
        {
            for (const auto u : h.getUsers())
                cout << u.id << " " << u.username << " " << u.role << "\n";
        }
        else if (c == 3)
            addPatient(h);
        else if (c == 4)
            patients(h);
        else if (c == 5)
        {
            int id = askInt("Patient ID: "), a;
            string n = ask("New name: ");
            a = askInt("New age: ");
            string g = ask("Gender: "), p = ask("Phone: ");
            cout << (h.updatePatient(id, n, a, g, p) ? "Updated.\n" : "Update failed.\n");
        }
        else if (c == 6)
        {
            int id = askInt("Patient ID: ");
            cout << (h.deletePatient(id) ? "Deleted.\n" : "Delete failed (not found or linked appointment).\n");
        }
        else if (c == 7)
            addDoctor(h);
        else if (c == 8)
            doctors(h);
        else if (c == 9)
            newAppointment(h);
        else if (c == 10)
            appointments(h);
        else if (c == 11)
        {
            int id = askInt("Appointment ID: ");
            string s = ask("Status (Approved/Rejected): ");
            cout << (h.updateAppointmentStatus(id, s) ? "Status updated.\n" : "Invalid ID/status.\n");
        }
        else
            cout << "Invalid choice.\n";
    }
}
void receptionist(HospitalSystem &h)
{
    while (true)
    {
        cout << "\nRECEPTIONIST\n1 Add patient\n2 List patients\n3 List doctors\n4 Create appointment\n5 List appointments\n0 Logout\n";
        int c = askInt("Choice: ");
        if (c == 0)
            return;
        if (c == 1)
            addPatient(h);
        else if (c == 2)
            patients(h);
        else if (c == 3)
            doctors(h);
        else if (c == 4)
            newAppointment(h);
        else if (c == 5)
            appointments(h);
        else
            cout << "Invalid choice.\n";
    }
}
void approver(HospitalSystem &h)
{
    while (true)
    {
        cout << "\nAPPROVER\n1 List appointments\n2 Approve/Reject\n0 Logout\n";
        int c = askInt("Choice: ");
        if (c == 0)
            return;
        if (c == 1)
            appointments(h);
        else if (c == 2)
        {
            int id = askInt("Appointment ID: ");
            string s = ask("Status (Approved/Rejected): ");
            cout << (h.updateAppointmentStatus(id, s) ? "Updated.\n" : "Invalid ID/status.\n");
        }
        else
            cout << "Invalid choice.\n";
    }
}
int main()
{
    HospitalSystem h;
    if (!h.isOpen() || !h.initialize())
    {
        cerr << "Database initialization failed.\n";
        return 1;
    }
    if (!h.hasUsers())
    {
        cout << "First-time admin setup\n";
        string u = ask("Admin username: "), p = ask("Admin password (8+ chars): ");
        if (!h.createFirstAdmin(u, p))
        {
            cout << "Admin setup failed.\n";
            return 1;
        }
    }
    while (true)
    {
        cout << "\nHOSPITAL MANAGEMENT SYSTEM LOGIN\n";
        string u = ask("Username: "), p = ask("Password: ");
        User user;
        if (!h.authenticate(u, p, user))
        {
            cout << "Invalid login.\n";
            string r = ask("Try again? (y/n): ");
            if (r != "y" && r != "Y")
                break;
            continue;
        }
        cout << "Welcome " << user.username << " (" << user.role << ")\n";
        if (user.role == "admin")
            admin(h);
        else if (user.role == "receptionist")
            receptionist(h);
        else
            approver(h);
    }
    cout << "Data saved in hospital.db\n";
    return 0;
}
