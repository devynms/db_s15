-- every paper in the same release as your paper, given a title
SELECT *
FROM papers
WHERE release_id = ?;

-- every paper in the same release as your paper, given the release_id
-- works given title is unique
-- if title becomes a non-unique field, change WHERE release_id = to WHERE release_id in
SELECT *
FROM papers
WHERE release_id = (
	SELECT release_id
	FROM papers
    WHERE title = ?
);

-- all the journals and conferences released by a publisher, given a publisher's name
SELECT id
FROM releases
WHERE publisher_name = ?;
-- store, then:
SELECT *
FROM journals
WHERE FIND_IN_SET(release_id, ?);
SELECT *
FROM conferences
WHERE FIND_IN_SET(release_id, ?);
-- or
SELECT *
FROM journals
WHERE release_id IN (
	SELECT id
    FROM releases
    WHERE publisher_name = ?
);
SELECT *
FROM conferences
WHERE release_id IN (
	SELECT id
    FROM releases
    WHERE publisher_name = ?
);
-- note that mysql does not support the 'with' clause

-- all the authors who are a keynote speaker for conferences by a publisher
SELECT authors.name
FROM authors
JOIN conferences
ON conferences.keynote_speaker = authors.name
JOIN releases
ON releases.id = conferences.release_id
WHERE releases.publisher_name = ?;

-- all the authors who have published at any of the same conferences as a given author
SELECT paper_authors.author_name
-- all papers published at a conference, and their authors...
FROM conferences
JOIN papers
	ON papers.release_id = conferences.release_id
JOIN paper_authors
	ON paper_authors.paper_id = papers.id
-- where that conferences is one of...
WHERE conferences.release_id IN (
	-- every release the target author has published a paper in
	SELECT papers.release_id
	FROM papers
	JOIN paper_authors as P1
		ON P1.paper_id = papers.id
		AND EXISTS (
			SELECT P2.author_name
			FROM paper_authors as P2
			WHERE P2.author_name = ?
			AND P2.paper_id = P1.paper_id
		)
);