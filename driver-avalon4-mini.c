/*
 * Copyright 2013-2014 Con Kolivas <kernel@kolivas.org>
 * Copyright 2012-2014 Xiangfu <xiangfu@openmobilefree.com>
 * Copyright 2014-2015 Mikeqin <Fengling.Qin@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */
#include "config.h"
#include "miner.h"
#include "driver-avalon4-mini.h"
#include "crc.h"
#include "sha2.h"
#include "hexdump.c"

#define UNPACK32(x, str)			\
{						\
	*((str) + 3) = (uint8_t) ((x)      );	\
	*((str) + 2) = (uint8_t) ((x) >>  8);	\
	*((str) + 1) = (uint8_t) ((x) >> 16);	\
	*((str) + 0) = (uint8_t) ((x) >> 24);	\
}

int opt_avalonm_freq[3] = {AVAM_DEFAULT_FREQUENCY,
			   AVAM_DEFAULT_FREQUENCY,
			   AVAM_DEFAULT_FREQUENCY};
uint16_t opt_avalonm_ntime_offset = AVAM_DEFAULT_ASIC_COUNT;
int opt_avalonm_voltage = AVAM_DEFAULT_VOLTAGE;
static uint32_t g_freq_array[][2] = {
	{100, 0x1e678447},
	{113, 0x22688447},
	{125, 0x1c470447},
	{138, 0x2a6a8447},
	{150, 0x22488447},
	{163, 0x326c8447},
	{175, 0x1a268447},
	{188, 0x1c270447},
	{200, 0x1e278447},
	{213, 0x20280447},
	{225, 0x22288447},
	{238, 0x24290447},
	{250, 0x26298447},
	{263, 0x282a0447},
	{275, 0x2a2a8447},
	{288, 0x2c2b0447},
	{300, 0x2e2b8447},
	{313, 0x302c0447},
	{325, 0x322c8447},
	{338, 0x342d0447},
	{350, 0x1a068447},
	{363, 0x382e0447},
	{375, 0x1c070447},
	{388, 0x3c2f0447},
	{400, 0x1e078447},
	{413, 0x40300447},
	{425, 0x20080447},
	{438, 0x44310447},
	{450, 0x22088447},
	{463, 0x48320447},
	{475, 0x24090447},
	{488, 0x4c330447},
	{500, 0x26098447},
	{513, 0x50340447},
	{525, 0x280a0447},
	{538, 0x54350447},
	{550, 0x2a0a8447},
	{563, 0x58360447},
	{575, 0x2c0b0447},
	{588, 0x5c370447},
	{600, 0x2e0b8447},
	{613, 0x60380447},
	{625, 0x300c0447},
	{638, 0x64390447},
	{650, 0x320c8447},
	{663, 0x683a0447},
	{675, 0x340d0447},
	{688, 0x6c3b0447},
	{700, 0x360d8447},
	{713, 0x703c0447},
	{725, 0x380e0447},
	{738, 0x743d0447},
	{750, 0x3a0e8447},
	{763, 0x783e0447},
	{775, 0x3c0f0447},
	{788, 0x7c3f0447},
	{800, 0x3e0f8447},
	{813, 0x3e0f8447},
	{825, 0x40100447},
	{838, 0x40100447},
	{850, 0x42108447},
	{863, 0x42108447},
	{875, 0x44110447},
	{888, 0x44110447},
	{900, 0x46118447},
	{913, 0x46118447},
	{925, 0x48120447},
	{938, 0x48120447},
	{950, 0x4a128447},
	{963, 0x4a128447},
	{975, 0x4c130447},
	{988, 0x4c130447},
	{1000, 0x4e138447}
};

static int avalonm_init_pkg(struct avalonm_pkg *pkg, uint8_t type, uint8_t idx, uint8_t cnt)
{
	unsigned short crc;

	pkg->head[0] = AVAM_H1;
	pkg->head[1] = AVAM_H2;

	pkg->type = type;
	pkg->opt = 0;
	pkg->idx = idx;
	pkg->cnt = cnt;

	crc = crc16(pkg->data, AVAM_P_DATA_LEN);

	pkg->crc[0] = (crc & 0xff00) >> 8;
	pkg->crc[1] = crc & 0x00ff;
	return 0;
}

