CC := gcc

# To eliminate debugging messages use -DNDEBUG
CFLAGS := -O0 -std=gnu11 -Wall -Werror -Wno-unused-parameter -Werror=vla -g \
	  -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 -D_GNU_SOURCE
CPPFLAGS := `pkg-config --cflags glib-2.0`
LDFLAGS :=`pkg-config --libs glib-2.0` -lfuse

export CC
export CFLAGS
export CPPFLAGS
export LDFLAGS


HEADERS := $(wildcard *.h)
SOURCES := $(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)
TARGET := fat-fuse

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)


test-ht: hierarchy_tree.o
	make -C tests test_ht

test-ft:
	make -C tests test_ft

mount:
	@echo "Limpiando..."
	@make clean
	@echo "Compilando..."
	@make
	@echo "Montando imagen modo foreground..."
	@./fat-fuse -f ../imagenes/bb_fs.img ~/mnt

umount:
	@echo "Desmontando..."
	@fusermount -u ~/mnt
	@echo "Limpiando..."
	@make clean

debugMode:
	@echo "Limpiando..."
	@make clean
	@echo "Compilando..."
	@make
	@echo "Montando imagen modo debug..."
	@./fat-fuse -d ../imagenes/bb_fs.img ~/mnt

gdbMode:
	@echo "Limpiando..."
	@make clean
	@echo "Compilando..."
	@make
	@echo "Entrando en gdb"
	@echo "Agregue los breakpoint"
	@echo "para correr escriba: run -f ../imagenes/bb_fs.img ~/mnt"
	@gdb ./fat-fuse

clang_format:
	@echo "Aplicando clang_format..."
	@git-clang-format *.c *.h


image_clean:
	@echo "Limpiando imagen..."
	@rm ../imagenes/bb_fs.img
	@cp ../imagenes/bb_fs.img.bak ../imagenes/bb_fs.img

clean:
	rm -f $(TARGET) $(OBJECTS) tags cscope*
	make -C tests clean

.PHONY: clean
