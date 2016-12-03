/*
 * Copyright 2016 Mikeqin <Fengling.Qin@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */
#ifndef LIBSSPLUS_H
#define LIBSSPLUS_H

#define HT_SIZE (1 << 30)
#define HT_PRB_LMT 2
#define HT_PRB_C1 1
#define HT_PRB_C2 0

typedef uint32_t ssp_pair[2];

int  ssp_hasher_init(void);
void ssp_hasher_update_stratum(struct pool *pool, bool clean);
void ssp_hasher_test(void);

void ssp_sorter_init(uint32_t max_size, uint32_t limit, uint32_t c1, uint32_t c2);
void ssp_sorter_flush(void);
int  ssp_sorter_get_pair(ssp_pair pair);

#endif /* LIBSSPLUS_H */
