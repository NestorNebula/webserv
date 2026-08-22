NAME=test

CC=c++
CFLAGS=-Wall -Wextra -Werror -std=c++98 -MMD -MP -g3
# -Wno-unused
# -Wshadow=local

GIT_DIR=../..

SRC_DIR=$(GIT_DIR)/src
INC_DIR=$(GIT_DIR)/inc
OBJ_DIR=.obj
LIB_DIR=.

CFLAGS+=-I . \
	-I $(INC_DIR) \
	-I $(INC_DIR)/server \
	-I $(INC_DIR)/utils \
	-I $(INC_DIR)/http \
	-I $(INC_DIR)/streams \
	-I $(INC_DIR)/resources

LFLAGS=
LIBS =

ifdef DBG
DEFS += -DDBG_CANON="$(DBG)"
endif

ifdef TEST
DEFS += -DTEST="$(TEST)"
endif

FILES = \
	utils/WsLog.cpp \
	utils/helpers.cpp \
	utils/http_utils.cpp \
	utils/libft.cpp \
	http/Headers.cpp \
	http/Request.cpp \
	http/Response.cpp \
	http/Session.cpp \
	resources/StaticResource.cpp \
	resources/DirectoryResource.cpp \
	resources/BuiltinResource.cpp \
	streams/Stream.cpp \
	streams/TemporaryFileStream.cpp \
	server/EpollClient.cpp \
	server/Epoll.cpp \
	server/Server.cpp \
	server/Socket.cpp \
	server/Connection.cpp \
	server/CgiEnv.cpp \
	server/CgiPipe.cpp \
	server/ResourceCgi.cpp \
	server/ResourcePiped.cpp \
	server/ResourceFcgi.cpp \
	server/FcgiMsg.cpp \
	server/FcgiConn.cpp \
	server/FcgiPipe.cpp \
	parsing/ConfigParser.cpp \
	parsing/ParseBlocks.cpp \
	parsing/ParserTools.cpp \
	parsing/Tokenizer.cpp \
	parsing/Validation.cpp 

TPP = 

SRCS = $(addprefix $(SRC_DIR)/, $(FILES))
OBJS = $(addprefix $(OBJ_DIR)/, $(FILES:.cpp=.o))

BIN = $(OBJ_DIR)/test.o
# OBJS += $(BIN)

NULL :=
\t   := $(NULL)	$(NULL)


all: $(NAME)

$(NAME): $(OBJS) $(TPP) $(BIN)
	$(CC) $(DEFS) $(CFLAGS) $(LFLAGS) $(OBJS) $(BIN) $(LIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(INC_DIR)/%.hpp | $(OBJ_DIR)
	@echo "$(<F)$(\t)=>$(\t)$(@F)"
	@$(CC) $(DEFS) $(CFLAGS) -o $@ -c $<

# ATTN : these rules over-rode detection of changes in (hpp) files 
# ---- : when they were place BEFORE it
$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	@echo "$(<F)$(\t)=>$(\t)$(@F)"
	@$(CC) $(DEFS) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
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
	@rm -fr $(OBJ_DIR)

fclean: clean
	@rm -f $(NAME)
	@rm -f fcgi fcgi.d

re: fclean $(NAME)

.PHONY:  all clean fclean re
