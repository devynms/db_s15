import mysql.connector as sql
import itertools

def _get_database_connection ():
	return sql.connect(user='root', password='apple516'	, database='Papers')

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
class Release(object):
	next_id = 0

	def __init__(self, publisher, pubdate, pubtime = None, topics = []):
		self.id = Release.next_id
		Release.next_id += 1
		self.publisher = publisher
		self.pubdate = pubdate
		self.pubtime = pubtime
		self.topics = topics
		#self.subtopics = subtopics

# See Release class.
# volume: an integer for volume
# issue: an integer for issue
class Journal(Release):
	def __init__(self, name, volume, issue, publisher, pubdate, pubtime = None, topics = []):
		super(Journal, self).__init__(publisher, pubdate, pubtime, topics)
		self.name = name
		self.volume = volume
		self.issue = issue

# See Release class.
# location: a string representing the location of the conference
# speaker: an optional string of 255 chars or less representing the speaker's name
class Conference(Release):
	def __init__(self, name, location, publisher, pubdate, speaker = None, pubtime = None, topics = []):
		super(Conference, self).__init__(publisher, pubdate, pubtime, topics, subtopics)
		self.name = name
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
		self.id = AcademicPaper.next_id
		AcademicPaper.next_id += 1
		self.title = title
		self.keyphrases = keyphrases
		self.release = release
		self.topics = topics
		self.authors = authors
		self.citations = citations
		self.abstract = abstract

def publish_papers_to_dbms(academic_papers, subtopics):
	struct = sql_structures_from_papers(academic_papers, subtopics)
	sql_con = _get_database_connection()
	publish_sql_structures(sql_con, struct)
	sql_con.close()

# convert academic papers to a series of data structures that correspond to rows
# in all the tables of the database schma being used.
# see /sql/dbconfig/create_tables.sql for the schema
# academic papers: a list of AcademicPaper objects
# subtopics: a map from topic -> subtopic (both are strings < 255 chars)
def sql_structures_from_papers (academic_papers, subtopics):
	struct = {}
	struct['topics'] = set()		# a set of topic strings
	struct['topic_subtopics'] = subtopics	# map from subtopic to topic
	struct['authors'] = set()		# a set of author names
	struct['publishers'] = {}		# a map name to optional address
	struct['releases'] = {}			# a map id to (pubdate, pubtime?, pubname)
	struct['release_topics'] = set()# a set of (release id, topic string)
	struct['journals'] = {}			# a map from release id to (name, volume, issue)
									# should check release id exists, name unique among journals
	struct['conferences'] = {}		# a map from release id to (name, location, speaker)
									# should check release id exists, name unique among conferences
	struct['papers'] = {}			# a map from id to (release id, title, abstract)
									# should check that release id exists
	struct['paper_topics'] = set()	# a set of (paper id, topic string)
									# should check that paper id and topic exist
	struct['paper_authors'] = set()	# a set of (paper id, author name) (string<255)
									# should check that paper id and author id exists
	struct['paper_citations'] = set()	# a set of (paper id, paper title)
	struct['keyphrases'] = set()	# a set of keyphrases (string<255)
	struct['paper_keyphrases'] = set()	# a set of (paper id, keyphrase string<255)

	for paper in academic_papers:
		# populate topics, paper_topics
		for paper_topic in paper.topics:
			struct['topics'].add(paper_topic)
			struct['paper_topics'].add((paper.id, paper_topic))
		# populate authors
		for paper_author in paper.authors:
			struct['authors'].add(paper_author)
		# populate publishers
		pub = paper.release.publisher
		if pub.name in struct['publishers']:
			if struct['publishers'][pub.name] == None:
				struct['publishers'][pub.name] = pub.address
			elif struct['publishers'][pub.name] != pub.address:
				raise
		else:
			struct['publishers'][pub.name] = pub.address
		# populate releases
		release = paper.release
		if release.id in struct['releases']:
			if (release.pubdate, release.pubtime, release.publisher) != struct['releases'][release.id]:
				raise
		struct['releases'][release.id] = (release.pubdate, release.pubtime, release.publisher)
		#		journals
		if type(release) is Journal:
			if release.id in struct['journals'] and (release.name, release.volume, release.issue) != struct['journals'][release.id]:
				raise
			else:
				struct['journals'][release.id] = (release.name, release.volume, release.issue)
		#		conferences
		elif type(release) is Conference:
			if release.id in struct['conferences'] and (release.name, release.location, release.speaker) != struct['conferences'][release.id]:
				raise
			else:
				struct['conferences'][release.id] = (release.name, release.location, release.speaker)
		else:
			raise
		#		topics
		for release_topc in release.topics:
			struct['release_topcs'].add((release.id, release_topic))
		# populate papers
		struct['papers'][paper.id] = (paper.release.id, paper.title, paper.abstract)
		#		topics
		for paper_topic in paper.topics:
			struct['paper_topics'].add((paper.id, paper_topic))
		#		authors
		for paper_author in paper.authors:
			struct['paper_authors'].add((paper.id, paper_author))
		#		citations
		for paper_citation in paper.citations:
			struct['paper_citations'].add((paper.id, paper_citation))

		# keywords
		for keyphrase in paper.keyphrases:
			struct['keyphrases'].add(keyphrase)
			struct['paper_keyphrases'].add((paper.id, keyphrase))
	return struct


