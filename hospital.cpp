#include "hospital.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iomanip>
#include <sstream>

namespace
{
    constexpr int SALT = 16, HASH = 32, ITERATIONS = 200000;
    std::string hex(const unsigned char *p, int n)
    {
        std::ostringstream s;
        s << std::hex << std::setfill('0');
        for (int i = 0; i < n; i++)
            s << std::setw(2) << (int)p[i];
        return s.str();
    }
    bool unhex(const std::string &s, unsigned char *p, int n)
    {
        if (s.size() != static_cast<size_t>(2 * n))
            return false;
        try
        {
            for (int i = 0; i < n; i++)
                p[i] = (unsigned char)std::stoul(s.substr(2 * i, 2), nullptr, 16);
        }
        catch (...)
        {
            return false;
        }
        return true;
    }
    bool hashPassword(const std::string &pass, std::string &salt, std::string &digest)
    {
        unsigned char s[SALT], h[HASH];
        if (RAND_bytes(s, SALT) != 1 || PKCS5_PBKDF2_HMAC(pass.c_str(), (int)pass.size(),
                                                          s, SALT, ITERATIONS, EVP_sha256(), HASH, h) != 1)
            return false;
        salt = hex(s, SALT);
        digest = hex(h, HASH);
        return true;
    }
    bool verify(const std::string &pass, const std::string &salt, const std::string &digest)
    {
        unsigned char s[SALT], expected[HASH], actual[HASH];
        if (!unhex(salt, s, SALT) || !unhex(digest, expected, HASH))
            return false;
        if (PKCS5_PBKDF2_HMAC(pass.c_str(), (int)pass.size(), s, SALT, ITERATIONS,
                              EVP_sha256(), HASH, actual) != 1)
            return false;
        unsigned char diff = 0;
        for (int i = 0; i < HASH; i++)
            diff |= actual[i] ^ expected[i];
        return diff == 0;
    }
    bool prep(sqlite3 *db, const char *sql, sqlite3_stmt **st)
    {
        return sqlite3_prepare_v2(db, sql, -1, st, nullptr) == SQLITE_OK;
    }
    void text(sqlite3_stmt *st, int i, const std::string &v)
    {
        sqlite3_bind_text(st, i, v.c_str(), -1, SQLITE_TRANSIENT);
    }
    std::string col(sqlite3_stmt *st, int i)
    {
        const unsigned char *p = sqlite3_column_text(st, i);
        return p ? reinterpret_cast<const char *>(p) : "";
    }
}
HospitalSystem::HospitalSystem(const std::string &path)
{
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK)
    {
        if (db_)
            sqlite3_close(db_);
        db_ = nullptr;
    }
    else
        sqlite3_exec(db_, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);
}
HospitalSystem::~HospitalSystem()
{
    if (db_)
        sqlite3_close(db_);
}
bool HospitalSystem::isOpen() const { return db_ != nullptr; }
bool HospitalSystem::initialize() { return db_ && createTables(); }
bool HospitalSystem::createTables()
{
    const char *sql = R"(
 CREATE TABLE IF NOT EXISTS users(id INTEGER PRIMARY KEY,username TEXT UNIQUE NOT NULL,
 salt TEXT NOT NULL,password_hash TEXT NOT NULL,
 role TEXT NOT NULL CHECK(role IN ('admin','receptionist','approver')));
 CREATE TABLE IF NOT EXISTS patients(id INTEGER PRIMARY KEY,name TEXT NOT NULL,
 age INTEGER NOT NULL CHECK(age BETWEEN 0 AND 120),gender TEXT NOT NULL,phone TEXT NOT NULL);
 CREATE TABLE IF NOT EXISTS doctors(id INTEGER PRIMARY KEY,name TEXT NOT NULL,
 specialization TEXT NOT NULL,phone TEXT NOT NULL);
 CREATE TABLE IF NOT EXISTS appointments(id INTEGER PRIMARY KEY,patient_id INTEGER NOT NULL,
 doctor_id INTEGER NOT NULL,appointment_date TEXT NOT NULL,status TEXT NOT NULL DEFAULT 'Pending'
 CHECK(status IN ('Pending','Approved','Rejected')),
 FOREIGN KEY(patient_id) REFERENCES patients(id),FOREIGN KEY(doctor_id) REFERENCES doctors(id));
 )";
    return sqlite3_exec(db_, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
}
bool HospitalSystem::hasUsers() const
{
    sqlite3_stmt *s = nullptr;
    bool yes = false;
    if (db_ && prep(db_, "SELECT COUNT(*) FROM users", &s) && sqlite3_step(s) == SQLITE_ROW)
        yes = sqlite3_column_int(s, 0) > 0;
    sqlite3_finalize(s);
    return yes;
}
bool HospitalSystem::createFirstAdmin(const std::string &u, const std::string &p)
{
    if (!db_ || hasUsers() || u.empty() || p.size() < 8)
        return false;
    std::string salt, digest;
    if (!hashPassword(p, salt, digest))
        return false;
    sqlite3_stmt *s = nullptr;
    if (!prep(db_, "INSERT INTO users(username,salt,password_hash,role) VALUES(?,?,?,'admin')", &s))
        return false;
    text(s, 1, u);
    text(s, 2, salt);
    text(s, 3, digest);
    bool ok = sqlite3_step(s) == SQLITE_DONE;
    sqlite3_finalize(s);
    return ok;
}
bool HospitalSystem::addStaff(const std::string &u, const std::string &p, const std::string &role)
{
    if (!db_ || u.empty() || p.size() < 8 || (role != "receptionist" && role != "approver"))
        return false;
    std::string salt, digest;
    if (!hashPassword(p, salt, digest))
        return false;
    sqlite3_stmt *s = nullptr;
    if (!prep(db_, "INSERT INTO users(username,salt,password_hash,role) VALUES(?,?,?,?)", &s))
        return false;
    text(s, 1, u);
    text(s, 2, salt);
    text(s, 3, digest);
    text(s, 4, role);
    bool ok = sqlite3_step(s) == SQLITE_DONE;
    sqlite3_finalize(s);
    return ok;
}
bool HospitalSystem::authenticate(const std::string &u, const std::string &p, User &out) const
{
    sqlite3_stmt *s = nullptr;
    if (!db_ || !prep(db_, "SELECT id,username,salt,password_hash,role FROM users WHERE username=?", &s))
        return false;
    text(s, 1, u);
    bool ok = false;
    if (sqlite3_step(s) == SQLITE_ROW && verify(p, col(s, 2), col(s, 3)))
    {
        out.id = sqlite3_column_int(s, 0);
        out.username = col(s, 1);
        out.role = col(s, 4);
        ok = true;
    }
    sqlite3_finalize(s);
    return ok;
}
std::vector<User> HospitalSystem::getUsers() const
{
    std::vector<User> v;
    sqlite3_stmt *s = nullptr;
    if (db_ && prep(db_, "SELECT id,username,role FROM users ORDER BY id", &s))
        while (sqlite3_step(s) == SQLITE_ROW)
            v.push_back({sqlite3_column_int(s, 0), col(s, 1), col(s, 2)});
    sqlite3_finalize(s);
    return v;
}
bool HospitalSystem::addPatient(const std::string &n, int a, const std::string &g, const std::string &p)
{
    if (!db_ || n.empty() || a < 0 || a > 120 || g.empty() || p.empty())
        return false;
    sqlite3_stmt *s = nullptr;
    if (!prep(db_, "INSERT INTO patients(name,age,gender,phone) VALUES(?,?,?,?)", &s))
        return false;
    text(s, 1, n);
    sqlite3_bind_int(s, 2, a);
    text(s, 3, g);
    text(s, 4, p);
    bool ok = sqlite3_step(s) == SQLITE_DONE;
    sqlite3_finalize(s);
    return ok;
}
std::vector<Patient> HospitalSystem::getPatients() const
{
    std::vector<Patient> v;
    sqlite3_stmt *s = nullptr;
    if (db_ && prep(db_, "SELECT id,name,age,gender,phone FROM patients ORDER BY id", &s))
        while (sqlite3_step(s) == SQLITE_ROW)
            v.push_back({sqlite3_column_int(s, 0), sqlite3_column_int(s, 2), col(s, 1), col(s, 3), col(s, 4)});
    sqlite3_finalize(s);
    return v;
}
bool HospitalSystem::updatePatient(int id, const std::string &n, int a, const std::string &g, const std::string &p)
{
    if (!db_ || id <= 0 || n.empty() || a < 0 || a > 120 || g.empty() || p.empty())
        return false;
    sqlite3_stmt *s = nullptr;
    if (!prep(db_, "UPDATE patients SET name=?,age=?,gender=?,phone=? WHERE id=?", &s))
        return false;
    text(s, 1, n);
    sqlite3_bind_int(s, 2, a);
    text(s, 3, g);
    text(s, 4, p);
    sqlite3_bind_int(s, 5, id);
    bool ok = sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db_) > 0;
    sqlite3_finalize(s);
    return ok;
}
bool HospitalSystem::deletePatient(int id)
{
    sqlite3_stmt *s = nullptr;
    if (!db_ || id <= 0 || !prep(db_, "DELETE FROM patients WHERE id=?", &s))
        return false;
    sqlite3_bind_int(s, 1, id);
    bool ok = sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db_) > 0;
    sqlite3_finalize(s);
    return ok;
}
bool HospitalSystem::addDoctor(const std::string &n, const std::string &sp, const std::string &p)
{
    if (!db_ || n.empty() || sp.empty() || p.empty())
        return false;
    sqlite3_stmt *s = nullptr;
    if (!prep(db_, "INSERT INTO doctors(name,specialization,phone) VALUES(?,?,?)", &s))
        return false;
    text(s, 1, n);
    text(s, 2, sp);
    text(s, 3, p);
    bool ok = sqlite3_step(s) == SQLITE_DONE;
    sqlite3_finalize(s);
    return ok;
}
std::vector<Doctor> HospitalSystem::getDoctors() const
{
    std::vector<Doctor> v;
    sqlite3_stmt *s = nullptr;
    if (db_ && prep(db_, "SELECT id,name,specialization,phone FROM doctors ORDER BY id", &s))
        while (sqlite3_step(s) == SQLITE_ROW)
            v.push_back({sqlite3_column_int(s, 0), col(s, 1), col(s, 2), col(s, 3)});
    sqlite3_finalize(s);
    return v;
}
bool HospitalSystem::addAppointment(int pid, int did, const std::string &date)
{
    if (!db_ || pid <= 0 || did <= 0 || date.empty())
        return false;
    sqlite3_stmt *s = nullptr;
    if (!prep(db_, "INSERT INTO appointments(patient_id,doctor_id,appointment_date) VALUES(?,?,?)", &s))
        return false;
    sqlite3_bind_int(s, 1, pid);
    sqlite3_bind_int(s, 2, did);
    text(s, 3, date);
    bool ok = sqlite3_step(s) == SQLITE_DONE;
    sqlite3_finalize(s);
    return ok;
}
std::vector<Appointment> HospitalSystem::getAppointments() const
{
    std::vector<Appointment> v;
    sqlite3_stmt *s = nullptr;
    const char *q = "SELECT a.id,a.patient_id,a.doctor_id,p.name,d.name,a.appointment_date,a.status FROM appointments a JOIN patients p ON p.id=a.patient_id JOIN doctors d ON d.id=a.doctor_id ORDER BY a.id";
    if (db_ && prep(db_, q, &s))
        while (sqlite3_step(s) == SQLITE_ROW)
            v.push_back({sqlite3_column_int(s, 0), sqlite3_column_int(s, 1), sqlite3_column_int(s, 2), col(s, 3), col(s, 4), col(s, 5), col(s, 6)});
    sqlite3_finalize(s);
    return v;
}
bool HospitalSystem::updateAppointmentStatus(int id, const std::string &status)
{
    if (!db_ || id <= 0 || (status != "Approved" && status != "Rejected"))
        return false;
    sqlite3_stmt *s = nullptr;
    if (!prep(db_, "UPDATE appointments SET status=? WHERE id=?", &s))
        return false;
    text(s, 1, status);
    sqlite3_bind_int(s, 2, id);
    bool ok = sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db_) > 0;
    sqlite3_finalize(s);
    return ok;
}
