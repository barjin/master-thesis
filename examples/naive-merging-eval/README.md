# Naïve merging evaluation scripts

To evaluate the performance of the naïve merging algorithm, we turn the merging problem into a classification problem (more about this in the thesis).

For calculating the macro / micro averaged F1 score, we use the following SQL script on the source database:

```sql
/**
  * Create a table with all the internal authors and their normalized names.
  * Note that `PUBLICATION_AUTHOR_ALL` is a relational table joining authors to their publications 
  *    (`GROUP BY` is used to remove duplicates).
  */
CREATE TEMP TABLE if not exists AUTHORS as 
	SELECT PERSON_ID, PERSON_NAME, normalize_name(PERSON_NAME) as NORM 
from PUBLICATION_AUTHOR_ALL where EXTERNAL = 0 GROUP BY PERSON_ID;

/**
  * We run the "classifier" on the `AUTHORS` table (subquery `MERGED`). 
  *   The resulting table contains only the nodes with the MIN(PERSON_ID) - and the count of the nodes with the same normalized name.
  * We join this with the table containing all the nodes and calculate TP, FP and FN.
  * We also calculate the precision and recall for each node - in the "classifier" context, each node is a class.
  *   Note that a perfect classifier would map each node to a single class with the node's ID.
  *   This would be only possible in the case of no normalized names collisions (i.e. normalized names are unique identifiers).
  */
CREATE TABLE IF NOT EXISTS CONFUSION_MATRIX as
SELECT 
	AUTHORS.PERSON_ID as PERSON_ID, 
	AUTHORS.PERSON_NAME AS PERSON_NAME, 
	AUTHORS.NORM as NORM, 
	MIN(coalesce(C, 0), 1) AS TP,
	MAX(0, coalesce(C, 0) - 1) AS FP,
	ABS(1 - MIN(coalesce(C, 0), 1)) AS FN,
	CAST(MIN(coalesce(C, 0), 1) as real) / (MIN(coalesce(C, 0), 1) + MAX(0, coalesce(C, 0) - 1)) AS PRECISION,
	CAST(MIN(coalesce(C, 0), 1) as real) / (MIN(coalesce(C, 0), 1) + ABS(1 - MIN(coalesce(C, 0), 1))) AS RECALL
FROM AUTHORS
	LEFT JOIN 
(SELECT *, count(PERSON_ID) as C FROM AUTHORS GROUP BY NORM) AS MERGED
	USING(PERSON_ID);

/**
  * Calculate the macro averaged F1 score.
  *   The F1 score is calculated for each class and then averaged.
  *   The F1 score is calculated as 2 * (precision * recall) / (precision + recall).
  */
SELECT AVG(F1_PER_CLASS) as F1_MACRO 
	FROM (
		SELECT COALESCE(2*(PRECISION * RECALL)/(PRECISION + RECALL), 0.0) as F1_PER_CLASS 
			FROM
		CONFUSION_MATRIX
	) 
	GROUP BY NULL;

/**
  * Calculate the micro averaged F1 score.
  */
SELECT CAST(SUM(TP) as REAL) / COUNT(*) AS F1_MICRO FROM CONFUSION_MATRIX;
```