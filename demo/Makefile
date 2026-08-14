NAME = demo

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD -MP

CFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD -MP \
	-I../inc \
	-I../inc/http \
	-I../inc/resources \
	-I../inc/streams \
	-I../inc/utils  \
	-I../inc/server \
	-I../tst/server \
	-I ../tst/server/FCGI/include

RM = rm -rf
MKDIR = mkdir -p

OBJ_DIR = .obj

SRC_DIR = ../src

FILES = http/Headers.cpp \
	  utils/helpers.cpp \
	  http/Request.cpp \
	  http/Response.cpp \
	  resources/StaticResource.cpp \
	  resources/DirectoryResource.cpp \
	  resources/BuiltinResource.cpp \
	  streams/Stream.cpp \
	  streams/TemporaryFileStream.cpp \
	  utils/http_utils.cpp \
	  utils/WsLog.cpp \
	  http/Session.cpp \
	  parsing/ConfigParser.cpp \
	  parsing/ParseBlocks.cpp \
	  parsing/ParserTools.cpp \
	  parsing/Tokenizer.cpp \
	  parsing/Validation.cpp \
	utils/WsLog.cpp \
	server/EpollClient.cpp \
	server/Epoll.cpp \
	server/Server.cpp \
	server/Socket.cpp \
	server/Connection.cpp \
	server/CgiEnv.cpp \
	server/CgiPipe.cpp \
	server/ResourceCgi.cpp \
	server/FcgiMsg.cpp \
	server/FcgiConn.cpp \
	server/FcgiPipe.cpp \
	server/bridge.cpp \
	main.cpp

# DEMO_SRC = main.cpp

# OBJ = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/src/%.o,$(SRC)) \
# 	  $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(DEMO_SRC))

SRCS = $(addprefix $(SRC_DIR)/, $(FILES))
OBJS = $(addprefix $(OBJ_DIR)/, $(FILES:.cpp=.o))

# DEP = $(OBJ:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CFLAGS) $^ -o $@

# $(OBJ_DIR)/src/%.o: $(SRC_DIR)/%.cpp
# 	@$(MKDIR) $(dir $@)
# 	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# $(OBJ_DIR)/%.o: %.cpp
# 	@$(MKDIR) $(dir $@)
# 	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@


$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	@echo "$(<F)$(\t)=>$(\t)$(@F)"
	@$(CC) $(DEFS) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@echo "$(<F)$(\t)=>$(\t)$(@F)"
	@$(CC) $(DEFS) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(INC_DIR)/%.hpp | $(OBJ_DIR)
	@echo "$(<F)$(\t)=>$(\t)$(@F)"
	@$(CC) $(DEFS) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/utils
	@mkdir -p $(OBJ_DIR)/http
	@mkdir -p $(OBJ_DIR)/resources
	@mkdir -p $(OBJ_DIR)/streams
	@mkdir -p $(OBJ_DIR)/parsing
	@mkdir -p $(OBJ_DIR)/server



clean:
	$(RM) $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

-include $(DEP)

.PHONY: all clean fclean re
