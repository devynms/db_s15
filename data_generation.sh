#!/bin/bash 

a=0
declare -a papers
while [ $a -lt 10 ]
do
  cd scraper
	python scraper.py "./paper/test.txt" "_" > ./test.txt
	cd ..
	mv ./scraper/test.txt ./test.txt
	papers[$a] = $(cat ./test.txt)
	#python ./data_generation.py
  a=`expr $a + 1`
done
python ./data_generation.py papers