CREATE TABLE authors (
    name	VARCHAR(255) NOT NULL,
    PRIMARY KEY (name)
);

CREATE TABLE publishers (
    name	VARCHAR(255) NOT NULL,
    PRIMARY KEY (name)
);

CREATE TABLE papers (
	id				INTEGER NOT NULL AUTO_INCREMENT,
    title			VARCHAR(255) NOT NULL,
    published_date	DATE NOT NULL,
    publisher_name	VARCHAR (255) NOT NULL,
    published_time	TIME,
    abstract		TEXT,
    PRIMARY KEY (id),
    CONSTRAINT fk_papers_publisher
		FOREIGN KEY (publisher_name)
        REFERENCES publishers(name)
        ON DELETE RESTRICT
);

CREATE TABLE paper_authors (
	paper_id	INTEGER NOT NULL,
    author_name	VARCHAR(255) NOT NULL,
    PRIMARY KEY (paper_id, author_name),
    CONSTRAINT fk_authors_paper
		FOREIGN KEY (paper_id)
        REFERENCES papers(id)
        ON DELETE CASCADE,
	CONSTRAINT fk_papers_author
		FOREIGN KEY (author_name)
        REFERENCES authors(name)
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
	phrase	VARCHAR(255) NOT NULL,
    PRIMARY KEY (phrase)
);

CREATE TABLE paper_keyphrases (
	paper_id	INTEGER NOT NULL,
    keyphrase	VARCHAR(255) NOT NULL,
    count		INTEGER NOT NULL,
    PRIMARY KEY (paper_id, keyphrase),
    CONSTRAINT fk_keyphrases_paper
		FOREIGN KEY (paper_id)
        REFERENCES papers(id)
        ON DELETE CASCADE,
	CONSTRAINT fk_papers_keyphrase
		FOREIGN KEY (keyphrase)
        REFERENCES keyphrases(phrase)
        ON DELETE RESTRICT
);