NAME=test

CC=c++
CFLAGS=-std=c++98 -Wall -Wextra -Werror -MMD -MP -g3 
# -Wno-unused
# -Wshadow=local

SRC_DIR=.
INC_DIR=.
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
	WsLog.cpp \
	EpollClient.cpp \
	Epoll.cpp \
	Server.cpp \
	Connection.cpp \
	CgiPipe.cpp \
	test.cpp

TPP = 

SRCS = $(addprefix $(SRC_DIR)/, $(FILES:.cpp=.o))
OBJS = $(addprefix $(OBJ_DIR)/, $(FILES:.cpp=.o))

NULL :=
\t   := $(NULL)	$(NULL)

all: $(NAME)

$(NAME): $(OBJS) $(TPP)
	$(CC) $(DEFS) $(CFLAGS) $(LFLAGS) $(OBJS) $(LIBS) -o $@

# $(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(SRC_DIR)/%.hpp | $(OBJ_DIR)
$(OBJ_DIR)/%.o: %.cpp %.hpp | $(OBJ_DIR)
	@echo "$(<F)$(\t)=>$(\t)$(@F)"
	@$(CC) $(DEFS) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	@echo "$(<F)$(\t)=>$(\t)$(@F)"
	@$(CC) $(DEFS) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

clean:
	@rm -fr $(OBJ_DIR)

fclean: clean
	@rm -f $(NAME)

re: fclean $(NAME)

.PHONY:  all clean fclean re
