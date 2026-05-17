
static int strcmp_cmd(char *a, char *b)
{
    while (*a && *b)
    {
        if (*a != *b)
            return 0;
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0');
}

static void parse(char *input, char **cmd, char **arg)
{
    *cmd = input;
    *arg = 0;

    while (*input)
    {
        if (*input == ' ')
        {
            *input = "\0";
            input++;
            if (*input)
                *arg = input;
            return;
        }
        input++;
    }
}