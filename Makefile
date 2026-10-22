CPP = c++

CFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS = main.cpp server.cpp

OBJS = $(SRCS:.cpp=.o)

NAME = ircserv

all: $(NAME)

$(NAME): $(OBJS)
	$(CPP) $(CFLAGS) -o $(NAME) $(OBJS)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re