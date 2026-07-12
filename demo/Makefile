NAME = http-demo

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD -MP

CPPFLAGS = -I../inc

RM = rm -rf
MKDIR = mkdir -p

OBJ_DIR = .obj

SRC_DIR = ../src

SRC = $(SRC_DIR)/Headers.cpp \
	  $(SRC_DIR)/helpers.cpp \
	  $(SRC_DIR)/Request.cpp \
	  $(SRC_DIR)/Response.cpp \
	  $(SRC_DIR)/StaticResource.cpp \
	  $(SRC_DIR)/DirectoryResource.cpp \
	  $(SRC_DIR)/BuiltinResource.cpp \
	  $(SRC_DIR)/Stream.cpp \
	  $(SRC_DIR)/TemporaryFileStream.cpp \
	  $(SRC_DIR)/http_utils.cpp \
	  $(SRC_DIR)/utils/WsLog.cpp \
	  $(SRC_DIR)/Session.cpp \

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
