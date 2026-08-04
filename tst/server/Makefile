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

CFLAGS+=-I . -I $(INC_DIR) -I $(INC_DIR)/server 
LFLAGS=
LIBS =

FCGI_INC=-I ./FCGI/include
FCGI_LIB=-L ./FCGI/lib -Wl,-rpath,./FCGI/lib/ -lfcgi

CFLAGS += $(FCGI_INC)
LIBS += $(FCGI_LIB)

ifdef DBG
DEFS += -DDBG_CANON="$(DBG)"
endif

ifdef TEST
DEFS += -DTEST="$(TEST)"
endif

FILES = \
	bridge.cpp \
	utils/WsLog.cpp \
	server/EpollClient.cpp \
	server/Epoll.cpp \
	server/Server.cpp \
	server/Socket.cpp \
	server/Connection.cpp \
	server/CgiEnv.cpp \
	server/CgiPipe.cpp \
	server/ResourceCgi.cpp 

TPP = 

SRCS = $(addprefix $(SRC_DIR)/, $(FILES))
OBJS = $(addprefix $(OBJ_DIR)/, $(FILES:.cpp=.o))

BIN = $(OBJ_DIR)/test.o
# OBJS += $(BIN)

OBJS += $(OBJ_DIR)/FcgiMsg.o

NULL :=
\t   := $(NULL)	$(NULL)


all: $(NAME)

$(NAME): $(OBJS) $(TPP) $(BIN)
	$(CC) $(DEFS) $(CFLAGS) $(LFLAGS) $(OBJS) $(BIN) $(LIBS) -o $@

fcgi: $(OBJS)
	$(CC) $(FCGI_INC) $(DEFS) -DBUILD_MAIN_FCGI=1 $(CFLAGS) $(LFLAGS) $(OBJS) FcgiConn.cpp $(LIBS) $(FCGI_LIB) -o fcgi


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
	@rm -f fcgi fcgi.d

re: fclean $(NAME)

.PHONY:  all clean fclean re
