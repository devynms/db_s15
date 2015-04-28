import mysql.connector as sql
import itertools

def _get_database_connection ():
	return sql.connect(user='root', password='password', database='db_s15')

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
		'	WHERE papers.title <> "%s");')
	args = (paper_title)
	titles = []
	cursor = conn.cursor()
	cursor.execute(query, args)
	for (title,) in cursor:
		titles.append(title)
	cursor.close()
	return titles

def find_paper_correlation_by_topic(conn, paper_1, paper_2):
	query = (
		'SELECT paper_topics.topic'
		'FROM paper_topics, papers'
		'WHERE paper_topics.paper_id = papers.id and papers.title = %s'
		'EXCEPT'
		'SELECT paper_topics.topic'
		'FROM paper_topics, papers'
		'WHERE paper_topics.paper_id = papers.id and papers.title = %s;'
		)
	args = (paper_1, paper_2)
	topics = []
	cursor = conn.cursor()
	cursor.execute(query, args)
	for (topic,) in cursor:
		topics.append(title)
	cursor.close()
	return 1 - .2 * len(topics)
