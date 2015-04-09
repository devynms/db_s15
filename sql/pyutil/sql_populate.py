import mysql.connector as sql

def _get_database_connection ():
	return sql.connect(user='root', password='password', database='db_s15')

class AcademicPaper:

	next_id = 0

	# title: a string
	# published date: the date the paper was published. format TBD
	# kephrases: a map from keyphrase (string) to a count of the number of times it appears
	# authors: a list of author names (strings)
	# publisher: the name of the publisher, if there is one
	# published_time: the time the article was publish, or None if unkown. format TBD
	# abstract: the body of the abstract, as a plaintext string
	def __init__(self, title, published_date, keyphrases, authors = [], publisher = None,
		published_time = None, abstract = None):
		self.id = next_id
		next_id += 1
		self.title = title
		self.keyphrases = keyphrases
		self.published_date = published_date
		self.authors = authors
		self.publisher = publisher
		self.published_time = published_time
		self.abstract = abstract

# convert academic papers to a series of data structures that correspond to rows
# in all the tables of the database schma being used.
# see /sql/dbconfig/create_tables.sql for the schema
def sql_structures_from_papers (academic_papers):
	struct = {}
	struct['authors'] = Set()		# set of author names
	struct['publishers'] = Set()	# set of publisher names
	struct['papers'] = {}			# id to {title, date, time, publisher, abstract}
	struct['keyphrases'] = Set()	# set of keyphrases found
	struct['paper_authors'] = Set()	# set of (paper_id, author_name)
	struct['paper_keyphrases'] = {}	# (paper_id, phrase) to count

	for paper in academic_papers:
		# populate papers
		if paper.id in struct['papers']: return None
		struct['papers'] = {
			'title': paper.title,
			'published_date': paper.published_date,
			'published_time': paper.published_time,
			'publisher_name': paper.publisher,
			'abstract'		: paper.abstract
		}

		# populate publishers
		struct['publishers'] = paper.publisher

		# populate authors, and paper_authors
		for author in paper.authors:
			struct['authors'].add(author)
			struct['paper_authors'].add((paper.id, author))

		# populate keyphrases, and paper_keyphrases
		for (phrase, count) in paper.keyphrases.iteritems():
			struct['keyphrases'].add(phrase)
			if (paper.id, phrase) in struct['paper_keyphrases']: return None
			struct['paper_keyphrases'][(paper.id, phrase)] = count

	return struct


def publish_sql_structures (sql_structures):
	pass