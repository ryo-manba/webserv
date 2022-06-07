NAME := server
SRCS := Server.cpp test_common.cpp main.cpp
OBJS := $(SRCS:.cpp=.o)
all: $(NAME)
CXXFLAGS := -Wall -Wextra -Werror -g -fsanitize=address

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(NAME)

clean:
	rm -rf $(OBJS)

fclean: clean
	rm -rf $(NAME)

re: fclean all

t: test

test: $(NAME)
	./$(NAME)
