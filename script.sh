echo ------ script test begin -------
echo test command: find "include" in myShell.c
echo display result:
cat < myShell.c | grep include > test
cat test
echo ------ script test end -------