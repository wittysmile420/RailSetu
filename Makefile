CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -pthread
INCLUDES = -Iinclude
SOURCES = src/Logger.cpp src/Persistence.cpp src/BookingManager.cpp src/ConsoleUi.cpp

.PHONY: all app test stress clean

all: app stress test

app: $(SOURCES) src/main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) src/main.cpp -o railsetu_app

stress: $(SOURCES) src/stress.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) src/stress.cpp -o railsetu_stress

test: $(SOURCES) tests/test_booking.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) tests/test_booking.cpp -o railsetu_tests
	./railsetu_tests
	@for test in test_concurrent_booking test_cancellation test_waiting_promotion test_semaphore test_persistence test_race_validation test_visual_features; do \
		$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) tests/$$test.cpp -o $$test; \
		./$$test; \
	done

clean:
	rm -f railsetu_app railsetu_stress railsetu_tests railsetu.log railsetu.json
