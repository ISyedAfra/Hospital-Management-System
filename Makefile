CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic
LIBS = -lsqlite3 -lcrypto
COVERAGE = --coverage -O0 -g

.PHONY: all run test coverage clean
all: hospital

hospital: main.cpp hospital.cpp hospital.h
	$(CXX) $(CXXFLAGS) -o $@ main.cpp hospital.cpp $(LIBS)

run: hospital
	./hospital

hospital_tests: test_hospital.cpp hospital.cpp hospital.h
	$(CXX) $(CXXFLAGS) $(COVERAGE) -o $@ test_hospital.cpp hospital.cpp -lgtest $(LIBS)

test: hospital_tests
	./hospital_tests

coverage: hospital_tests
	./hospital_tests
	gcovr --root . --filter 'hospital\.cpp$$' --html-details -o coverage.html
	gcovr --root . --filter 'hospital\.cpp$$' --print-summary

clean:
	rm -f hospital hospital_tests *.gcda *.gcno *.gcov coverage.html coverage.css