# take the sql structure (returned by sql_structures_from_papers, presumably) and
# publish it to the database connected to by sql_con
def publish_sql_structures (sql_con, sql_structures):
	topics_query = "INSERT INTO topics VALUES "
	topic_subtopics_query = "INSERT INcountcountcountTO topic_subtopics VALUES "
	authors_query = "INSERT INTO authors VALUES "
	publishers_query = "INSERT INTO publishers VALUES "
	releases_query = "INSERT INTO releases VALUES "
	release_topics_query = "INSERT INTO release_topics VALUES "
	journals_query = "INSERT INTO journals VALUES "
	conferences_query = "INSERT INTO conferences VALUES "
	papers_query = "INSERT INTO papers VALUES "
	paper_topics_query = "INSERT INTO paper_topics VALUES "
	paper_authors_query = "INSERT INTO paper_authors VALUES "
	paper_citations_query = "INSERT INTO paper_citations VALUES "
	keyphrases_query = "INSERT INTO keyphrases VALUES "
	paper_keyphrases_query = "INSERT INTO paper_keyphrases VALUES "

	# topics
	for _ in range(len(sql_structures['topics']) - 1):
		topics_query += "(%s),"
	topics_query += "(%s);"
	topics_tuple = tuple(sql_structures['topics'])

	# topics_subtopics
	# TODO

	# authors
	for _ in range(len(sql_structures['authors']) - 1):
		authors_query += "(%s),"
	authors_query += "(%s);"
	authors_tuple = tuple(sql_structures['authors'])

	# publishers (name -> address)
	publishers_list = []
	for (name, addr) in sql_structures['publishers'].items():
		publishers_list.add(name) # add name
		if addr:
			publishers_list.add(name)
		else:
			publishers_list.add("NULL")
	publishers_tuple = tuple(publishers_list)
	for _ in range(len(sql_structures['publishers'].items()) - 1):
		publishers_query += "(%s, %s),"
	publishers_query += "(%s, %s);"

	# releases (id -> (pubdate, pubtime? pubname))
	releases_list = []
	print(sql_structures['releases'])
	for rid, attrs in sql_structures['releases'].items():
		releases_list.append(rid)
		releases_list.append(attrs[0])
		if attrs[1]:
			releases_list.append(attrs[1])
		else:
			releases_list.append("NULL")
		releases_list.append(attrs[2])
	releases_tuple = tuple(releases_list)
	for _ in range(len(sql_structures['releases'].items()) - 1):
		releases_query += "(%s, %s, %s, %s),"
	releases_query += "(%s, %s, %s, %s);"

	# release_topics (release_id, topics_string)
	for _ in range(len(sql_structures['release_topics']) - 1):
		release_topics_query += "(%s, %s),"
	release_topics_query += "(%s, %s);"
	release_topics_list = []
	for attrs in sql_structures['release_topics']:
		release_topics_list.add(attrs[0])
		release_topics_list.add(attrs[1])
	release_topics_tuple = tuple(release_topics_list)

	# journals (id -> (name, volume, issue))
	journals_list = []
	for (rid, attrs) in sql_structures['journals'].items():
		releases_list.append(rid)
		releases_list.append(attrs[0])
		releases_list.append(attrs[1])
		releases_list.append(attrs[2])
	journals_tuple = tuple(journals_list)
	for _ in range(len(sql_structures['journals'].items()) - 1):
		journals_query += "(%s, %s, %s, %s),"
	journals_query += "(%s, %s, %s, %s);"

	# conferences (id -> (name, loc, speaker))
	conferences_list = []
	for (rid, attrs) in sql_structures['conferences'].items():
		releases_list.append(rid)
		releases_list.append(attrs[0])
		releases_list.append(attrs[1])
		releases_list.append(attrs[2])
	conferences_tuple = tuple(conferences_list)
	for _ in range(len(sql_structures['conferences'].items()) - 1):
		conferneces_query = "(%s, %s, %s, %s)"
	conferences_query += "(%s, %s, %s, %s);"

	# papers id to (release id, title, abstract)
	papers_list = []
	for (pid, attrs) in sql_structures['papers'].items():
		papers_list.append(pid)
		papers_list.append(attrs[0])
		papers_list.append(attrs[1])
		if attrs[2]:
			papers_list.append(attrs[2])
		else:
			papers_list.append("NULL")
	papers_tuple = tuple(papers_list)
	for _ in range(len(sql_structures['papers'].items()) - 1):
		papers_query += "(%s, %s, %s, %s),"
	papers_query += "(%s, %s, %s, %s);"

	# paper_topics (paper id, topic string)
	for _ in range(len(sql_structures['paper_topics']) - 1):
		paper_topics_query += "(%s, %s),"
	paper_topics_query += "(%s, %s);"
	paper_topics_list = []
	for attrs in sql_structures['paper_topics']:
		paper_topics_list.append(attrs[0])
		paper_topics_list.append(attrs[1])
	paper_topics_tuple = tuple(paper_topics_list)

	# paper_authors (paper id, author string)
	for _ in range(len(sql_structures['paper_authors']) - 1):
		paper_authors_query += "(%s, %s),"
	paper_authors_query += "(%s, %s);"
	paper_authors_list = []
	for attrs in sql_structures['paper_authors']:
		paper_authors_list.append(attrs[0])
		paper_authors_list.append(attrs[1])
	paper_authors_tuple = tuple(paper_authors_list)

	# paper_citations (paper id, citations string)
	for _ in range(len(sql_structures['paper_citations']) - 1):
		paper_citations_query += "(%s, %s),"
	paper_citations_query += "(%s, %s);"
	paper_citations_list = []
	for attrs in sql_structures['paper_citations']:
		paper_citations_list.append(attrs[0])
		paper_citations_list.append(attrs[1])
	paper_citations_tuple = tuple(paper_citations_list)

	# keyphrases
	for _ in range(len(sql_structures['keyphrases']) - 1):
		keyphrases_query += "(%s),"
	keyphrases_query += "(%s);"
	keyphrases_tuple = tuple(sql_structures['keyphrases'])

	# paper_keyphrases (paper id, citations string)
	for _ in range(len(sql_structures['paper_keyphrases']) - 1):
		paper_keyphrases_query += "(%s, %s),"
	paper_keyphrases_query += "(%s, %s);"
	paper_keyphrases_list = []
	for attrs in sql_structures['paper_keyphrases']:
		paper_keyphrases_list.append(attrs[0])
		paper_keyphrases_list.append(attrs[1])
	paper_keyphrases_tuple = tuple(paper_keyphrases_list)

	# Perform Insertions
	cursor = sql_con.cursor()
	cursor.execute(topics_query, topics_tuple)
	cursor.execute(authors_query, authors_tuple)
	cursor.execute(publishers_query, publishers_tuple)
	cursor.execute(releases_query, releases_tuple)
	cursor.execute(release_topics_query, release_topics_tuple)
	cursor.execute(journals_query, journals_tuple)
	cursor.execute(conferences_query, conferences_tuple)
	cursor.execute(papers_query, papers_tuple)
	cursor.execute(paper_topics_query, paper_topics_tuple)
	cursor.execute(paper_authors_query, paper_authors_tuple)
	cursor.execute(paper_citations_query, paper_citations_tuple)
	cursor.execute(keyphrases_query, keyphrases_tuple)
	cursor.execute(paper_keyphrases_query, paper_keyphrases_tuple)
	sql_con.commit()
	cursor.close()
