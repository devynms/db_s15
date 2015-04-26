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
class Release(object):
	next_id = 0

	def __init__(self, publisher, pubdate, pubtime = None, topics = []):
		self.id = Release.next_id
		Release.next_id += 1
		self.publisher = publisher
		self.pubdate = pubdate
		self.pubtime = pubtime
		self.topics = topics
		self.subtopics = subtopics

# See Release class.
# volume: an integer for volume
# issue: an integer for issue
class Journal(Release):
	def __init__(self, name, volume, issue, publisher, pubdate, pubtime = None, topics = []):
		super(Journal, self).__init__(publisher, pubdate, pubtime, topics, subtopics)
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

	AcademicPaper.next_id = 0

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

# convert academic papers to a series of data structures that correspond to rows
# in all the tables of the database schma being used.
# see /sql/dbconfig/create_tables.sql for the schema
# academic papers: a list of AcademicPaper objects
# subtopics: a map from topic -> subtopic (both are strings < 255 chars)
def sql_structures_from_papers (academic_papers, subtopics):
	struct = {}
	struct['topics'] = Set()		# a set of topic strings
	struct['topic_subtopics'] = subtopics	# map from subtopic to topic
	struct['authors'] = Set()		# a set of author names
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
		for paper_topic in paper.topics
			struct['topics'].add(paper_topic)
			struct['paper_topics'].add((paper.id, paper_topic))
		# populate authors
		for paper_author in paper.authors
			struct['authors'].add(paper_author)
		# populate publishers
		pub = paper.release.publisher
		if pub.name in struct['publishers']:
			if struct['publishers'][pub.name] = None:
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
		struct['releases'] = (release.pubdate, release.pubtime, release.publisher)
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
			struct['paper_topics'].add((paper.id, paper_topc))
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


# take the sql structure (returned by sql_structures_from_papers, presumably) and
# publish it to the database connected to by sql_con
def publish_sql_structures (sql_con, sql_structures):
	pass