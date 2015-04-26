import mysql.connector as sql

def _get_database_connection ():
	return sql.connect(user='root', password='password', database='db_s15')

# name: a string of less than 255 chars
# address: an optional string
class Publisher:
	def __init__(self, name, address = None):
		self.name = name
		self.address = address

# publisher: an object of the Publisher type
# pubdate: a string in the date format that SQL will expect on insert
# pubtime: a string in the time format that SQL will expect on insert
# topics: an array of primary topics
class Release:
	def __init__(self, publisher, pubdate, pubtime = None, topics = []):
		self.publisher = publisher
		self.pubdate = pubdate
		self.pubtime = pubtime
		self.topics = topics
		self.subtopics = subtopics

# See Release class.
# volume: an integer for volume
# issue: an integer for issue
class Journal(Release):
	def __init__(self, volume, issue, publisher, pubdate, pubtime = None, topics = []):
		super(Journal, self).__init__(publisher, pubdate, pubtime, topics, subtopics)
		self.volume = volume
		self.issue = issue

# See Release class.
# location: a string representing the location of the conference
# speaker: an optional string of 255 chars or less representing the speaker's name
class Conference(Release):
	def __init__(self, location, publisher, pubdate, speaker = None, pubtime = None, topics = []):
		super(Conference, self).__init__(publisher, pubdate, pubtime, topics, subtopics)
		self.location = location
		self.speaker = speaker

class AcademicPaper:

	next_id = 0

	# title: a string < 255 chars
	# kephrases: a map from keyphrase (string) to a count of the number of times it appears
	# release: a Release object representing the format the paper was released in
	# topics: an array of topics as strings < 255 chars
	# authors: a list of author names as strings < 255 chars
	# citations: a list of paper titles
	# abstract: an optional string for the abstract text
	def __init__(self, title, release, keyphrases, topics = [], authors = [], citations = [], abstract = None):
		self.id = next_id
		next_id += 1
		self.title = title
		self.keyphrases = keyphrases
		self.release = release
		self.topics = topics
		self.authors = authors
		self.citations = citations
		self.abstract = abstract

# convert academic papers to a series of data structures that correspond to rows
# in all the tables of the database schma being used.
# see /sql/dbconfig/create_tables.sql for the schema
# academic papers: a list of AcademicPaper objects
# subtopics: a map from topic -> subtopic (both are strings < 255 chars)
def sql_structures_from_papers (academic_papers, subtopics):
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

# take the sql structure (returned by sql_structures_from_papers, presumably) and
# publish it to the database connected to by sql_con
def publish_sql_structures (sql_con, sql_structures):
	pass