static int job_idcmp(uint8_t *job_id, char *pool_job_id)
{
	int job_id_len;
	unsigned short crc, crc_expect;

	if (!pool_job_id)
		return 1;

	job_id_len = strlen(pool_job_id);
	crc_expect = crc16((unsigned char *)pool_job_id, job_id_len);

	crc = job_id[0] << 8 | job_id[1];

	if (crc_expect == crc)
		return 0;

	applog(LOG_DEBUG, "Avalon4 mini: job_id doesn't match! [%04x:%04x (%s)]",
	       crc, crc_expect, pool_job_id);

	return 1;
}

static int decode_pkg(struct thr_info *thr, struct avalonm_ret *ar)
{
	struct cgpu_info *avalonm = thr->cgpu;
	struct avalonm_info *info = avalonm->device_data;

	unsigned int expected_crc;
	unsigned int actual_crc;
	uint32_t nonce, nonce2, ntime;
	uint8_t job_id[2];
	int pool_no;
	struct pool *pool, *real_pool;
	struct pool *pool_stratum0 = &info->pool0;
	struct pool *pool_stratum1 = &info->pool1;
	struct pool *pool_stratum2 = &info->pool2;


	if (ar->head[0] != AVAM_H1 && ar->head[1] != AVAM_H2) {
		applog(LOG_DEBUG, "%s-%d: H1 %02x, H2 %02x",
				avalonm->drv->name, avalonm->device_id,
				ar->head[0], ar->head[1]);
		hexdump(ar->data, 32);
		return 1;
	}

	expected_crc = crc16(ar->data, AVAM_P_DATA_LEN);
	actual_crc = (ar->crc[1] & 0xff) | ((ar->crc[0] & 0xff) << 8);
	if (expected_crc != actual_crc) {
		applog(LOG_DEBUG, "%s-%d: %02x: expected crc(%04x), actual_crc(%04x)",
		       avalonm->drv->name, avalonm->device_id,
		       ar->type, expected_crc, actual_crc);
		return 1;
	}

	switch(ar->type) {
	case AVAM_P_NONCE:
		applog(LOG_DEBUG, "%s-%d: AVAM_P_NONCE", avalonm->drv->name, avalonm->device_id);
		hexdump(ar->data, 32);
		job_id[0] = ar->data[0];
		job_id[1] = ar->data[1];
		pool_no = (ar->data[2] << 8) | ar->data[3];
		nonce2 = (ar->data[4] << 24) | (ar->data[5] << 16) | (ar->data[6] << 8) | ar->data[7];
		nonce = (ar->data[11] << 24) | (ar->data[10] << 16) | (ar->data[9] << 8) | ar->data[8];

		if (pool_no >= total_pools || pool_no < 0) {
			applog(LOG_DEBUG, "%s-%d: Wrong pool_no %d",
					avalonm->drv->name, avalonm->device_id,
					pool_no);
			break;
		}

		nonce = be32toh(nonce);
		nonce -= 0x4000;
		ntime = 0;

		applog(LOG_DEBUG, "%s-%d: Found! P:%d - J:%02x%02x N2:%08x N:%08x NR:%d",
		       avalonm->drv->name, avalonm->device_id,
		       pool_no, job_id[0], job_id[1], nonce2, nonce, ntime);

		real_pool = pool = pools[pool_no];

		if (job_idcmp(job_id, pool->swork.job_id)) {
			if (!job_idcmp(job_id, pool_stratum0->swork.job_id)) {
				applog(LOG_DEBUG, "%s-%d: Match to previous stratum0! (%s)",
				       		avalonm->drv->name, avalonm->device_id,
						pool_stratum0->swork.job_id);
				pool = pool_stratum0;
			} else if (!job_idcmp(job_id, pool_stratum1->swork.job_id)) {
				applog(LOG_DEBUG, "%s-%d: Match to previous stratum1! (%s)",
						avalonm->drv->name, avalonm->device_id,
						pool_stratum1->swork.job_id);
				pool = pool_stratum1;
			} else if (!job_idcmp(job_id, pool_stratum2->swork.job_id)) {
				applog(LOG_DEBUG, "%s-%d: Match to previous stratum2! (%s)",
						avalonm->drv->name, avalonm->device_id,
						pool_stratum2->swork.job_id);
				pool = pool_stratum2;
			} else {
				applog(LOG_ERR, "%s-%d: Cannot match to any stratum! (%s)",
						avalonm->drv->name, avalonm->device_id,
						pool->swork.job_id);
				inc_hw_errors(thr);
				break;
			}
		}

		submit_nonce2_nonce(thr, pool, real_pool, nonce2, nonce, ntime);
		info->nonce_cnts++;
		break;
	case AVAM_P_STATUS:
		applog(LOG_DEBUG, "%s-%d: AVAM_P_STATUS", avalonm->drv->name, avalonm->device_id);
		hexdump(ar->data, 32);
		break;
	default:
		applog(LOG_DEBUG, "%s-%d: Unknown response (%x)", avalonm->drv->name, avalonm->device_id,
				ar->type);
		break;
	}
	return 0;
}

