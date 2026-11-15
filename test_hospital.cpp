#include "hospital.h"

#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include <chrono>

namespace
{
std::string makeDatabasePath()
{
    static unsigned long counter = 0;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return (std::filesystem::temp_directory_path() /
            ("hospital_gtest_" + std::to_string(stamp) + "_" +
             std::to_string(++counter) + ".db")).string();
}

class HospitalTest : public ::testing::Test
{
protected:
    std::string dbPath;
    HospitalSystem *hospital = nullptr;

    void SetUp() override
    {
        dbPath = makeDatabasePath();
        hospital = new HospitalSystem(dbPath);
        ASSERT_TRUE(hospital->isOpen());
        ASSERT_TRUE(hospital->initialize());
    }

    void TearDown() override
    {
        delete hospital;
        hospital = nullptr;

        std::error_code ec;
        std::filesystem::remove(dbPath, ec);
        std::filesystem::remove(dbPath + "-journal", ec);
        std::filesystem::remove(dbPath + "-wal", ec);
        std::filesystem::remove(dbPath + "-shm", ec);
    }

    void createBasicRecords()
    {
        ASSERT_TRUE(hospital->addPatient("Asha", 25, "Female", "9876543210"));
        ASSERT_TRUE(hospital->addDoctor("Dr Kumar", "Cardiology", "9123456780"));
    }

