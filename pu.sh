headerdoc2html -pqt src/secs.h -o docs/
gatherheaderdoc docs/
mv docs/masterTOC.html docs/index.html
cd tests/
make all
SUCCESS=$?
cd ../
if [ $SUCCESS -eq 0 ];
then
    printf "\e[0;32mGOOD TO GO!\e[m\n"
else
    printf "\e[0;31mFIX YOUR SHIT!\e[m\n"
fi