static int avalonm_send_pkg(struct cgpu_info *avalonm, const struct avalonm_pkg *pkg)
{
	int err = -1;
	int writecnt;

	if (unlikely(avalonm->usbinfo.nodev))
		return -1;

	err = usb_write(avalonm, (char *)pkg, AVAM_P_COUNT, &writecnt, C_AVAM_WRITE);
	if (err || writecnt != AVAM_P_COUNT) {
		applog(LOG_DEBUG, "%s-%d: avalonm_send_pkg %d, w(%d-%d)!", avalonm->drv->name, avalonm->device_id, err, AVAM_P_COUNT, writecnt);
		return -1;
	}

	return writecnt;
}

static int avalonm_receive_pkg(struct cgpu_info *avalonm, struct avalonm_ret *ret)
{
	int err = -1;
	int readcnt;

	if (unlikely(avalonm->usbinfo.nodev))
		return -1;

	err = usb_read(avalonm, (char*)ret, AVAM_P_COUNT, &readcnt, C_AVAM_READ);
	if (err || readcnt != AVAM_P_COUNT) {
		applog(LOG_DEBUG, "%s-%d: avalonm_receive_pkg %d, w(%d-%d)!", avalonm->drv->name, avalonm->device_id, err, AVAM_P_COUNT, readcnt);
		return -1;
	}

	return readcnt;
}

static int avalonm_xfer_pkg(struct cgpu_info *avalonm, const struct avalonm_pkg *pkg, struct avalonm_ret *ret)
{
	if (sizeof(struct avalonm_pkg) != avalonm_send_pkg(avalonm, pkg))
		return AVAM_SEND_ERROR;

	cgsleep_ms(50);
	if (sizeof(struct avalonm_ret) != avalonm_receive_pkg(avalonm, ret))
		return AVAM_SEND_ERROR;

	return AVAM_SEND_OK;
}

static struct cgpu_info *avalonm_detect_one(struct libusb_device *dev, struct usb_find_devices *found)
{
	struct cgpu_info *avalonm = usb_alloc_cgpu(&avalonm_drv, 1);
	struct avalonm_info *info;
	struct avalonm_pkg send_pkg;
	struct avalonm_ret ar;
	int ret;

	if (!usb_init(avalonm, dev, found)) {
		applog(LOG_ERR, "Avalonm failed usb_init");
		avalonm = usb_free_cgpu(avalonm);
		return NULL;
	}

	/* Avalonm prefers not to use zero length packets */
	avalonm->nozlp = true;

	/* We have an Avalonm connected */
	avalonm->threads = 1;
	memset(send_pkg.data, 0, AVAM_P_DATA_LEN);
	avalonm_init_pkg(&send_pkg, AVAM_P_DETECT, 1, 1);
	ret = avalonm_xfer_pkg(avalonm, &send_pkg, &ar);
	if ((ret != AVAM_SEND_OK) && (ar.type != AVAM_P_ACKDETECT)) {
		applog(LOG_DEBUG, "%s-%d: Failed to detect Avalon4 mini!", avalonm->drv->name, avalonm->device_id);
		return NULL;
	}

