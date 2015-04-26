from sys import argv
from collections import Counter
import re
import subprocess
import random

def find_keywords(article):
	words = re.findall(r'\w+', article)
	keywords = Counter([word.upper() for word in words])
	topics = get_topics(keywords)
	return keywords.most_common(10), topics

def get_topics(keywords):
	all_words = list(keywords.elements())
	i = 5
	topics = []
	while(i > 0):
		topics.append(all_words[random.randint(0, len(all_words)-1)])
		i-=1
	return topics

def find_author(tex_article):
	article = open(tex_article)
	#print(article.read())
	all_authors = re.search('author{(.*)}', article.read()).group()[7:-1]
	return re.split(',| and ', all_authors)

def find_citations(bibtex):
	citations = open(bibtex)
	return re.findall('title = {(.*)}', citations.read())

def create_citations():
	i = 10
	citations = []
	while i > 0:
		pipe = subprocess.Popen(["perl", "./mathgen.pl", "--product=article", "--mode=raw", "--output=./cit"])
		pipe.communicate()
		citations.append(find_author("./cit"))
		i -= 1


def find_title(tex_article):
	article = open(tex_article).read()
	title = re.search('scititle{(.*)}', article)
	if title:
		return title.group()[9:-1]
	return re.search('title{(.*)}', article).group()[9:-1]

def find_abstract(tex_article):
	article = open(tex_article)
	return re.search('begin{abstract}(.|\n)*end{abstract}', article.read()).group()[15:-15]

if __name__ == '__main__':
	i = 100
	filename = argv[1]
	citations = argv[2]
	while i > 0:
		pipe = subprocess.Popen(["perl", "./mathgen.pl", "--product=article", "--mode=raw", "--output=" + filename])
		pipe.communicate()
		paper = dict()
		paper['title'] = find_title(filename)
		paper['author'] = find_author(filename)
		paper['abstract'] = find_abstract(filename)
		paper['citations'] = create_citations()
		pipe = subprocess.Popen(["perl", "./remove_tex.pl", filename], stdout=subprocess.PIPE)
		article = pipe.communicate()[0]
		paper['keywords'], paper['topics'] = find_keywords(article.decode("utf-8"))
		print(paper)
		i -=1
	