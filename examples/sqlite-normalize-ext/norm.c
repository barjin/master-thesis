/*
** 2024-05-13
**
** Jindřich Bär, https://jindrich.bar
**   | https://github.com/barjin/master-thesis
**
******************************************************************************
**
** This is a one-use SQLite extension for normalizing the names of academic researchers.
** For more context, see the thesis "Social network analysis in academic environment"
**    at https://jindrich.bar/master-thesis/bar-social-network-analysis-in-academic-environment-2024.pdf
*/
#include "./sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include <assert.h>
#include <string.h>
#include <regex.h>
#include <stdio.h>
#include <glib-2.0/glib.h>
#include <regex.h>

char* titles_before_name_regex = "^((prof|doc|rndr|mudr|ing|mgr|bc|phdr|judr|msc|phd|csc|bba|mba|bsc|msc|ms|phil|et|assoc|dr) *)*";
char* titles_after_name_regex = " ((phd|csc|dr|drsc|mag|phil) *)+";
GRegex* titles_before_name_compiled;
GRegex* titles_after_name_compiled;

/**
 * Removes all diacritics from a string and convert it to lowercase.
 *  Trims leading and trailing whitespaces and normalizes any internal whitespaces to a single space. 
 *    The result is a trimmed string with only ASCII characters.
 * Params:
 *  - str: the input string
 *  - len: the length of the input string
 * Returns:
 *  - the normalized string. The caller is responsible for freeing the memory.
*/
static const gchar* unac(const gchar* str, gssize len) {
    const gchar* normalized = g_utf8_normalize(str, len, G_NORMALIZE_DEFAULT);

    gchar* acc = g_utf8_strdown(normalized, -1);
    gsize j = 0;

    char is_space = 0;
    for (gsize i = 0; i < strlen(normalized); i++) {
        if(normalized[i] == ' ') {
            is_space = 1;
        } 
        if (g_ascii_isalpha(normalized[i])) {
            if (is_space) {
                acc[j++] = ' ';
                is_space = 0;
            }
            acc[j++] = g_ascii_tolower(normalized[i]);
        }
    }
    acc[j] = '\0';

    g_free((gpointer)normalized);

    return acc;
}

static gchar* remove_titles(const gchar* source, gssize len) {
  GMatchInfo *match_info;
 
  g_regex_match (titles_before_name_compiled, source, 0, &match_info);
  int end;
  g_match_info_fetch_pos(match_info, 0, NULL, &end);

  gchar* result = g_strndup(source + end, len - end);

  // return result;

  g_regex_match (titles_after_name_compiled, result, 0, &match_info);
  int start;
  if(!g_match_info_fetch_pos(match_info, 0, &start, NULL)) {
    g_match_info_free(match_info);
    return result;
  }

  gchar* result2 = g_strndup(result, start);
  g_match_info_free(match_info);
  g_free(result);

  return result2;  
}

/*
** Implementation of the normalize_name() function.
** This function takes a single argument which is a string.
** It returns a copy of the input string with all diacritics removed and converted to lowercase. 
** It also removes academic titles from the beginning and end of the name.
*/
static void normalize_name(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  const unsigned char *zIn;
  int nIn;
  unsigned const char *zOut;
  int i;

  assert( argc==1 );
  if( sqlite3_value_type(argv[0])==SQLITE_NULL ) return;
  zIn = (const unsigned char*)sqlite3_value_text(argv[0]);
  nIn = sqlite3_value_bytes(argv[0]);

  zOut = unac(zIn, nIn);
  sqlite3_result_text(context, remove_titles(zOut, strlen(zOut)), -1, SQLITE_TRANSIENT);
  g_free((gpointer)zOut);
}

#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_norm_init(
  sqlite3 *db, 
  char **pzErrMsg, 
  const sqlite3_api_routines *pApi
){
  int rc = SQLITE_OK;

  titles_before_name_compiled = g_regex_new(titles_before_name_regex, 0, 0, NULL);
  titles_after_name_compiled = g_regex_new(titles_after_name_regex, 0, 0, NULL);

  SQLITE_EXTENSION_INIT2(pApi);
  (void)pzErrMsg;  /* Unused parameter */
  rc = sqlite3_create_function(db, "normalize_name", 1,
                   SQLITE_UTF8|SQLITE_INNOCUOUS|SQLITE_DETERMINISTIC,
                   0, normalize_name, 0, 0);
  return rc;
}
