//
// Created by lucas on 18/11/24.
//

#ifndef POWERMONIT_RAPL_H
#define POWERMONIT_RAPL_H

typedef struct rapl_s rapl_t;

rapl_t *rapl_init();
rapl_t *rapl_retrieve(rapl_t *rapl, bool reset);
void rapl_free(rapl_t **rapl);

#endif //POWERMONIT_RAPL_H
