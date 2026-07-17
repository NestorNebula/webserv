NAME = http-demo

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD -MP

CPPFLAGS = -I../inc \
		   -I../inc/http \
		   -I../inc/resources \
		   -I../inc/streams \
		   -I../inc/utils \

RM = rm -rf
MKDIR = mkdir -p

OBJ_DIR = .obj

SRC_DIR = ../src

SRC = $(SRC_DIR)/http/Headers.cpp \
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

DEMO_SRC = main.cpp

OBJ = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/src/%.o,$(SRC)) \
	  $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(DEMO_SRC))

DEP = $(OBJ:.o=.d)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJ_DIR)/src/%.o: $(SRC_DIR)/%.cpp
	@$(MKDIR) $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp
	@$(MKDIR) $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

-include $(DEP)

.PHONY: all clean fclean re
