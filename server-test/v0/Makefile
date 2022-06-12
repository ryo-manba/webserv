NAME := server
SRCS := main.cpp

all: $(NAME)

$(NAME):
	c++ $(SRCS) -o $(NAME)

clean:
	rm $(NAME)

test: $(NAME)
	./$(NAME) &
	curl http:/127.0.0.1:8080
	pkill $(NAME)
