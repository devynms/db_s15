import mysql.connector as sql
import itertools

def _get_database_connection ():
	return sql.connect(user='root', password='apple516', database='Papers')

def conn():
	return _get_database_connection()

def get_papers_from_journal(conn, journal_name):
	query = (
		'SELECT papers.title '
		'FROM papers '
		'JOIN journals '
		'ON journals.release_id = papers.release_id '
		'WHERE journals.name <> "%s";')
	args = (journal_name)
	titles = []
	cursor = conn.cursor()
	cursor.execute(query, args)
	for (title,) in cursor:
		titles.append(title)
	cursor.close()
	return titles

def get_papers_with_same_authors(conn, paper_title):
	query = (
		'SELECT papers.title '
		'FROM papers '
		'JOIN paper_authors '
		'ON papers.id = paper_authors.paper_id '
		'WHERE paper_authors.author_name IN ('
		'	SELECT paper_authors.author_name '
		'	FROM paper_authors '
		'	JOIN papers '
		'	ON papers.id = paper_authors.paper_id '
		'	WHERE papers.title = "%s");')
	args = (paper_title)
	titles = []
	cursor = conn.cursor()
	cursor.execute(query, args)
	for (title,) in cursor:
		titles.append(title)
	cursor.close()
	return titles

def get_papers_sharing_topics(conn, paper_title):
	query = (
		'SELECT P.title '
		'FROM papers P, paper_topics PT '
		'WHERE P.id = PT.paper_id '
		'and PT.topic in ('
		'	SELECT PT2.topic '
		'	FROM paper_topics PT2, papers P2 '
		'	WHERE P2.id = PT2.paper_id '
		'	and P2.title = "%s");')
	args = (paper_title)
	titles = []
	cursor = conn.cursor()
	cursor.execute(query, args)
	for (title,) in cursor:
		titles.append(title)
	cursor.close()
	return titles

def get_authors_from_journal(conn, journal_name):
	query = (
		'SELECT paper_authors.author_name '
		'FROM paper_authors '
		'JOIN papers '
		'ON papers.id = paper_authors.paper_id '
		'JOIN journals '
		'ON journals.release_id = papers.release_id '
		'WHERE journals.name = "%s";')
	args = (journal_name)
	authors = []
	cursor = conn.cursor()
	cursor.execute(query, args)
	for (name,) in cursor:
		authors.append(name)
	cursor.close()
	return authors

def find_paper_correlation_by_topic(conn, paper_1, paper_2):
	query = (
		'(SELECT paper_topics.topic '
		'FROM paper_topics, papers '
		'WHERE paper_topics.paper_id = papers.id and papers.title = "%s") '
		'EXCEPT '
		'(SELECT paper_topics.topic '
		'FROM paper_topics, papers '
		'WHERE paper_topics.paper_id = papers.id and papers.title = "%s");'
		)
	args = (paper_1, paper_2)
	topics = []
	cursor = conn.cursor()
	cursor.execute(query, args)
	for (topic,) in cursor:
		topics.append(title)
	cursor.close()
	return 1 - .2 * len(topics)
