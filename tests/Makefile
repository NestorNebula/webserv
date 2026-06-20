NAME = unit_tests

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98

CPPFLAGS = -I../include $(GTEST_INC)

RM = rm -rf

MKDIR = mkdir -p

GTEST_DIR = gtest

GTEST_SRC = $(GTEST_DIR)/googletest/src/gtest-all.cc

GTEST_INC = -I$(GTEST_DIR)/googletest \
	-I$(GTEST_DIR)/googletest/include
  
GTEST_OBJ = .obj/gtest-all.o

GTEST_CXXFLAGS = -std=c++98

PROJECT_SRC = ../src/Headers.cpp ../src/Request.cpp

TEST_SRC = Request.cpp main.cpp

OBJ_DIR = .obj

PROJECT_OBJ = $(patsubst ../src/%.cpp,$(OBJ_DIR)/src/%.o,$(PROJECT_SRC))

TEST_OBJ    = $(patsubst %.cpp,$(OBJ_DIR)/tests/%.o,$(TEST_SRC))

OBJ = $(PROJECT_OBJ) $(TEST_OBJ)

all: $(NAME)

$(NAME): $(OBJ) $(GTEST_OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) $(GTEST_OBJ) -o $@

$(GTEST_OBJ): $(GTEST_SRC)
	@$(MKDIR) $(dir $@)
	$(CXX) $(GTEST_CXXFLAGS) $(CPPFLAGS) -c $< -o $@

.obj/src/%.o: ../src/%.cpp
	@$(MKDIR) $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

.obj/tests/%.o: %.cpp
	@$(MKDIR) $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all
