# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/06/29 15:47:03 by mamarti           #+#    #+#              #
#    Updated: 2026/06/29 16:07:48 by mamarti          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

RESET   	= \033[0;39m
GREEN 		= \033[0;92m
YELLOW      = \033[0;93m
RED         = \033[0;91m

SRC =	main.cpp parsing/ConfigParser.cpp parsing/Tokenizer.cpp \
		parsing/ParserTools.cpp parsing/ParseBlocks.cpp parsing/Validation.cpp \

SRC_DIR			=	src
SRCS			=	$(addprefix $(SRC_DIR)/, $(SRC))
OBJ_DIR			=	objs
OBJS			=	$(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))
DEPS			=	$(OBJS:.o=.d)

NAME			=	webserv
CC				=	c++
CFLAGS			=	-Wall -Wextra -Werror -MMD -MP -std=c++98

all:			$(NAME)

$(NAME):		$(OBJS)
				@echo "$(YELLOW)[webserv] Linking $(NAME)...$(RESET)"
				@$(CC) $(CFLAGS) $(OBJS) -o $(NAME)
				@echo "$(GREEN)[webserv] $(NAME) compilation success!$(RESET)"

$(OBJ_DIR)/%.o: %.cpp $(HEADERS)
				@mkdir -p $(dir $@)
				@echo "$(YELLOW)[webserv] Compiling : $< $(RESET)"
				@$(CC) $(CFLAGS) -c $< -o $@

clean:
				@$(RM) -rf $(OBJ_DIR)
				@echo "$(GREEN)[webserv] Object files cleaned!$(RESET)"

fclean:			clean
				@$(RM) -f $(NAME)
				@echo "$(GREEN)[webserv] All executables cleaned!$(RESET)"

re:				fclean all

.PHONY:			all clean fclean re
