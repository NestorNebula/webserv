NAME=webserv

CC=c++
CFLAGS=-Wall -Wextra -Werror -std=c++98 -MMD -MP
# -Wno-unused
# -Wshadow=local

GIT_DIR=.

INC_DIR=$(GIT_DIR)/inc
SRC_DIR=$(GIT_DIR)/src
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
	utils/FilePath.cpp \
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
OBJS = $(addprefix $(OBJ_DIR)/, $(FILES:.cpp=.o)) \
		$(OBJ_DIR)/main.o
DEPS = $(OBJS:.o=.d)

# alternative approach
# OBJ = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/src/%.o,$(SRC)) \
# 	  $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(DEMO_SRC))

RM = rm -f
RMDIR = rm -fr
MKDIR = mkdir -p

NULL :=
\t   := $(NULL)	$(NULL)


all: $(NAME)

$(NAME): $(OBJS) $(TPP)
	$(CC) $(DEFS) $(CFLAGS) $(LFLAGS) $(OBJS) $(LIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(INC_DIR)/%.hpp 
	@$(MKDIR) $(dir $@)
	@echo "$(<F)$(\t)=>$(\t)$(@F)"
	@$(CC) $(DEFS) $(CFLAGS) -c $< -o $@ 

$(OBJ_DIR)/%.o: %.cpp 
	@$(MKDIR) $(dir $@)
	@echo "$(<F)$(\t)=>$(\t)$(@F)"
	@$(CC) $(DEFS) $(CFLAGS) -c $< -o $@ 
	
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp 
	@$(MKDIR) $(dir $@)
	@echo "$(<F)$(\t)=>$(\t)$(@F)"
	@$(CC) $(DEFS) $(CFLAGS) -c $< -o $@ 

clean:
	@$(RMDIR) $(OBJ_DIR)

fclean: clean
	@$(RM) $(NAME)

re: fclean $(NAME)

-include $(DEPS)

.PHONY:  all clean fclean re
