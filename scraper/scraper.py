from sys import argv
from collections import Counter
import re

def find_keywords(article):
	words = re.findall(r'\w+', article)
	keywords = Counter([word.upper() for word in words])
	print(keywords.most_common(10))



if __name__ == '__main__':
	filename = argv[1]
	article = open(filename)
	find_keywords(article.read())
	