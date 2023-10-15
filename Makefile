# Variáveis para configurar o compilador e as opções de compilação
CC = gcc
CFLAGS = -Wall -g

# Lista de arquivos de origem
SOURCES = pingpong-preempcao.c ppos-core-aux.c libppos_static.a

# Nome do arquivo de saída
TARGET = ppos-test

# Regra principal para a compilação
$(TARGET): $(SOURCES)
    $(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

# Regra para limpar os arquivos intermediários e o executável
clean:
    rm -f $(TARGET)

# Marcar as regras "clean" e "all" como regras que não dependem de arquivos
.PHONY: clean all

# Regra padrão quando você simplesmente digita "make"
all: $(TARGET)