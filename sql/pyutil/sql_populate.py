import mysql.connector as sql

class AcademicPaper:
	# title: a string
	# published date: the date the paper was published. format TBD
	# kephrases: a map from keyphrase (string) to a count of the number of times it appears
	# authors: a list of author names (strings)
	# publisher: the name of the publisher, if there is one
	# published_time: the time the article was publish, or None if unkown. format TBD
	# abstract: the body of the abstract, as a plaintext string
	def __init__(self, title, published_date, keyphrases, authors = [], publisher = None,
		published_time = None, abstract = None):
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
	pass