NAME=test

CC=c++
CFLAGS=-std=c++98 -Wall -Wextra -Werror -MMD -MP -g3
# -Wno-unused
# -Wshadow=local

GIT_DIR=../..
SRC_DIR=$(GIT_DIR)/src
INC_DIR=$(GIT_DIR)/inc
OBJ_DIR=.obj
LIB_DIR=.

CFLAGS+=-I $(INC_DIR) 
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
	server/EpollClient.cpp \
	server/Epoll.cpp \
	server/Server.cpp \
	server/Socket.cpp \
	server/Connection.cpp \
	server/CgiEnv.cpp \
	server/CgiPipe.cpp

TPP = 

SRCS = $(addprefix $(SRC_DIR)/, $(FILES))
OBJS = $(addprefix $(OBJ_DIR)/, $(FILES:.cpp=.o))

BIN = $(OBJ_DIR)/test.o

OBJS += $(BIN)

NULL :=
\t   := $(NULL)	$(NULL)


all: $(NAME)

$(NAME): $(OBJS) $(TPP)
	$(CC) $(DEFS) $(CFLAGS) $(LFLAGS) $(OBJS) $(LIBS) -o $@

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
	@mkdir -p $(OBJ_DIR)/server

clean:
	@rm -fr $(OBJ_DIR)

fclean: clean
	@rm -f $(NAME)

re: fclean $(NAME)

.PHONY:  all clean fclean re
