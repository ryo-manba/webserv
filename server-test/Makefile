NAME := server
SRCS := main.cpp

all: $(NAME)

$(NAME):
	c++ $(SRCS)

clean:
	rm a.out

test: $(NAME)
	./a.out &
	curl http:/127.0.0.1:8080
	pkill a.out