	add_cgpu(avalonm);

	usb_buffer_clear(avalonm);
	update_usb_stats(avalonm);
	applog(LOG_INFO, "%s-%d: Found at %s", avalonm->drv->name, avalonm->device_id,
	       avalonm->device_path);

	avalonm->device_data = cgcalloc(sizeof(struct avalonm_info), 1);
	info = avalonm->device_data;
	info->mainthr = NULL;
	info->workinit = 0;
	info->nonce_cnts = 0;
	memcpy(info->avam_ver, ar.data, AVAM_MM_VER_LEN);
	return avalonm;
}

static uint32_t avalonm_get_cpm(int freq)
{
	int i;

	for (i = 0; i < sizeof(g_freq_array) / sizeof(g_freq_array[0]); i++)
		if (freq >= g_freq_array[i][0] && freq < g_freq_array[i+1][0])
			return g_freq_array[i][1];

	/* return the lowest freq if not found */
	return g_freq_array[0][1];
}

static void avalonm_set_freq(struct cgpu_info *avalonm)
{
	struct avalonm_info *info = avalonm->device_data;
	struct avalonm_pkg send_pkg;
	uint32_t tmp;

	if ((info->set_frequency[0] == opt_avalonm_freq[0]) &&
		(info->set_frequency[1] == opt_avalonm_freq[1]) &&
			(info->set_frequency[2] == opt_avalonm_freq[2]))
		return;

	info->set_frequency[0] = opt_avalonm_freq[0];
	info->set_frequency[1] = opt_avalonm_freq[1];
	info->set_frequency[2] = opt_avalonm_freq[2];

	memset(send_pkg.data, 0, AVAM_P_DATA_LEN);
	tmp = avalonm_get_cpm(info->set_frequency[0]);
	tmp = be32toh(tmp);
	memcpy(send_pkg.data, &tmp, 4);
	tmp = avalonm_get_cpm(info->set_frequency[1]);
	tmp = be32toh(tmp);
	memcpy(send_pkg.data + 4, &tmp, 4);
	tmp = avalonm_get_cpm(info->set_frequency[2]);
	tmp = be32toh(tmp);
	memcpy(send_pkg.data + 8, &tmp, 4);

	avalonm_init_pkg(&send_pkg, AVAM_P_SET_FREQ, 1, 1);
	avalonm_send_pkg(avalonm, &send_pkg);
	applog(LOG_DEBUG, "%s-%d: Avalonm set freq %d,%d,%d",
			avalonm->drv->name, avalonm->device_id,
			info->set_frequency[0],
			info->set_frequency[1],
			info->set_frequency[2]);
}

/* TODO: encode */
static uint16_t encode_voltage(uint32_t v)
{
	return 0;
}

static void avalonm_set_voltage(struct cgpu_info *avalonm)
{
	struct avalonm_info *info = avalonm->device_data;
	struct avalonm_pkg send_pkg;
	uint16_t tmp;

	if (info->set_voltage == opt_avalonm_voltage)
		return;

	info->set_voltage = opt_avalonm_voltage;
	memset(send_pkg.data, 0, AVAM_P_DATA_LEN);
	/* Use shifter to set voltage */
	tmp = info->set_voltage;
	tmp = encode_voltage(tmp);
	tmp = htobe16(tmp);
	memcpy(send_pkg.data, &tmp, 2);

	/* Package the data */
	avalonm_init_pkg(&send_pkg, AVAM_P_SET_VOLT, 1, 1);
	avalonm_send_pkg(avalonm, &send_pkg);
	applog(LOG_DEBUG, "%s-%d: Avalonm set volt %d",
			avalonm->drv->name, avalonm->device_id,
			info->set_voltage);
}

static inline void avalonm_detect(bool __maybe_unused hotplug)
{
	usb_detect(&avalonm_drv, avalonm_detect_one);
}

