@echo off

echo -------
echo -------

set Wildcard=*.h *.c

echo TODOS FOUND:

findstr -s -n -i -l "todo" %Wildcard%
