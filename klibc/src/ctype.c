#include "../include/ctype.h"

static int is_ascii_letter(int character)
{
    return (character >= 'A' && character <= 'Z')
        || (character >= 'a' && character <= 'z');
}

int isalnum(int character)
{
    return is_ascii_letter(character) || isdigit(character);
}

int isalpha(int character)
{
    return is_ascii_letter(character);
}

int isblank(int character)
{
    return character == ' ' || character == '\t';
}

int iscntrl(int character)
{
    return (character >= 0 && character < 0x20) || character == 0x7F;
}

int isdigit(int character)
{
    return character >= '0' && character <= '9';
}

int isgraph(int character)
{
    return character >= 0x21 && character <= 0x7E;
}

int islower(int character)
{
    return character >= 'a' && character <= 'z';
}

int isprint(int character)
{
    return character >= 0x20 && character <= 0x7E;
}

int ispunct(int character)
{
    return isgraph(character) && !isalnum(character);
}

int isspace(int character)
{
    return character == ' ' || (character >= '\t' && character <= '\r');
}

int isupper(int character)
{
    return character >= 'A' && character <= 'Z';
}

int isxdigit(int character)
{
    return isdigit(character) || (character >= 'A' && character <= 'F')
        || (character >= 'a' && character <= 'f');
}

int tolower(int character)
{
    return isupper(character) ? character + ('a' - 'A') : character;
}

int toupper(int character)
{
    return islower(character) ? character - ('a' - 'A') : character;
}