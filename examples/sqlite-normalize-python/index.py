#  2024-05-16

#  Jindřich Bär, https://jindrich.bar
#    | https://github.com/barjin/master-thesis

# ****************************************************************************

#  This is a one-use Python script for normalizing the names of academic researchers.
#  For more context, see the thesis "Social network analysis in academic environment"
#   at https://jindrich.bar/master-thesis/bar-social-network-analysis-in-academic-environment-2024.pdf


import sqlite3
import re


def create_table():
    """
    Creates the output `TEST` table with the same schema as the `PUBLICATION_AUTHOR_ALL` table.
    """
    sql_statements = [
        """CREATE TABLE IF NOT EXISTS TEST AS SELECT * FROM PUBLICATION_AUTHOR_ALL LIMIT 0;""",
        """ALTER TABLE TEST ADD COLUMN NORMALIZED_NAME TEXT;""",
    ]

    try:
        with sqlite3.connect("explorer.db") as conn:
            for statement in sql_statements:
                conn.execute(statement)

            conn.commit()
    except sqlite3.Error as e:
        print(e)


dia = "ěščřžýáíéúůďťňó"
dia = dia + dia.upper()
trans = str.maketrans(dia, "escrzyaieuudtno" + "ESCRZYAIEUUDTNO")
alpha_regex = re.compile("[^\w\s]+", re.UNICODE)
titles_before_name_regex = re.compile(
    "^((prof|doc|rndr|mudr|ing|mgr|bc|phdr|judr|msc|phd|csc|bba|mba|bsc|msc|ms|phil|et|assoc|dr) *)*"
)
titles_after_name_regex = re.compile(" ((phd|csc|dr|drsc|mag|phil) *)+")


def normalize_name(name: str):
    """
    Normalizes the name by removing diacritics, non-alphanumeric characters and academic titles before and after the name.

    Args:
        name (str): The name to normalize.
    """
    name = " ".join(alpha_regex.sub(" ", name.translate(trans).lower().strip()).split())
    name = titles_before_name_regex.sub("", name)
    name = titles_after_name_regex.sub("", name)

    return name.strip()


def read_from_publications_and_normalize():
    """
    Reads the `PUBLICATION_AUTHOR_ALL` table in batches, normalizes the `PERSON_NAME` column and writes the normalized name to the `TEST` table.
    """
    READ_BATCH_SIZE = 1000
    WRITE_BATCH_SIZE = 500

    try:
        with sqlite3.connect("explorer.db") as conn:
            cursor = conn.cursor()
            cursor.execute(
                "SELECT PUBLICATION_ID, PERSON_ID, RANK, PERSON_NAME, EXTERNAL FROM PUBLICATION_AUTHOR_ALL"
            )
            rows = cursor.fetchmany(READ_BATCH_SIZE)

            while rows:
                write_batch = []
                for row in rows:
                    normalized_name = normalize_name(row[3])

                    write_batch.append((normalized_name, *row))

                    if len(write_batch) == WRITE_BATCH_SIZE:
                        conn.executemany(
                            "INSERT INTO TEST (NORMALIZED_NAME, PUBLICATION_ID, PERSON_ID, RANK, PERSON_NAME, EXTERNAL) VALUES (?, ?, ?, ?, ?, ?)",
                            write_batch,
                        )
                        conn.commit()
                        write_batch = []

                if write_batch:
                    conn.executemany(
                        "INSERT INTO TEST (NORMALIZED_NAME, PUBLICATION_ID, PERSON_ID, RANK, PERSON_NAME, EXTERNAL) VALUES (?, ?, ?, ?, ?, ?)",
                        write_batch,
                    )
                    conn.commit()

                rows = cursor.fetchmany(READ_BATCH_SIZE)
    except sqlite3.Error as e:
        print(e)


if __name__ == "__main__":
    create_table()
    read_from_publications_and_normalize()
