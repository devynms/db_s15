cd scraper
python scraper.py "./paper/test.txt" "_" > ./test.txt
cd ..
mv ./scraper/test.txt ./test.txt
python ./data_generation.py