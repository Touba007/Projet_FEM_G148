# === VARIABLES ===
CC = gcc
CFLAGS = -Wall -Iheaders -I/opt/homebrew/include -DGL_SILENCE_DEPRECATION
LDFLAGS = -L/opt/homebrew/lib -lglfw -lgmsh -framework OpenGL -lm
SRC = src/run.c src/fem.c src/glfem.c
EXEC = monProjet

# === REGLES ===

all: build

build:
	$(CC) $(CFLAGS) -o $(EXEC) $(SRC) $(LDFLAGS)
	@echo "Build terminé."

run: build
	./$(EXEC) $(ARGS)


clean:
	rm -f $(EXEC)
	@echo "Nettoyage terminé."
