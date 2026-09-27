#include "search.h"
#include "../../../Lib/string.h"

static char query[SEARCH_MAX_QUERY_LEN] = "";

void search_set_query(const char *q)
{
    strncpy(query, q, SEARCH_MAX_QUERY_LEN);
}

void search_clear_query(void)
{
    query[0] = '\0';
}

uint32_t search_get_query(char *out, uint32_t max_len)
{
    strncpy(out, query, (int)max_len);
    return strlen(query);
}

static bool contains(const char *haystack, const char *needle)
{
    if (needle[0] == '\0')
        return true;

    for (uint32_t i = 0; haystack[i]; i++)
    {
        uint32_t j = 0;

        while (needle[j] && haystack[i + j] == needle[j])
            j++;
        if (needle[j] == '\0')
            return true;
    }
    return false;
}

uint32_t search_filter(const list_entry_t *in, uint32_t in_count, list_entry_t *out, uint32_t max_out)
{
    uint32_t n = 0;
    for (uint32_t i = 0; i < in_count && n < max_out; i++)

    {
        if (contains(in[i].name, query))
            out[n++] = in[i];
    }
    return n;
}

const char *search_dependency_note(void)
{
    return "List and Registry keep working fine if this stops ... search_filter() "
           "just stops narrowing the list, so every entry shows unfiltered. "
           "Nothing crashes, the search bar just stops doing anything.";
}

bool search_selftest(void)
{
    list_entry_t in[4];

    strcpy(in[0].name, "terminal");

    strcpy(in[1].name, "files");

    strcpy(in[2].name, "text-editor");

    strcpy(in[3].name, "settings");

    list_entry_t out[4];

    search_clear_query();
    uint32_t n = search_filter(in, 4, out, 4);
    if (n != 4)
        return false;

    search_set_query("te");

    n = search_filter(in, 4, out, 4);
    if (n != 2)
        return false;
    if (strcmp(out[0].name, "terminal") != 0)
        return false;
    if (strcmp(out[1].name, "text-editor") != 0)
        return false;

    search_set_query("zzz");
    n = search_filter(in, 4, out, 4);
    if (n != 0)
        return false;

    search_set_query("files");
    char q[SEARCH_MAX_QUERY_LEN];

    uint32_t len = search_get_query(q, SEARCH_MAX_QUERY_LEN);
    if (len != 5 || strcmp(q, "files") != 0)
        return false;

    search_set_query("t");
    n = search_filter(in, 4, out, 1);
    if (n != 1)
        return false;

    return true;
}
