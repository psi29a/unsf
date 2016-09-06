/* This file is in the public domain */

#include <string.h>

static char *
unsf_strtok_r(char *str, const char *delim, char **saveptr)
{
   char *p;

   if (str == NULL)
      str = *saveptr;

   if (str == NULL)
      return NULL;

   while (*str && strchr(delim, *str))
      str++;

   if (*str == '\0')
   {
      *saveptr = NULL;
      return NULL;
   }

   p = str;
   while (*p && !strchr(delim, *p))
      p++;

   if (*p == '\0')
      *saveptr = NULL;
   else
   {
      *p = '\0';
      p++;
      *saveptr = p;
   }

   return str;
}