    int createAppointment()
    {
        createBasicRecords();
        EXPECT_TRUE(hospital->addAppointment(1, 1, "2026-12-01 10:30"));
        const auto appointments = hospital->getAppointments();
        EXPECT_EQ(appointments.size(), 1u);
        return appointments.empty() ? -1 : appointments[0].id;
    }
};

// Database and user-management tests

TEST_F(HospitalTest, DatabaseOpensAndInitializes)
{
    EXPECT_TRUE(hospital->isOpen());
    EXPECT_TRUE(hospital->initialize());
}

TEST_F(HospitalTest, InvalidDatabasePathDoesNotOpen)
{
    const std::string badPath =
        (std::filesystem::temp_directory_path() /
         "hospital_gtest_missing_directory" / "hospital.db").string();

    HospitalSystem invalid(badPath);
    EXPECT_FALSE(invalid.isOpen());
    EXPECT_FALSE(invalid.initialize());
    EXPECT_FALSE(invalid.hasUsers());
}

TEST_F(HospitalTest, InitiallyThereAreNoUsers)
{
    EXPECT_FALSE(hospital->hasUsers());
    EXPECT_TRUE(hospital->getUsers().empty());
}

TEST_F(HospitalTest, CreatesFirstAdminSuccessfully)
{
    EXPECT_TRUE(hospital->createFirstAdmin("admin", "Admin@123"));
    EXPECT_TRUE(hospital->hasUsers());

    const auto users = hospital->getUsers();
    ASSERT_EQ(users.size(), 1u);
    EXPECT_EQ(users[0].username, "admin");
    EXPECT_EQ(users[0].role, "admin");
    EXPECT_GT(users[0].id, 0);
}

TEST_F(HospitalTest, RejectsInvalidFirstAdminDetails)
{
    EXPECT_FALSE(hospital->createFirstAdmin("", "Admin@123"));
    EXPECT_FALSE(hospital->createFirstAdmin("admin", "short"));
    EXPECT_FALSE(hospital->hasUsers());
}

TEST_F(HospitalTest, DoesNotAllowSecondFirstAdmin)
{
    ASSERT_TRUE(hospital->createFirstAdmin("admin", "Admin@123"));
    EXPECT_FALSE(hospital->createFirstAdmin("another", "Another@123"));
    EXPECT_EQ(hospital->getUsers().size(), 1u);
}

TEST_F(HospitalTest, RejectsDuplicateAdminUsername)
{
    ASSERT_TRUE(hospital->createFirstAdmin("admin", "Admin@123"));
    // The first-admin-only rule also prevents another admin from being added.
    EXPECT_FALSE(hospital->createFirstAdmin("admin", "Different@123"));
    EXPECT_EQ(hospital->getUsers().size(), 1u);
}

TEST_F(HospitalTest, AuthenticatesAdminAndFillsUser)
{
    ASSERT_TRUE(hospital->createFirstAdmin("admin", "Admin@123"));

    User user;
    ASSERT_TRUE(hospital->authenticate("admin", "Admin@123", user));
    EXPECT_GT(user.id, 0);
    EXPECT_EQ(user.username, "admin");
    EXPECT_EQ(user.role, "admin");
}

TEST_F(HospitalTest, AuthenticationRejectsWrongPasswordAndUnknownUser)
{
    ASSERT_TRUE(hospital->createFirstAdmin("admin", "Admin@123"));

    User user;
    EXPECT_FALSE(hospital->authenticate("admin", "WrongPassword", user));
    EXPECT_FALSE(hospital->authenticate("unknown", "Admin@123", user));
}

TEST_F(HospitalTest, AddsReceptionistAndApprover)
{
    ASSERT_TRUE(hospital->createFirstAdmin("admin", "Admin@123"));

    EXPECT_TRUE(hospital->addStaff("reception", "Reception@123", "receptionist"));
    EXPECT_TRUE(hospital->addStaff("approver", "Approver@123", "approver"));

    const auto users = hospital->getUsers();
    ASSERT_EQ(users.size(), 3u);
    EXPECT_EQ(users[1].username, "reception");
    EXPECT_EQ(users[1].role, "receptionist");
    EXPECT_EQ(users[2].username, "approver");
    EXPECT_EQ(users[2].role, "approver");
}

TEST_F(HospitalTest, RejectsInvalidStaffDetails)
{
    EXPECT_FALSE(hospital->addStaff("", "Password@123", "receptionist"));
    EXPECT_FALSE(hospital->addStaff("user", "short", "receptionist"));
    EXPECT_FALSE(hospital->addStaff("user", "Password@123", "admin"));
    EXPECT_FALSE(hospital->addStaff("user", "Password@123", "unknown"));
}

TEST_F(HospitalTest, RejectsDuplicateStaffUsername)
{
    ASSERT_TRUE(hospital->addStaff("reception", "Reception@123", "receptionist"));
    EXPECT_FALSE(hospital->addStaff("reception", "Another@123", "approver"));
    EXPECT_EQ(hospital->getUsers().size(), 1u);
}

TEST_F(HospitalTest, StaffCanAuthenticateWithTheirRole)
{
    ASSERT_TRUE(hospital->addStaff("reception", "Reception@123", "receptionist"));

    User user;
    ASSERT_TRUE(hospital->authenticate("reception", "Reception@123", user));
    EXPECT_EQ(user.username, "reception");
    EXPECT_EQ(user.role, "receptionist");
}

// Patient tests

TEST_F(HospitalTest, AddsAndListsPatients)
{
    ASSERT_TRUE(hospital->addPatient("Asha", 25, "Female", "9876543210"));
    ASSERT_TRUE(hospital->addPatient("Ravi", 40, "Male", "9876500000"));

    const auto patients = hospital->getPatients();
    ASSERT_EQ(patients.size(), 2u);
    EXPECT_GT(patients[0].id, 0);
    EXPECT_EQ(patients[0].name, "Asha");
    EXPECT_EQ(patients[0].age, 25);
    EXPECT_EQ(patients[0].gender, "Female");
    EXPECT_EQ(patients[0].phone, "9876543210");
    EXPECT_EQ(patients[1].name, "Ravi");
}

TEST_F(HospitalTest, AcceptsPatientAgeBoundaries)
{
    EXPECT_TRUE(hospital->addPatient("Newborn", 0, "Other", "9000000001"));
    EXPECT_TRUE(hospital->addPatient("Senior", 120, "Other", "9000000002"));
    EXPECT_EQ(hospital->getPatients().size(), 2u);
}

TEST_F(HospitalTest, RejectsInvalidPatientDetails)
{
    EXPECT_FALSE(hospital->addPatient("", 25, "Female", "9876543210"));
    EXPECT_FALSE(hospital->addPatient("Asha", -1, "Female", "9876543210"));
    EXPECT_FALSE(hospital->addPatient("Asha", 121, "Female", "9876543210"));
    EXPECT_FALSE(hospital->addPatient("Asha", 25, "", "9876543210"));
    EXPECT_FALSE(hospital->addPatient("Asha", 25, "Female", ""));
    EXPECT_TRUE(hospital->getPatients().empty());
}

TEST_F(HospitalTest, UpdatesPatientSuccessfully)
{
    ASSERT_TRUE(hospital->addPatient("Asha", 25, "Female", "9876543210"));

    EXPECT_TRUE(hospital->updatePatient(1, "Asha R", 26, "Female", "9000011111"));

    const auto patients = hospital->getPatients();
    ASSERT_EQ(patients.size(), 1u);
    EXPECT_EQ(patients[0].name, "Asha R");
    EXPECT_EQ(patients[0].age, 26);
    EXPECT_EQ(patients[0].phone, "9000011111");
}

TEST_F(HospitalTest, RejectsInvalidPatientUpdate)
{
    ASSERT_TRUE(hospital->addPatient("Asha", 25, "Female", "9876543210"));

    EXPECT_FALSE(hospital->updatePatient(0, "Asha", 25, "Female", "9876543210"));
    EXPECT_FALSE(hospital->updatePatient(-1, "Asha", 25, "Female", "9876543210"));
    EXPECT_FALSE(hospital->updatePatient(1, "", 25, "Female", "9876543210"));
    EXPECT_FALSE(hospital->updatePatient(1, "Asha", -1, "Female", "9876543210"));
    EXPECT_FALSE(hospital->updatePatient(1, "Asha", 121, "Female", "9876543210"));
    EXPECT_FALSE(hospital->updatePatient(1, "Asha", 25, "", "9876543210"));
    EXPECT_FALSE(hospital->updatePatient(1, "Asha", 25, "Female", ""));
}

TEST_F(HospitalTest, UpdateReturnsFalseWhenPatientDoesNotExist)
{
    EXPECT_FALSE(hospital->updatePatient(999, "Nobody", 30, "Other", "9000000000"));
}

TEST_F(HospitalTest, DeletesPatientSuccessfully)
{
    ASSERT_TRUE(hospital->addPatient("Asha", 25, "Female", "9876543210"));

    EXPECT_TRUE(hospital->deletePatient(1));
    EXPECT_TRUE(hospital->getPatients().empty());
}

TEST_F(HospitalTest, RejectsInvalidOrMissingPatientDeletion)
{
    EXPECT_FALSE(hospital->deletePatient(0));
    EXPECT_FALSE(hospital->deletePatient(-1));
    EXPECT_FALSE(hospital->deletePatient(999));
}

// Doctor tests

TEST_F(HospitalTest, AddsAndListsDoctors)
{
    ASSERT_TRUE(hospital->addDoctor("Dr Kumar", "Cardiology", "9123456780"));
    ASSERT_TRUE(hospital->addDoctor("Dr Priya", "Neurology", "9123456781"));

    const auto doctors = hospital->getDoctors();
    ASSERT_EQ(doctors.size(), 2u);
    EXPECT_GT(doctors[0].id, 0);
    EXPECT_EQ(doctors[0].name, "Dr Kumar");
    EXPECT_EQ(doctors[0].specialization, "Cardiology");
    EXPECT_EQ(doctors[0].phone, "9123456780");
    EXPECT_EQ(doctors[1].name, "Dr Priya");
}

TEST_F(HospitalTest, RejectsInvalidDoctorDetails)
{
    EXPECT_FALSE(hospital->addDoctor("", "Cardiology", "9123456780"));
    EXPECT_FALSE(hospital->addDoctor("Dr Kumar", "", "9123456780"));
    EXPECT_FALSE(hospital->addDoctor("Dr Kumar", "Cardiology", ""));
    EXPECT_TRUE(hospital->getDoctors().empty());
}

// Appointment tests

TEST_F(HospitalTest, CreatesAndListsAppointmentWithPendingStatus)
{
    createBasicRecords();

    ASSERT_TRUE(hospital->addAppointment(1, 1, "2026-12-01 10:30"));

    const auto appointments = hospital->getAppointments();
    ASSERT_EQ(appointments.size(), 1u);
    EXPECT_GT(appointments[0].id, 0);
    EXPECT_EQ(appointments[0].patientId, 1);
    EXPECT_EQ(appointments[0].doctorId, 1);
    EXPECT_EQ(appointments[0].patientName, "Asha");
    EXPECT_EQ(appointments[0].doctorName, "Dr Kumar");
    EXPECT_EQ(appointments[0].dateTime, "2026-12-01 10:30");
    EXPECT_EQ(appointments[0].status, "Pending");
}

TEST_F(HospitalTest, RejectsInvalidAppointmentArguments)
{
    createBasicRecords();

    EXPECT_FALSE(hospital->addAppointment(0, 1, "2026-12-01 10:30"));
    EXPECT_FALSE(hospital->addAppointment(-1, 1, "2026-12-01 10:30"));
    EXPECT_FALSE(hospital->addAppointment(1, 0, "2026-12-01 10:30"));
    EXPECT_FALSE(hospital->addAppointment(1, -1, "2026-12-01 10:30"));
    EXPECT_FALSE(hospital->addAppointment(1, 1, ""));
    EXPECT_TRUE(hospital->getAppointments().empty());
}

TEST_F(HospitalTest, RejectsAppointmentWithUnknownPatientOrDoctor)
{
    createBasicRecords();

    // Foreign-key constraints reject IDs that do not exist.
    EXPECT_FALSE(hospital->addAppointment(999, 1, "2026-12-01 10:30"));
    EXPECT_FALSE(hospital->addAppointment(1, 999, "2026-12-01 10:30"));
    EXPECT_TRUE(hospital->getAppointments().empty());
}

TEST_F(HospitalTest, ApprovesAppointment)
{
    const int id = createAppointment();

    EXPECT_TRUE(hospital->updateAppointmentStatus(id, "Approved"));
    const auto appointments = hospital->getAppointments();
    ASSERT_EQ(appointments.size(), 1u);
    EXPECT_EQ(appointments[0].status, "Approved");
}

TEST_F(HospitalTest, RejectsAppointment)
{
    const int id = createAppointment();

    EXPECT_TRUE(hospital->updateAppointmentStatus(id, "Rejected"));
    const auto appointments = hospital->getAppointments();
    ASSERT_EQ(appointments.size(), 1u);
    EXPECT_EQ(appointments[0].status, "Rejected");
}

TEST_F(HospitalTest, RejectsInvalidAppointmentStatusAndId)
{
    const int id = createAppointment();

    EXPECT_FALSE(hospital->updateAppointmentStatus(0, "Approved"));
    EXPECT_FALSE(hospital->updateAppointmentStatus(-1, "Approved"));
    EXPECT_FALSE(hospital->updateAppointmentStatus(id, "Pending"));
    EXPECT_FALSE(hospital->updateAppointmentStatus(id, "approved"));
    EXPECT_FALSE(hospital->updateAppointmentStatus(id, ""));
    EXPECT_FALSE(hospital->updateAppointmentStatus(999, "Approved"));

    const auto appointments = hospital->getAppointments();
    ASSERT_EQ(appointments.size(), 1u);
    EXPECT_EQ(appointments[0].status, "Pending");
}

TEST_F(HospitalTest, CannotDeletePatientLinkedToAppointment)
{
    createAppointment();

    // SQLite foreign-key protection prevents deleting a referenced patient.
    EXPECT_FALSE(hospital->deletePatient(1));
    EXPECT_EQ(hospital->getPatients().size(), 1u);
    EXPECT_EQ(hospital->getAppointments().size(), 1u);
}

TEST_F(HospitalTest, AppointmentListIsEmptyWhenNoAppointmentsExist)
{
    EXPECT_TRUE(hospital->getAppointments().empty());
}

} // namespace