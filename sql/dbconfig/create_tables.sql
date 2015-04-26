CREATE TABLE topics (
	topic_text	VARCHAR(255) NOT NULL,
    PRIMARY KEY (topic_text)
);

CREATE TABLE topic_subtopics (
	parent_topic	VARCHAR(255) NOT NULL,
    subtopic		VARCHAR(255) NOT NULL,
    PRIMARY KEY (subtopic),
    FOREIGN KEY (parent_topic)
		REFERENCES topics(topic_text)
        ON DELETE RESTRICT
        ON UPDATE CASCADE,
	FOREIGN KEY (subtopic)
		REFERENCES topics(topic_text)
        ON DELETE RESTRICT
        ON UPDATE CASCADE
);

CREATE TABLE authors (
    name	VARCHAR(255) NOT NULL,
    PRIMARY KEY (name)
);

CREATE TABLE publishers (
    name	VARCHAR(255) NOT NULL,
    address	TEXT NULL,
    PRIMARY KEY (name)
);

CREATE TABLE releases (
	id              INTEGER NOT NULL AUTO_INCREMENT,
    published_date  DATE NOT NULL,
    published_time  TIME NULL,
    publisher_name  VARCHAR (255) NOT NULL,
    PRIMARY KEY (id),
    FOREIGN KEY (publisher_name)
        REFERENCES publishers(name)
        ON DELETE RESTRICT
        ON UPDATE CASCADE
);

CREATE TABLE release_topics (
	release_id	INTEGER NOT NULL,
    topic		VARCHAR(255) NOT NULL,
    PRIMARY KEY (release_id, topic),
    FOREIGN KEY (release_id)
		REFERENCES releases(id)
        ON DELETE CASCADE,
	FOREIGN KEY (topic)
		REFERENCES topics(topic_text)
        ON DELETE RESTRICT
        ON UPDATE CASCADE
);

CREATE TABLE journals (
	release_id	INTEGER NOT NULL,
    name        VARCHAR(255) NOT NULL,
    volume		INTEGER NOT NULL,
    issue		INTEGER NOT NULL,
    PRIMARY KEY (release_id),
    INDEX (name),
    UNIQUE (name),
    FOREIGN KEY (release_id)
        REFERENCES releases(id)
        ON DELETE CASCADE
        ON UPDATE RESTRICT
);

CREATE TABLE conferences (
	release_id	INTEGER NOT NULL,
    location	TEXT NOT NULL,
    keynote_speaker	VARCHAR(255) NULL,
    PRIMARY KEY (release_id),
    FOREIGN KEY (release_id)
        REFERENCES releases(id)
        ON DELETE CASCADE
        ON UPDATE RESTRICT,
	FOREIGN KEY (keynote_speaker)
        REFERENCES authors(name)
        ON DELETE SET NULL
        ON UPDATE CASCADE
);

CREATE TABLE papers (
	id				INTEGER NOT NULL AUTO_INCREMENT,
    release_id		INTEGER NOT NULL,
    title			VARCHAR(255) NOT NULL,
    abstract		TEXT,
    PRIMARY KEY (id),
    INDEX (title),
    UNIQUE(title),
	FOREIGN KEY (release_id)
		REFERENCES releases(id)
        ON DELETE RESTRICT
        ON UPDATE RESTRICT
);

CREATE TABLE paper_topics (
	paper_id	INTEGER NOT NULL,
    topic		VARCHAR(255) NOT NULL,
    PRIMARY KEY (paper_id, topic),
    FOREIGN KEY (paper_id)
		REFERENCES papers(id)
        ON DELETE CASCADE
        ON UPDATE RESTRICT,
	FOREIGN KEY (topic)
		REFERENCES topics(topic_text)
        ON DELETE RESTRICT
        ON UPDATE CASCADE
);

CREATE TABLE paper_authors (
	paper_id	INTEGER NOT NULL,
    author_name	VARCHAR(255) NOT NULL,
    PRIMARY KEY (paper_id, author_name),
    FOREIGN KEY (paper_id)
        REFERENCES papers(id)
        ON DELETE CASCADE
        ON UPDATE RESTRICT,
	FOREIGN KEY (author_name)
        REFERENCES authors(name)
        ON DELETE RESTRICT
        ON UPDATE CASCADE
);

CREATE TABLE paper_citations (
	citing_paper_id    INTEGER NOT NULL,
    cited_paper_title  VARCHAR (255) NOT NULL,
    PRIMARY KEY (citing_paper_id, cited_paper_id),
    FOREIGN KEY (citing_paper_id)
        REFERENCES papers(id)
        ON DELETE RESTRICT
        ON UPDATE RESTRICT
);

CREATE TABLE keyphrases (
	phrase	VARCHAR(255) NOT NULL,
    PRIMARY KEY (phrase)
);

CREATE TABLE paper_keyphrases (
	paper_id	INTEGER NOT NULL,
    keyphrase	VARCHAR(255) NOT NULL,
    count		INTEGER NOT NULL,
    PRIMARY KEY (paper_id, keyphrase),
    FOREIGN KEY (paper_id)
        REFERENCES papers(id)
        ON DELETE CASCADE
        ON UPDATE RESTRICT,
	FOREIGN KEY (keyphrase)
        REFERENCES keyphrases(phrase)
        ON DELETE RESTRICT
        ON UPDATE CASCADE
);