static void *avalonm_get_reports(void *userdata)
{
	struct cgpu_info *avalonm = (struct cgpu_info *)userdata;
	struct avalonm_info *info = avalonm->device_data;
	struct thr_info *thr = info->mainthr;
	char threadname[16];
	struct avalonm_pkg send_pkg;
	struct avalonm_ret ar;
	int ret;

	/* wait miner thread start */
	while (!info->workinit)
		cgsleep_ms(200);

	snprintf(threadname, sizeof(threadname), "%d/AvamRecv", avalonm->device_id);
	RenameThread(threadname);

	while (likely(!avalonm->shutdown)) {
		memset(send_pkg.data, 0, AVAM_P_DATA_LEN);
		avalonm_init_pkg(&send_pkg, AVAM_P_POLLING, 1, 1);
		ret = avalonm_xfer_pkg(avalonm, &send_pkg, &ar);
		if (ret == AVAM_SEND_OK) {
			applog(LOG_ERR, "%s-%d: Get report 4 %02x%02x%02x%02x ...................",
			       avalonm->drv->name, avalonm->device_id,
			       ar.data[4], ar.data[5], ar.data[6], ar.data[7]);

			decode_pkg(thr, &ar);
		}

		cgsleep_ms(20);
	}

	return NULL;
}

static bool avalonm_prepare(struct thr_info *thr)
{
	struct cgpu_info *avalonm = thr->cgpu;
	struct avalonm_info *info = avalonm->device_data;

	info->mainthr = thr;

	cglock_init(&info->pool0.data_lock);
	cglock_init(&info->pool1.data_lock);
	cglock_init(&info->pool2.data_lock);

	if (pthread_create(&info->read_thr, NULL, avalonm_get_reports, (void*)avalonm))
		quit(1, "Failed to create avalonm read_thr");

	return true;
}

static void rev(unsigned char *s, size_t l)
{
	size_t i, j;
	unsigned char t;

	for (i = 0, j = l - 1; i < j; i++, j--) {
		t = s[i];
		s[i] = s[j];
		s[j] = t;
	}
}

static void copy_pool_stratum(struct pool *pool_stratum, struct pool *pool)
{
	int i;
	int merkles = pool->merkles;
	size_t coinbase_len = pool->coinbase_len;

	if (!pool->swork.job_id)
		return;

	if (!job_idcmp((unsigned char *)pool->swork.job_id, pool_stratum->swork.job_id))
		return;

	cg_wlock(&pool_stratum->data_lock);
	free(pool_stratum->swork.job_id);
	free(pool_stratum->nonce1);
	free(pool_stratum->coinbase);

	pool_stratum->coinbase = cgcalloc(coinbase_len, 1);
	memcpy(pool_stratum->coinbase, pool->coinbase, coinbase_len);

	for (i = 0; i < pool_stratum->merkles; i++)
		free(pool_stratum->swork.merkle_bin[i]);
	if (merkles) {
		pool_stratum->swork.merkle_bin = cgrealloc(pool_stratum->swork.merkle_bin,
							   sizeof(char *) * merkles + 1);
		for (i = 0; i < merkles; i++) {
			pool_stratum->swork.merkle_bin[i] = cgmalloc(32);
			memcpy(pool_stratum->swork.merkle_bin[i], pool->swork.merkle_bin[i], 32);
		}
	}

	pool_stratum->sdiff = pool->sdiff;
	pool_stratum->coinbase_len = pool->coinbase_len;
	pool_stratum->nonce2_offset = pool->nonce2_offset;
	pool_stratum->n2size = pool->n2size;
	pool_stratum->merkles = pool->merkles;

	pool_stratum->swork.job_id = strdup(pool->swork.job_id);
	pool_stratum->nonce1 = strdup(pool->nonce1);

	memcpy(pool_stratum->ntime, pool->ntime, sizeof(pool_stratum->ntime));
	memcpy(pool_stratum->header_bin, pool->header_bin, sizeof(pool_stratum->header_bin));
	cg_wunlock(&pool_stratum->data_lock);
}

