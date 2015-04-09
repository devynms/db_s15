CREATE TABLE authors (
	id		INTEGER NOT NULL AUTO_INCREMENT,
    name	VARCHAR(255) NOT NULL,
    PRIMARY KEY (id)
);

CREATE TABLE publishers (
	id		INTEGER NOT NULL AUTO_INCREMENT,
    name	VARCHAR(255) NOT NULL,
    PRIMARY KEY (id)
);

CREATE TABLE papers (
	id				INTEGER NOT NULL AUTO_INCREMENT,
    title			VARCHAR(255) NOT NULL,
    published_date	DATE NOT NULL,
    publisher_id	INTEGER NULL,
    published_time	TIME,
    abstract		TEXT,
    PRIMARY KEY (id),
    CONSTRAINT fk_papers_publisher
		FOREIGN KEY (publisher_id)
        REFERENCES publishers(id)
        ON DELETE RESTRICT
);

CREATE TABLE paper_authors (
	paper_id	INTEGER NOT NULL,
    author_id	INTEGER NOT NULL,
    PRIMARY KEY (paper_id, author_id),
    CONSTRAINT fk_authors_paper
		FOREIGN KEY (paper_id)
        REFERENCES papers(id)
        ON DELETE CASCADE,
	CONSTRAINT fk_papers_author
		FOREIGN KEY (author_id)
        REFERENCES authors(id)
        ON DELETE RESTRICT
);

CREATE TABLE paper_citations (
	citing_paper_id	INTEGER NOT NULL,
    cited_paper_id	INTEGER NOT NULL,
    citation_text	TEXT NOT NULL,
    PRIMARY KEY (citing_paper_id, cited_paper_id),
    CONSTRAINT fk_citing_paper
		FOREIGN KEY (citing_paper_id)
        REFERENCES papers(id)
        ON DELETE RESTRICT,
	INDEX idx_cited_paper (cited_paper_id),
	CONSTRAINT fk_cited_paper
		FOREIGN KEY (cited_paper_id)
        REFERENCES papers(id)
        ON DELETE RESTRICT
);

CREATE TABLE keyphrases (
	id		INTEGER NOT NULL AUTO_INCREMENT,
	phrase	VARCHAR(255) NOT NULL,
    PRIMARY KEY (id),
    INDEX idx_phrase (phrase),
    UNIQUE (phrase)
);

CREATE TABLE paper_keyphrases (
	paper_id		INTEGER NOT NULL,
    keyphrase_id	INTEGER NOT NULL,
    count			INTEGER NOT NULL,
    PRIMARY KEY (paper_id, keyphrase_id),
    CONSTRAINT fk_keyphrases_paper
		FOREIGN KEY (paper_id)
        REFERENCES papers(id)
        ON DELETE CASCADE,
	CONSTRAINT fk_papers_keyphrase
		FOREIGN KEY (keyphrase_id)
        REFERENCES keyphrases(id)
        ON DELETE RESTRICT
);