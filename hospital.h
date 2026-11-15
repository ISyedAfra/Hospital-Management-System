#ifndef HOSPITAL_H
#define HOSPITAL_H
#include <string>
#include <vector>
#include <sqlite3.h>

struct User { int id{}; std::string username, role; };
struct Patient { int id{}, age{}; std::string name, gender, phone; };
struct Doctor { int id{}; std::string name, specialization, phone; };
struct Appointment {
    int id{}, patientId{}, doctorId{};
    std::string patientName, doctorName, dateTime, status;
};

class HospitalSystem {
    sqlite3* db_{nullptr};
    bool createTables();
public:
    explicit HospitalSystem(const std::string& path = "hospital.db");
    ~HospitalSystem();
    HospitalSystem(const HospitalSystem&) = delete;
    HospitalSystem& operator=(const HospitalSystem&) = delete;
    bool isOpen() const;
    bool initialize();
    bool hasUsers() const;
    bool createFirstAdmin(const std::string&, const std::string&);
    bool addStaff(const std::string&, const std::string&, const std::string&);
    bool authenticate(const std::string&, const std::string&, User&) const;
    std::vector<User> getUsers() const;
    bool addPatient(const std::string&, int, const std::string&, const std::string&);
    std::vector<Patient> getPatients() const;
    bool updatePatient(int, const std::string&, int, const std::string&, const std::string&);
    bool deletePatient(int);
    bool addDoctor(const std::string&, const std::string&, const std::string&);
    std::vector<Doctor> getDoctors() const;
    bool addAppointment(int, int, const std::string&);
    std::vector<Appointment> getAppointments() const;
    bool updateAppointmentStatus(int, const std::string&);
};
#endif