static int64_t avalonm_scanhash(struct thr_info *thr)
{
	struct cgpu_info *avalonm = thr->cgpu;
	struct avalonm_info *info = avalonm->device_data;
	int64_t h;
	struct work *work;
	struct avalonm_pkg send_pkg;
	struct pool *pool;
	static int count = 0;
	int job_id_len;
	unsigned short crc;

	if (unlikely(avalonm->usbinfo.nodev)) {
		applog(LOG_ERR, "%s-%d: Device disappeared, shutting down thread",
		       avalonm->drv->name, avalonm->device_id);
		return -1;
	}

	work = get_work(thr, thr->id);
	applog(LOG_ERR, "%s-%d: Get work %08x ----------------------------------",
	       avalonm->drv->name, avalonm->device_id, work->nonce2);

	pool = current_pool();
	copy_pool_stratum(&info->pool2, &info->pool1);
	copy_pool_stratum(&info->pool1, &info->pool0);
	copy_pool_stratum(&info->pool0, pool);

	/* Configuration */
	avalonm_set_freq(avalonm);
	avalonm_set_voltage(avalonm);

	/* P_WORK part 1: midstate */
	memcpy(send_pkg.data, work->midstate, AVAM_P_DATA_LEN);
	rev((void *)(send_pkg.data), AVAM_P_DATA_LEN);

	avalonm_init_pkg(&send_pkg, AVAM_P_WORK, 1, 2);
	hexdump(send_pkg.data, 32);
	avalonm_send_pkg(avalonm, &send_pkg);

	/* P_WORK part 2:
	 * job_id(2)+pool_no(2)+nonce2(4)+reserved(14)+data(12) */
	memset(send_pkg.data, 0, AVAM_P_DATA_LEN);

	job_id_len = strlen(pool->swork.job_id);
	crc = crc16((unsigned char *)pool->swork.job_id, job_id_len);
	applog(LOG_DEBUG, "%s-%d: Pool stratum message JOBS_ID[%04x]: %s",
	       avalonm->drv->name, avalonm->device_id,
	       crc, pool->swork.job_id);
	send_pkg.data[0] = (crc & 0xff00) >> 8;
	send_pkg.data[1] = crc & 0x00ff;
	send_pkg.data[2] = pool->pool_no >> 8;
	send_pkg.data[3] = pool->pool_no & 0xff;
	send_pkg.data[4] = (work->nonce2 >> 24) & 0xff;
	send_pkg.data[5] = (work->nonce2 >> 16) & 0xff;
	send_pkg.data[6] = (work->nonce2 >> 8) & 0xff;
	send_pkg.data[7] = (work->nonce2) & 0xff;
	send_pkg.data[8] = opt_avalonm_ntime_offset >> 8;
	send_pkg.data[9] = opt_avalonm_ntime_offset & 0xff;

	memcpy(send_pkg.data + 20, work->data + 64, 12);
	rev((void *)(send_pkg.data + 20), 12);

	avalonm_init_pkg(&send_pkg, AVAM_P_WORK, 2, 2);
	hexdump(send_pkg.data, 32);
	avalonm_send_pkg(avalonm, &send_pkg);

	info->workinit = 1;

	cgsleep_ms(1);
	if (++count == 4) {
		count = 0;
		applog(LOG_ERR, "%s-%d: Get work 4 Delay =================",
		       avalonm->drv->name, avalonm->device_id);
		cgsleep_ms(200);
	}

	h = info->nonce_cnts;
	info->nonce_cnts = 0;
	return  h * 0xffffffffull;
}

static void avalonm_shutdown(struct thr_info *thr)
{
	struct cgpu_info *avalonm = thr->cgpu;
	struct avalonm_info *info = avalonm->device_data;

	pthread_join(info->read_thr, NULL);
}

