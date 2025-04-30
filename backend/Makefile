# Compilatore
CC = gcc

# Opzioni di compilazione
CFLAGS = -Wall -Wextra -std=c11 -I./includes 

# Librerie da linkare
LDFLAGS = -ljansson -lpthread

# Nome dell'eseguibile finale
TARGET = tris_server

# Directory dei file oggetto
OBJDIR = src/obj

# File sorgenti e oggetti
SRCS = src/client.c src/server.c src/game.c src/main.c src/messages.c src/routing.c
OBJS = $(SRCS:src/%.c=$(OBJDIR)/%.o)

# Header files
HEADERS = includes/client.h includes/server.h includes/game.h includes/messages.h includes/routing.h

# Regola predefinita
all: $(OBJDIR) $(TARGET)

# Crea la directory degli oggetti
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Link dell'eseguibile
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Regola generica per compilare .c -> .o
$(OBJDIR)/%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Pulizia
clean:
	clear; rm -rfv $(OBJDIR)/*.o; rm -fv $(TARGET)

.PHONY: all clean distclean
