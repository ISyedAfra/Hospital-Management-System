#include "hospital.h"
#include <gtest/gtest.h>
class HospitalTest:public::testing::Test{
protected: HospitalSystem* h=nullptr;
void SetUp()override{h=new HospitalSystem(":memory:");ASSERT_TRUE(h->isOpen());ASSERT_TRUE(h->initialize());}
void TearDown()override{delete h;}
};
TEST_F(HospitalTest,AdminSetupAndAuthentication){
 EXPECT_FALSE(h->hasUsers());EXPECT_TRUE(h->createFirstAdmin("admin","Admin@123"));
 EXPECT_FALSE(h->createFirstAdmin("second","Second@123"));
 User u;EXPECT_TRUE(h->authenticate("admin","Admin@123",u));EXPECT_EQ(u.role,"admin");
 EXPECT_FALSE(h->authenticate("admin","wrongpass",u));EXPECT_FALSE(h->authenticate("unknown","Admin@123",u));
}
TEST_F(HospitalTest,RejectsShortAdminPassword){EXPECT_FALSE(h->createFirstAdmin("admin","short"));}
TEST_F(HospitalTest,StaffRolesAndDuplicateNames){
 ASSERT_TRUE(h->createFirstAdmin("admin","Admin@123"));
 EXPECT_TRUE(h->addStaff("desk","Staff@123","receptionist"));
 EXPECT_TRUE(h->addStaff("approve","Approve@123","approver"));
 EXPECT_FALSE(h->addStaff("bad","Password@123","doctor"));
 EXPECT_FALSE(h->addStaff("desk","Another@123","receptionist"));
 EXPECT_EQ(h->getUsers().size(),3u);
}
TEST_F(HospitalTest,PatientCRUD){
 EXPECT_TRUE(h->addPatient("Rahul",28,"Male","9876543210"));auto v=h->getPatients();
 ASSERT_EQ(v.size(),1u);int id=v[0].id;EXPECT_EQ(v[0].name,"Rahul");
 EXPECT_TRUE(h->updatePatient(id,"Rahul Kumar",29,"Male","9876543211"));
 EXPECT_EQ(h->getPatients()[0].age,29);EXPECT_TRUE(h->deletePatient(id));
 EXPECT_FALSE(h->deletePatient(id));EXPECT_TRUE(h->getPatients().empty());
}
TEST_F(HospitalTest,RejectsInvalidPatient){EXPECT_FALSE(h->addPatient("",20,"Male","123"));EXPECT_FALSE(h->addPatient("A",-1,"Male","123"));EXPECT_FALSE(h->addPatient("A",121,"Male","123"));}
TEST_F(HospitalTest,DoctorCreateRead){EXPECT_TRUE(h->addDoctor("Dr Arun","Cardiology","123"));auto v=h->getDoctors();ASSERT_EQ(v.size(),1u);EXPECT_EQ(v[0].specialization,"Cardiology");EXPECT_FALSE(h->addDoctor("","",""));}
TEST_F(HospitalTest,AppointmentAndStatus){
 ASSERT_TRUE(h->addPatient("Priya",35,"Female","123"));ASSERT_TRUE(h->addDoctor("Dr Meena","Neurology","456"));
 EXPECT_FALSE(h->addAppointment(99,1,"2026-11-20 10:30"));ASSERT_TRUE(h->addAppointment(1,1,"2026-11-20 10:30"));
 auto v=h->getAppointments();ASSERT_EQ(v.size(),1u);EXPECT_EQ(v[0].status,"Pending");EXPECT_EQ(v[0].patientName,"Priya");
 EXPECT_TRUE(h->updateAppointmentStatus(1,"Approved"));EXPECT_EQ(h->getAppointments()[0].status,"Approved");
 EXPECT_TRUE(h->updateAppointmentStatus(1,"Rejected"));EXPECT_FALSE(h->updateAppointmentStatus(1,"Pending"));EXPECT_FALSE(h->updateAppointmentStatus(99,"Approved"));
}
TEST_F(HospitalTest,PatientWithAppointmentCannotBeDeleted){
 ASSERT_TRUE(h->addPatient("A",20,"F","123"));ASSERT_TRUE(h->addDoctor("D","General","456"));ASSERT_TRUE(h->addAppointment(1,1,"2026-11-20"));
 EXPECT_FALSE(h->deletePatient(1));
}
int main(int argc,char** argv){::testing::InitGoogleTest(&argc,argv);return RUN_ALL_TESTS();}
