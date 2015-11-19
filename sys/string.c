#include <sys/sbunix.h>
#include <sys/string.h>

int strcmp (const char *dest, const char *src){
        int i = 0;

        if(dest == NULL || src == NULL)
                return -1;

        if(strlen(dest) != strlen(src))
                return -1;

        while((dest[i] - src[i]) == 0 && i < strlen(dest))
                i++;

        if(i == strlen(dest))
                return 0;
        else
                return dest[i] - src[i];
}

size_t strlen (const char *s){
        size_t size = 0;

        if(s == NULL)
                return 0;

        while(s[size] != '\0')
                size++;
        return size;
}

char *strncpy (char *dest, const char *src, size_t size){
        int i = 0;

        if(dest == NULL || src == NULL)
                return NULL;

        while(i < size && i < strlen(src) &&  (dest[i] = src[i]) != '\0')
                i++;

        dest[i] = '\0';
        return dest;
}

