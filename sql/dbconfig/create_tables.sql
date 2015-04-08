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
    published_time	TIME,
    abstract		TEXT,
    PRIMARY KEY (id)
);

CREATE TABLE paper_authors (
	paper_id	INTEGER NOT NULL,
    author_id	INTEGER NOT NULL,
    PRIMARY KEY (paper_id, author_id)
);

CREATE TABLE paper_citations (
	citing_paper_id	INTEGER NOT NULL,
    cited_paper_id	INTEGER NOT NULL,
    PRIMARY KEY (citing_paper_id, cited_paper_id),