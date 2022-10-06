default:
	clang test/*.c src/*.c -Isrc/ -o build/secs

docs:
	headerdoc2html -udpb src/secs.h -o docs/
	gatherheaderdoc docs/
	mv docs/masterTOC.html docs/index.html

.PHONY: default
