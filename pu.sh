headerdoc2html -pqt src/secs.h -o docs/
gatherheaderdoc docs/
mv docs/masterTOC.html docs/index.html
cd tests/
make all
SUCCESS=$?
cd ../
if [[ $RET -eq 0 ]];
then
    echo "GOOD TO GO!"
else
    echo "FIX YOUR SHIT!"
fi
