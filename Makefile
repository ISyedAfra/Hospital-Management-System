CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic
LIBS = -lsqlite3 -lcrypto
COVERAGE = --coverage -O0 -g

APP = hospital
TEST = hospital_tests

.PHONY: all run test coverage valgrind helgrind cppcheck clean

all: $(APP)

$(APP): main.cpp hospital.cpp hospital.h
	$(CXX) $(CXXFLAGS) -o $(APP) main.cpp hospital.cpp $(LIBS)

run: $(APP)
	./$(APP)

$(TEST): test_hospital.cpp hospital.cpp hospital.h
	$(CXX) $(CXXFLAGS) $(COVERAGE) -o $(TEST) test_hospital.cpp hospital.cpp -lgtest -lgtest_main $(LIBS)

test: $(TEST)
	./$(TEST)

coverage: $(TEST)
	./$(TEST)
	gcovr --root . --filter 'hospital\.cpp$$' --html-details -o coverage.html
	gcovr --root . --filter 'hospital\.cpp$$' --print-summary

valgrind: $(TEST)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST)

helgrind: $(TEST)
	valgrind --tool=helgrind --error-exitcode=1 ./$(TEST)

cppcheck:
	cppcheck --enable=warning,style,performance,portability --error-exitcode=1 --suppress=missingIncludeSystem hospital.cpp main.cpp

clean:
	rm -f $(APP) $(TEST) *.gcda *.gcno *.gcov coverage.html coverage.css
