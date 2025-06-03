# Makefile
NAME    = parser
CC       = g++
FLAGS  = -Wall -Wextra -Werror -std=c++98

SRCS      = Parser.cpp main.cpp
OBJS      = $(SRCS:.cpp=.o)
HDRS      = HttpEnums.hpp HttpRequest.hpp HttpResponse.hpp Parser.hpp
RM		= rm -f



.PHONY: all clean

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(FLAGS) -o $(NAME) $(OBJS)

%.o: %.cpp $(HDRS)
	$(CC) $(FLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)