# Makefile
NAME    = webserver
CC      = g++
FLAGS   = -Wall -Wextra -Werror -std=c++98
SRCDIR  = ./src/
SRCS    = server.cpp client.cpp HttpRequest.cpp HttpResponse.cpp ConfigParser.cpp
OBJS    = $(SRCS:.cpp=.o)
RM      = rm -f

.PHONY: all clean fclean re

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(FLAGS) -o $(NAME) $(addprefix $(SRCDIR), $(OBJS))

%.o: $(SRCDIR)%.cpp
	$(CC) $(FLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all
