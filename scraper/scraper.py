from sys import argv
from collections import Counter
import re
import subprocess

def find_keywords(article):
	words = re.findall(r'\w+', article)
	keywords = Counter([word.upper() for word in words])
	print(keywords.most_common(10))

def find_author(tex_article):
	pass

def find_citations(tex_article):
	pass

if __name__ == '__main__':
	filename = argv[1]
	pipe = subprocess.Popen(["perl", "./remove_tex.pl", filename], stdout=subprocess.PIPE)
	article = pipe.communicate()[0]
	find_keywords(article.decode("utf-8"))
	