  1 #include <stdio.h>
  2 #include <stdlib.h>
  3 #include <string.h>
  4 #include <unistd.h>
  5 #include <assert.h>
  6 #include "dict.h"
  7 
  8 struct elt {
  9     struct elt *next;
 10     char *key;
 11     char *value;
 12 };
 13 
 14 struct dict {
 15     int size;
 16     int n;
 17     struct elt **table;
 18 };
 19 
 20 #define INITIAL_SIZE 100 // 크기가 100인 dictionary
 21 #define GROWTH_FACTOR 2
 22 #define MAX_LOAD_FACTOR 1
 23 
 24 Dict internalDictCreate(int size)
 25 {
 26     Dict d;
 27     int i;
 28 
 29     d = malloc(sizeof(*d));
 30 
 31     d->size = size;
 32     d->n = 0;
 33     d->table = malloc(sizeof(struct elt *) * d->size);
 34 
 35     assert(d->table != 0);
 36 
 37     for(i = 0; i < d->size; i++) d->table[i] = 0;
 38 
 39     return d;
 40 }
 41 
 42 Dict DictCreate(void)
 43 {
 44     return internalDictCreate(INITIAL_SIZE);
 45 }
 46 
 47 void DictDestroy(Dict d)
 48 {
 49     int i;
 50     struct elt *e;
 51     struct elt *next;
 52 
 53     for(i = 0; i < d->size; i++){
 54         for(e = d->table[i]; e != 0; e = next){
 55             next = e -> next;
 56 
 57             free(e->key);
 58             free(e->value);
 59             free(e);
 60         }
 61     }
 62 
 63     free(d->table);
 64     free(d);
 65 }
 66 
 67 #define MULTIPLER 97
 68 
 69 static unsigned long hash_function(const char *s)
 70 {
 71     unsigned const char *us;
 72     unsigned long h;
 73 
 74     h = 0;
 75 
 76     for(us = (unsigned const char *) s; *us; us++){
 77         h = h * MULTIPLER + *us;
 78     }
 79 
 80     return h;
 81 }
 82 
 83 static void grow(Dict d)
 84 {
 85     Dict d2;
 86     struct dict swap;
 87     int i;
 88     struct elt *e;
 89 
 90     d2 = internalDictCreate(d -> size * GROWTH_FACTOR);
 91 
 92     for(i = 0; i < d->size; i++){
 93         for(e = d->table[i]; e != 0; e = e->next){
 94             DictInsert(d2, e->key, e->value);
 95         }
 96     }
 97 
 98     swap = *d;
 99     *d = *d2;
100     *d2 = swap;
101 
102     DictDestroy(d2);
103 }
104 
105 int DictInsert(Dict d, const char *key, const char *value)
106 {
107     struct elt *e;
108     unsigned long h;
109 
110     assert(key);
111     assert(value);
112 
113     e = malloc(sizeof(*e));
114 
115     assert(e);
116 
117     e -> key = strdup(key);
118     e -> value = strdup(value);
119 
120     h = hash_function(key) % d -> size;
121 
122     e -> next = d -> table[h];
123     d -> table[h] = e;
124 
125     d -> n++;
126 
127     if(d->n >= d->size * MAX_LOAD_FACTOR) {
128         return -1;
129         //grow(d); 꽉 찰 경우 grow함수를 통해 크기를 늘린다.
130     }
131 
132     return 0;
133 }
134 
135 const char *DictSearch(Dict d, const char *key)
136 {
137     struct elt *e;
138 
139     for(e = d -> table[hash_function(key) % d -> size]; e != 0; e = e -> next){
140         if(!strcmp(e->key, key)){
141             return e->value;
142         }
143     }
144 
145     return 0;
146 }
147 
148 const char *DictUpdate(Dict d, const char *key, const char *value)
149 {
150     struct elt *e;
151 
152     for(e = d -> table[hash_function(key) % d -> size]; e != 0; e = e -> next){
153         if(!strcmp(e->key, key)){
154             e->value = strdup(value);
155         }
156     }
157 
158     return 0;
159 }
160 
161 void DictDelete(Dict d, const char *key)
162 {
163     struct elt **prev;
164     struct elt *e;
165 
166     for(prev = &(d->table[hash_function(key) % d -> size]); *prev != 0; prev = &((*prev) -> next)){
167         if(!strcmp((*prev)->key, key)){
168             e = *prev;
169             *prev = e -> next;
170 
171             free(e->key);
172             free(e->value);
173             free(e);
174 
175             return;
176         }
177     }
178 }