char *set_avalonm_freq(char *arg)
{
	char *colon1, *colon2;
	int val1 = 0, val2 = 0, val3 = 0;

	if (!(*arg))
		return NULL;

	colon1 = strchr(arg, ':');
	if (colon1)
		*(colon1++) = '\0';

	if (*arg) {
		val1 = atoi(arg);
		if (val1 < AVAM_DEFAULT_FREQUENCY_MIN || val1 > AVAM_DEFAULT_FREQUENCY_MAX)
			return "Invalid value1 passed to avalonm-freq";
	}

	if (colon1 && *colon1) {
		colon2 = strchr(colon1, ':');
		if (colon2)
			*(colon2++) = '\0';

		if (*colon1) {
			val2 = atoi(colon1);
			if (val2 < AVAM_DEFAULT_FREQUENCY_MIN || val2 > AVAM_DEFAULT_FREQUENCY_MAX)
				return "Invalid value2 passed to avalonm-freq";
		}

		if (colon2 && *colon2) {
			val3 = atoi(colon2);
			if (val3 < AVAM_DEFAULT_FREQUENCY_MIN || val3 > AVAM_DEFAULT_FREQUENCY_MAX)
				return "Invalid value3 passed to avalonm-freq";
		}
	}

	if (!val1)
		val3 = val2 = val1 = AVAM_DEFAULT_FREQUENCY;

	if (!val2)
		val3 = val2 = val1;

	if (!val3)
		val3 = val2;

	opt_avalonm_freq[0] = val1;
	opt_avalonm_freq[1] = val2;
	opt_avalonm_freq[2] = val3;

	return NULL;
}

static char *avalonm_set_device(struct cgpu_info *avalonm, char *option, char *setting, char *replybuf)
{
	if (strcasecmp(option, "help") == 0) {
		sprintf(replybuf, "frequency|voltage");
		return replybuf;
	}

	if (strcasecmp(option, "frequency") == 0) {
		if (!setting || !*setting) {
			sprintf(replybuf, "missing frequency value");
			return replybuf;
		}

		if (set_avalonm_freq(setting)) {
			sprintf(replybuf, "invalid frequency value, valid range %d-%d",
				AVAM_DEFAULT_FREQUENCY_MIN, AVAM_DEFAULT_FREQUENCY_MAX);
			return replybuf;
		}

		applog(LOG_NOTICE, "%s-%d: Update frequency to %d",
		       avalonm->drv->name, avalonm->device_id,
		       (opt_avalonm_freq[0] * 4 + opt_avalonm_freq[1] * 4 + opt_avalonm_freq[2]) / 9);

		return NULL;
	}

	if (strcasecmp(option, "voltage") == 0) {
		int val_volt;

		if (!setting || !*setting) {
			sprintf(replybuf, "missing voltage value");
			return replybuf;
		}

		sscanf(setting, "%d", &val_volt);
		if (val_volt < AVAM_DEFAULT_VOLTAGE_MIN || val_volt > AVAM_DEFAULT_VOLTAGE_MAX) {
			sprintf(replybuf, "invalid val_volt,  valid val_volt range %d-%d",
					AVAM_DEFAULT_VOLTAGE_MIN,
					AVAM_DEFAULT_VOLTAGE_MAX);
			return replybuf;
		}

		opt_avalonm_voltage = val_volt;
		applog(LOG_NOTICE, "%s-%d: Update voltage to %d",
		       avalonm->drv->name, avalonm->device_id, val_volt);

		return NULL;
	}

	sprintf(replybuf, "Unknown option: %s", option);
	return replybuf;
}

static struct api_data *avalonm_api_stats(struct cgpu_info *cgpu)
{
	struct api_data *root = NULL;
	struct avalonm_info *info = cgpu->device_data;

	root = api_add_string(root, "AVAM VER", info->avam_ver, false);
	return root;
}

static void avalonm_statline_before(char *buf, size_t bufsiz, struct cgpu_info *avalonm)
{
	struct avalonm_info *info = avalonm->device_data;
	int frequency;

	frequency = (info->set_frequency[0] * 4 + info->set_frequency[1] * 4 + info->set_frequency[2]) / 9;
	tailsprintf(buf, bufsiz, "%4dMhz", frequency);
}

struct device_drv avalonm_drv = {
	.drv_id = DRIVER_avalonm,
	.dname = "avalonm",
	.name = "AVM",
	.set_device = avalonm_set_device,
	.get_api_stats = avalonm_api_stats,
	.get_statline_before = avalonm_statline_before,
	.drv_detect = avalonm_detect,
	.thread_prepare = avalonm_prepare,
	.hash_work = hash_driver_work,
	.scanwork = avalonm_scanhash,
	.thread_shutdown = avalonm_shutdown,
};
