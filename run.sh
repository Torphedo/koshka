#!/bin/sh

if [ -z $1 ]
then
    ./koshka
else
    ./koshka $1
fi

