HOSPITAL MANAGEMENT SYSTEM

1. Project Overview

The Hospital Management System is a console-based application developed using C++. It helps manage hospital records such as staff, patients, doctors, and appointments. It includes role-based login and stores records permanently using an SQLite database.

2. Technologies Used

- Programming Language: C++ (C++17)
- Database: SQLite
- Password Security: OpenSSL
- Unit Testing: Google Test (GTest)
- Code Coverage: gcov and gcovr
- Memory Testing: Valgrind
- Thread Analysis: Helgrind
- Static Code Analysis: Cppcheck
- Build Automation: Makefile

3. Main Features

- Secure login with password hashing.
- Role-based access for Admin, Receptionist, and Approver.
- Add and manage staff accounts.
- Add, view, update, and delete patient records.
- Add and view doctor details.
- Create and view appointments.
- Approve or reject appointments.
- Store records in an SQLite database.

4. Project Files

- main.cpp – Contains the main program and menu handling.
- hospital.cpp – Contains the implementation of hospital management functions.
- hospital.h – Contains class declarations and function definitions.
- test_hospital.cpp – Contains Google Test unit test cases.
- Makefile – Automates compilation, testing, coverage, and analysis.
- hospital.db – SQLite database file created when the application runs.

5. Installation Requirements

Install the required packages:

sudo apt update

sudo apt install g++ libgtest-dev libsqlite3-dev libssl-dev gcovr valgrind cppcheck

6. Build and Execution

Compile the application:

make

Run the application:

make run

7. Unit Testing

Run Google Test cases:

make test

8. Code Coverage

Generate the code coverage report:

make coverage

The HTML report is generated as coverage.html.

9. Memory Leak Detection

Run Valgrind:

make valgrind

This checks for memory leaks and memory-related errors.

10. Thread Analysis

Run Helgrind:

make helgrind

This checks for potential threading and synchronization issues. The current application is single-threaded, so the results should be interpreted accordingly.

11. Static Code Analysis

Run Cppcheck:

make cppcheck

This checks the source code for potential errors, coding issues, and other problems.

12. Clean Generated Files

To remove generated executables and coverage files:

make clean

13. Important Note

This project is developed as an educational proof of concept. It should not be used to store real patient information without additional security, privacy, authorization, backup, and compliance measures.
