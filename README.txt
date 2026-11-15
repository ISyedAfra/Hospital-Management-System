HOSPITAL MANAGEMENT SYSTEM - C++ POC

Files:
hospital.h         Class declarations and data structures
hospital.cpp       SQLite database, authentication and data operations
main.cpp           Console menus and user interaction
test_hospital.cpp  Google Test unit tests
Makefile           Build, run, test and coverage commands

Install on Ubuntu / GitHub Codespaces:
sudo apt update
sudo apt install g++ libgtest-dev libsqlite3-dev libssl-dev gcovr

Build application:
make

Run application:
make run

First run asks you to create the first administrator account.
Password must contain at least 8 characters.

Run unit tests:
make test

Generate coverage report:
make coverage

Open coverage.html to inspect the coverage report.
Google Tests use an in-memory database (:memory:) and do not modify hospital.db.

This is an educational POC. Use sample data only, not real patient records.
