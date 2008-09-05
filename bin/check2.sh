#!/bin/bash

n=$1
shift
echo $1
gringo2 -l $* | clasp -n $n | grep -A 1 '^Answer: [0-9]*$' | grep -v '^Answer: [0-9]*$' > answers.txt
l=$(wc -l < answers.txt)
echo "found $l solutions"
if [[ $l == 0 ]]; then
	if lparse $* | clasp | grep "Models      : 0"; then
		rm answers.txt
		echo True
		exit 0
	else
		rm answers.txt
		echo False
		exit 1
	fi
else
	lparse $* > compare.sm
	g=$(cat answers.txt | awk 'BEGIN { while((getline str) > 0) { gsub(/\(/, "\\(", str); gsub(/\)/, "\\)", str); gsub(/\"/, "\\\"", str); system("testsm compare.sm " str " | smodels 1"); } }' | grep "True" | wc -l)
	if [[ $l == $g ]]; then
		rm answers.txt
		rm compare.sm
		echo True
		exit 0
	else
		rm answers.txt
		rm compare.sm
		echo False
		exit 1
	fi
fi
	
