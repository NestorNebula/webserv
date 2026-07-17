NAME = unit_tests

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD -MP
GTEST_CXXFLAGS = -std=c++98

CPPFLAGS = -I../inc \
		   -I../inc/http \
		   -I../inc/resources \
		   -I../inc/streams \
		   -I../inc/utils \
		   -Igtest/googletest \
		   -Igtest/googletest/include \
		   -Igtest/googlemock \
		   -Igtest/googlemock/include

RM = rm -rf
MKDIR = mkdir -p

OBJ_DIR = .obj

GTEST_DIR = gtest

SRC_DIR = ../src

PROJECT_SRC = $(SRC_DIR)/http/Headers.cpp \
			  $(SRC_DIR)/utils/helpers.cpp \
			  $(SRC_DIR)/http/Request.cpp \
			  $(SRC_DIR)/http/Response.cpp \
			  $(SRC_DIR)/resources/StaticResource.cpp \
			  $(SRC_DIR)/resources/DirectoryResource.cpp \
			  $(SRC_DIR)/resources/BuiltinResource.cpp \
			  $(SRC_DIR)/streams/Stream.cpp \
			  $(SRC_DIR)/streams/TemporaryFileStream.cpp \
			  $(SRC_DIR)/utils/http_utils.cpp \
			  $(SRC_DIR)/utils/WsLog.cpp \
			  $(SRC_DIR)/http/Session.cpp \
			  $(SRC_DIR)/parsing/ConfigParser.cpp \
			  $(SRC_DIR)/parsing/ParseBlocks.cpp \
			  $(SRC_DIR)/parsing/ParserTools.cpp \
			  $(SRC_DIR)/parsing/Tokenizer.cpp \
			  $(SRC_DIR)/parsing/Validation.cpp \
			  $(SRC_DIR)/server/CgiEnv.cpp \
			  $(SRC_DIR)/server/CgiPipe.cpp \
			  $(SRC_DIR)/server/Connection.cpp \
			  $(SRC_DIR)/server/Epoll.cpp \
			  $(SRC_DIR)/server/EpollClient.cpp \
			  $(SRC_DIR)/server/Server.cpp \
			  $(SRC_DIR)/server/Socket.cpp \

TEST_SRC_DIR = .

TEST_SRC = $(TEST_SRC_DIR)/Request.cpp \
		   $(TEST_SRC_DIR)/Response.cpp \
		   $(TEST_SRC_DIR)/StaticResource.cpp \
		   $(TEST_SRC_DIR)/DirectoryResource.cpp \
		   $(TEST_SRC_DIR)/BuiltinResource.cpp \
		   $(TEST_SRC_DIR)/http_utils.cpp \
		   $(TEST_SRC_DIR)/Session.cpp \
		   $(TEST_SRC_DIR)/main.cpp

GTEST_SRC = $(GTEST_DIR)/googletest/src/gtest-all.cc
GMOCK_SRC = $(GTEST_DIR)/googlemock/src/gmock-all.cc

PROJECT_OBJ = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/src/%.o,$(PROJECT_SRC))
TEST_OBJ    = $(patsubst $(TEST_SRC_DIR)/%.cpp,$(OBJ_DIR)/tests/%.o,$(TEST_SRC))

GTEST_OBJ = $(OBJ_DIR)/gtest-all.o
GMOCK_OBJ = $(OBJ_DIR)/gmock-all.o

OBJ = $(PROJECT_OBJ) $(TEST_OBJ)

DEP = $(OBJ:.o=.d)

all: $(NAME)

$(NAME): $(OBJ) $(GTEST_OBJ) $(GMOCK_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJ_DIR)/src/%.o: $(SRC_DIR)/%.cpp
	@$(MKDIR) $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR)/tests/%.o: %.cpp
	@$(MKDIR) $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(GTEST_OBJ): $(GTEST_SRC)
	@$(MKDIR) $(dir $@)
	$(CXX) $(GTEST_CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(GMOCK_OBJ): $(GMOCK_SRC)
	@$(MKDIR) $(dir $@)
	$(CXX) $(GTEST_CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

-include $(DEP)

.PHONY: all clean fclean re
