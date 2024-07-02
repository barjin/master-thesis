# SQLite extension for normalizing researcher names

This extension provides a function to normalize researcher names.

For more context, see the thesis "Social network analysis in academic environment" on my [GitHub](https://jindrich.bar/master-thesis/bar-social-network-analysis-in-academic-environment-2024.pdf).

## Usage

```sql
SELECT normalize_name('RNDr. Johň Doé PhD.'); -- returns 'john doe'
```

## Compilation

On Linux, first supply the `sqlite.h` and `sqlite3ext.h` from the latest release. Then compile the extension:

```bash
gcc -g -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -fPIC -shared ./norm.c -o norm.so -lglib-2.0
```

The extension requires glib `>2.14` because of the regex support.