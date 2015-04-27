#!/bin/bash 

a=0
declare -a papers
while [ $a -lt 10 ]
do
  cd scraper
	python scraper.py "./paper/test.txt" "_" > ./test.txt
	cd ..
	mv ./scraper/test.txt ./test.txt
	if [ $a -lt 9 ]
	then
		papers[$a]="$(cat ./test.txt),"
	else
		papers[$a]="$(cat ./test.txt)"
	fi
	#python ./data_generation.py
  a=`expr $a + 1`
done
echo '[' > ./test.txt
echo ${papers[@]} >> ./test.txt
echo ']' >> ./test.txt
python ./data_generation.py