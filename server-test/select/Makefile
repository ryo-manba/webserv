NAME := server
SRCS := utility.cpp Server.cpp test_common.cpp main.cpp
OBJS := $(SRCS:.cpp=.o)
all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(SRCS) -o $(NAME)

clean:
	rm $(OBJS)

fclean: clean
	rm $(NAME)

re: fclean all

t: test

test: $(NAME)
	./$(NAME) 8080
#	curl http:/127.0.0.1:8080
#	pkill $(NAME)
