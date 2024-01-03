#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include <string.h>

typedef struct {
    int size;
    char* text;
} StringView;

StringView create_string_view(char* text) {
    int size = strlen(text);

    return (StringView) {
        .size = size,
        .text = text
    };
}

StringView copy_string_view(StringView* view) {
    int size = view->size;
    char* text = (char*)malloc(size);

    for(int i = 0; i < size; i++){
        text[i] = view->text[i];
    }

    return (StringView) {
        .size = size,
        .text = text
    };
}

#endif /* STRING_VIEW_H */
