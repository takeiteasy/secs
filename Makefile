default:
	clang test/*.c src/*.c -Isrc/ -o secs

.PHONY: default