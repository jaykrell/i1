/* spec is rougly, from memory:

given multiline string such as:

line := [time1 time2] method endpoint status client

x := does not matter much, but has no spaces
method := get | post | put
time1 := x
time2 := x
status := [0-9]+
client := x
endpoint := endpoint1 | endpoint2
endpoint1 := no spaces
endpoint2 := no spaces, and ends with "/[0-9]+"

time1 and time2 are fixed width, which can be taken advantage or not
given endpoint2, replace /[0-9]+ with #
equivalence set := method + endpoint + status
produce output, sorted by count, of equivalence set
  output order is unspecified otherwise

unspecified if data fits in memory or efficiency requirements.

method+endpoint+status could just about be considered as one string but that is not done
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// This input is a little sloppy: multiple spaces between some fields.
char input[]=
"[1234 5678] get /users/123 200 cli1\n"
"[2345 6789] get /users    200  cli2\n"
"[1234 5678] get /users/123 200 cli3\n"
"[2345 6789] put /users    200  cli4\n"
"[1234 5678] put /users/123 200 cli5\n"
"[2345 6789] get /users    200  cli6\n"

"[a234 5678] get /users/123 200 cli1\n"
"[b345 6789] get /users    200  cli2\n"
"[c234 5678] get /users/423 200 cli3\n"
"[d345 6789] put /users    200  cli4\n"
"[e234 5678] put /users/523 200 cli5\n"
"[f345 6789] get /users    200  cli6\n"

"[a234 567a] get 1/users/123 200 cli1\n"
"[b345 678d] get 2/users    200  cli2\n"
"[c234 567f] get 3/users/423 200 cli3\n"
"[d345 678f] put 4/users    200  cli4\n"
"[e234 567f] put 5/users/523 200 cli5\n"
"[f345 67g9] get 5/users    200  cli6\n"
;

typedef struct Data
{
    char* method; 
    char* endpoint;
    int status;
    size_t count;
} Data;

int
Data_CompareWithoutCount(const void* va, const void* vb)
{
    Data* a = (Data*)va;
    Data* b = (Data*)vb;
    int i = strcmp(a->method, b->method);
    if (i)
        return i;
    i = strcmp(a->endpoint, b->endpoint);
    if (i)
        return i;
    if (a->status < b->status)
        return -1;
    if (a->status > b->status)
        return 1;
    return 0;
}

int
Data_CompareCount(const void* va, const void* vb)
{
    Data* a = (Data*)va;
    Data* b = (Data*)vb;
    if (a->count > b->count)
        return -1;
    if (a->count < b->count)
        return 1;
    return 0;
}

int
EndsWithDigits(const char* s)
{
    char ch;
    while (ch = *s++)
    {
        if (ch < '0' || ch > '9')
            return 0;
    }
    return 1;
}

char*
LastSlash(char* s)
{
    return strrchr(s, '/');
}

void
NormalizeEndpoint(char* endpoint)
// Given a string that might end in "/[0-9]+", replace that ending with "/#".
{
    char* number;

    char* slash = LastSlash(endpoint);

    // if no slash or end of string, done
    // i.e. string ending in "/" does not count
    if (!slash || !slash[1])
        return;

    number = slash + 1;

    // Does it end in all numbers? If not, done.
    if (!EndsWithDigits(number))
        return;

    *number = '#';
    number[1] = 0;
}

char*
SkipSpaces(char* p)
{
    while (*p == ' ')
        ++p;
    return p;
}

char*
SkipToSpace(char* p)
{
    char ch;
    while ((ch = *p) && ch != ' ')
        ++p;
    return p;
}

char*
FieldSplit(char** p)
{
    // skip spaces at field start (sloppy input)
    *p = SkipSpaces(*p);

    char* result = *p;

    // Skip to next space.
    *p = SkipToSpace(*p);

    assert(**p == ' ');

    // Nul terminate the field, sometimes useful.
    **p = 0;

    return result;
}

int main()
{
    char* p = input;
    char* newline = 0;
    size_t line_count = 0;
    Data* data = 0;
    int print = 0; // set to n to print the first n parses
    size_t i = 0;
    size_t j = 0;
    size_t total_print = 0;

    // count lines, also nul terminating them for possible slight future ease/optimization
    // really just that strlen probably more optimized than strchr and nul terminated
    // strings are somewhat more idiomatic; real world code should store lengths instead.
    while (newline = strchr(p, '\n'))
    {
        ++line_count;
        *newline = 0;
        p = ++newline;
    }

    //printf("line_count:%u\n", (unsigned)line_count);

    // allocate storage for data
    data = (Data*)calloc(line_count, sizeof(Data));
    assert(data);

    p = input;
    for (i = 0; i < line_count; ++i)
    {
        // split fields
        char* time1 = FieldSplit(&p); ++p;
        char* time2 = FieldSplit(&p); ++p;
        char* method = FieldSplit(&p); ++p;
        char* endpoint = FieldSplit(&p); ++p;
        char* status = FieldSplit(&p); ++p;
        char* client = p;
        p += strlen(p) + 1;

        if (print)
            printf(
                "time1:'%s' "
                "time2:'%s' "
                "method:'%s' "
                "endpoint:'%s' "
                "status:'%s' "
                "client:'%s' ",
                time1, time2, method, endpoint, status, client);

        NormalizeEndpoint(endpoint);
        if (print)
            printf("normalizedEndpoint:'%s' ", endpoint);

        int istatus = atoi(status);
        if (print)
            printf("istatus:'%d' ", istatus);

        if (print)
        {
            printf("\n");
            --print;
        }

        data[i].method = method;
        data[i].status = istatus;
        data[i].endpoint = endpoint;
        data[i].count = 1;
    }

    if (line_count > 1)
    {
        // sort by method + status + endpoint
        qsort(data, line_count, sizeof(*data), Data_CompareWithoutCount);

        // equivalent items are now adjacent; count them
        for (i = 0; i < line_count - 1; ++i)
        {
            for (j = i + 1; j < line_count; ++j)
            {
                if (Data_CompareWithoutCount(&data[i], &data[j]) == 0)
                {
                    data[j].count = 0; // don't print
                    data[i].count += 1;
                }
                else
                {
                    i = j - 1;
                    break;
                }
            }
        }

        // sort by count
        qsort(data, line_count, sizeof(*data), Data_CompareCount);
    }

    for (i = 0; i < line_count; ++i)
    {
        if (!data[i].count)
            continue;
        printf("%s %s %d %lu\n", data[i].method, data[i].endpoint, data[i].status, (unsigned long)data[i].count);
        total_print += data[i].count;
    }

    assert(total_print == line_count);
}
