TARGET := balancing_scale
SOURCES := src/main.cpp

CXXFLAGS := -Wall -Wextra -Wpedantic -Werror -std=gnu++20 -g

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	$(RM) $(TARGET)

