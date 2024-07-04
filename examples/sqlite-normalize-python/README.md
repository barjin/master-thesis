# Python script for normalizing researcher names

This script provides a function to normalize researcher names.

For more context, see the thesis "Social network analysis in academic environment" on my [GitHub](https://jindrich.bar/master-thesis/bar-social-network-analysis-in-academic-environment-2024.pdf).

## Usage

```sql
python ./index.py
```

The script expects an SQLite database `./explorer.db` with a table `PUBLICATION_AUTHOR_ALL`. The table should have a column `PERSON_NAME` with researcher names.