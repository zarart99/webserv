# Makefile
NAME    = webserver
CC      = g++
FLAGS   = -Wall -Wextra -Werror -std=c++98
SRCDIR  = ./src/
OBJDIR  = $(SRCDIR)obj/
SRCS    = server.cpp client.cpp HttpRequest.cpp HttpResponse.cpp webserv.cpp ConfigParser.cpp RequestHandler.cpp Cgi.cpp
OBJS    = $(addprefix $(OBJDIR), $(SRCS:.cpp=.o)) 
RM      = rm -f

.PHONY: all clean fclean re

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(FLAGS) -o $@ $^

$(OBJDIR)%.o: $(SRCDIR)%.cpp
	@mkdir -p $(OBJDIR)
	$(CC) $(FLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all
