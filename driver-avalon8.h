/*
 * Copyright 2017 xuzhenxing <xuzhenxing@canaan-creative.com>
 * Copyright 2016 Mikeqin <Fengling.Qin@gmail.com>
 * Copyright 2016 Con Kolivas <kernel@kolivas.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#ifndef _AVALON8_H_
#define _AVALON8_H_

#include "util.h"
#include "i2c-context.h"

#ifdef USE_AVALON8

#define AVA8_FREQUENCY_MAX	1404

#define AVA8_DEFAULT_FAN_MIN		5 /* % */
#define AVA8_DEFAULT_FAN_MAX		100
#define AVA8_DEFAULT_FAN_INTERVAL	15 /* Seconds */

#define AVA8_DEFAULT_TEMP_TARGET	90
#define AVA8_DEFAULT_TEMP_OVERHEAT	105
#define AVA8_DEFAULT_TEMP_HYSTERESIS	5

#define AVA8_DEFAULT_VOLTAGE_MIN	3889
#define AVA8_DEFAULT_VOLTAGE_MAX	5059
#define AVA8_INVALID_VOLTAGE	0
#define AVA8_DEFAULT_VOLTAGE_STEP	78

#define AVA8_DEFAULT_VOLTAGE_LEVEL_MIN	0
#define AVA8_DEFAULT_VOLTAGE_LEVEL_MAX	15

#define AVA8_DEFAULT_VOLTAGE_OFFSET_MIN	-2
#define AVA8_DEFAULT_VOLTAGE_OFFSET	0
#define AVA8_DEFAULT_VOLTAGE_OFFSET_MAX	1

#define AVA8_DEFAULT_FACTORY_INFO_0_MIN	-15
#define AVA8_DEFAULT_FACTORY_INFO_0	0
#define AVA8_DEFAULT_FACTORY_INFO_0_MAX	15

#define AVA8_DEFAULT_FREQUENCY_MIN	24 /* NOTE: It cann't support 0 */
#define AVA8_DEFAULT_FREQUENCY_0	600
#define AVA8_DEFAULT_FREQUENCY_1	636
#define AVA8_DEFAULT_FREQUENCY_2	672
#define AVA8_DEFAULT_FREQUENCY_3	708
#define AVA8_DEFAULT_FREQUENCY_MAX	1404
#define AVA8_DEFAULT_FREQUENCY_SEL	0

#define AVA8_DEFAULT_MODULARS	7	/* Only support 6 modules maximum with one AUC */
#define AVA8_DEFAULT_MINER_CNT	4
#define AVA8_DEFAULT_ASIC_MAX	26
#define AVA8_DEFAULT_PLL_CNT	4
#define AVA8_DEFAULT_PMU_CNT	2

#define AVA8_DEFAULT_POLLING_DELAY	20 /* ms */
#define AVA8_DEFAULT_NTIME_OFFSET	40

#define AVA8_DEFAULT_SMARTSPEED_OFF 0
#define AVA8_DEFAULT_SMARTSPEED_MODE1 1
#define AVA8_DEFAULT_SMART_SPEED	(AVA8_DEFAULT_SMARTSPEED_MODE1)

#define AVA8_DEFAULT_TH_PASS	(162)
#define AVA8_DEFAULT_TH_FAIL	(10921)
#define AVA8_DEFAULT_TH_INIT	(0xffff / 2)
#define AVA8_DEFAULT_TH_MS	1
#define AVA8_DEFAULT_TH_TIMEOUT	0
#define AVA8_DEFAULT_NONCE_MASK 29

#define AVA8_DEFAULT_IIC_DETECT	false

#define AVA8_PWM_MAX	0x3FF
#define AVA8_DRV_DIFFMAX	500
#define AVA8_ASIC_TIMEOUT_CONST	0x2000000 /* 2 ^ 32 / 128 */

#define AVA8_MODULE_DETECT_INTERVAL	30 /* 30 s */

#define AVA8_AUC_VER_LEN	12	/* Version length: 12 (AUC-YYYYMMDD) */
#define AVA8_AUC_SPEED		400000
#define AVA8_AUC_XDELAY  	19200	/* 4800 = 1ms in AUC (11U14)  */
#define AVA8_AUC_P_SIZE		64

#define AVA8_CONNECTER_AUC	1
#define AVA8_CONNECTER_IIC	2

/* avalon8 protocol package type from MM protocol.h
 * https://github.com/Canaan-Creative/MM/blob/avalon8/firmware/protocol.h */
#define AVA8_MM_VER_LEN	15
#define AVA8_MM_DNA_LEN	8
#define AVA8_H1	'C'
#define AVA8_H2	'N'

#define AVA8_P_COINBASE_SIZE	(6 * 1024 + 64)
#define AVA8_P_MERKLES_COUNT	30

#define AVA8_P_COUNT	40
#define AVA8_P_DATA_LEN 32

/* Broadcase with block iic_write*/
#define AVA8_P_DETECT	0x10

/* Broadcase With non-block iic_write*/
#define AVA8_P_STATIC	0x11
#define AVA8_P_JOB_ID	0x12
#define AVA8_P_COINBASE	0x13
#define AVA8_P_MERKLES	0x14
#define AVA8_P_HEADER	0x15
#define AVA8_P_TARGET	0x16
#define AVA8_P_JOB_FIN	0x17

/* ss package */
#define AVA8_P_MID	0xa7

/* Broadcase or with I2C address */
#define AVA8_P_SET	0x20
#define AVA8_P_SET_FIN	0x21
#define AVA8_P_SET_VOLT	0x22
#define AVA8_P_SET_PMU	0x24
#define AVA8_P_SET_PLL	0x25
#define AVA8_P_SET_SS	0x26
#define AVA8_P_SET_FAC	0x28

/* Have to send with I2C address */
#define AVA8_P_POLLING	0x30
#define AVA8_P_SYNC	0x31
#define AVA8_P_TEST	0x32
#define AVA8_P_RSTMMTX	0x33
#define AVA8_P_GET_VOLT	0x34

/* Back to host */
#define AVA8_P_ACKDETECT	0x40
#define AVA8_P_STATUS		0x41
#define AVA8_P_NONCE		0x42
#define AVA8_P_TEST_RET		0x43
#define AVA8_P_STATUS_VOLT	0x46
#define AVA8_P_STATUS_PMU	0x48
#define AVA8_P_STATUS_PLL	0x49
#define AVA8_P_STATUS_LOG	0x4a
#define AVA8_P_STATUS_ASIC	0x4b
#define AVA8_P_STATUS_PVT	0x4c
#define AVA8_P_STATUS_FAC	0x4d

#define AVA8_MODULE_BROADCAST	0
/* End of avalon8 protocol package type */

#define AVA8_IIC_RESET		0xa0
#define AVA8_IIC_INIT		0xa1
#define AVA8_IIC_DEINIT		0xa2
#define AVA8_IIC_XFER		0xa5
#define AVA8_IIC_INFO		0xa6

#define AVA8_FREQ_INIT_MODE	0x0
#define AVA8_FREQ_PLLADJ_MODE	0x1

#define AVA8_DEFAULT_FAVG_TIME	(15 * 60.0)
#define AVA8_DEFAULT_FREQADJ_TIME	60

#define AVA8_DEFAULT_DELTA_T	0
#define AVA8_DEFAULT_DELTA_FREQ	100

#define AVA8_DEFAULT_FACTORY_INFO_CNT	1

#define AVA8_MM841_VIN_ADC_RATIO	(3.3 / 1024.0 * 27.15 / 7.15 * 1000.0 * 100.0)
#define AVA8_MM861_VIN_ADC_RATIO	(3.3 / 1024.0 * 26.0 / 6.0 * 1000.0 * 100.0)

#define AVA8_MM841_VOUT_ADC_RATIO	(3.3 / 1024.0 * 63.0 / 20.0 * 10000.0 * 100.0)
#define AVA8_MM861_VOUT_ADC_RATIO	(3.3 / 1024.0 * 26.0 / 6.0 * 10000.0 * 100.0)

struct avalon8_pkg {
	uint8_t head[2];
	uint8_t type;
	uint8_t opt;
	uint8_t idx;
	uint8_t cnt;
	uint8_t data[32];
	uint8_t crc[2];
};
#define avalon8_ret avalon8_pkg

struct avalon8_info {
	/* Public data */
	int64_t last_diff1;
	int64_t pending_diff1;
	double last_rej;

	int mm_count;
	int xfer_err_cnt;
	int pool_no;

	struct timeval firsthash;
	struct timeval last_fan_adj;
	struct timeval last_stratum;
	struct timeval last_detect;

	cglock_t update_lock;

	struct pool pool0;
	struct pool pool1;
	struct pool pool2;

	bool work_restart;

	uint32_t last_jobid;

	/* For connecter */
	char auc_version[AVA8_AUC_VER_LEN + 1];

	int auc_speed;
	int auc_xdelay;
	int auc_sensor;

	struct i2c_ctx *i2c_slaves[AVA8_DEFAULT_MODULARS];

	uint8_t connecter; /* AUC or IIC */

	/* For modulars */
	bool enable[AVA8_DEFAULT_MODULARS];
	bool reboot[AVA8_DEFAULT_MODULARS];

	struct timeval elapsed[AVA8_DEFAULT_MODULARS];

	uint8_t mm_dna[AVA8_DEFAULT_MODULARS][AVA8_MM_DNA_LEN];
	char mm_version[AVA8_DEFAULT_MODULARS][AVA8_MM_VER_LEN + 1]; /* It's a string */
	uint32_t total_asics[AVA8_DEFAULT_MODULARS];
	uint32_t max_ntime; /* Maximum: 7200 */

	int mod_type[AVA8_DEFAULT_MODULARS];
	uint8_t miner_count[AVA8_DEFAULT_MODULARS];
	uint8_t asic_count[AVA8_DEFAULT_MODULARS];

	uint32_t freq_mode[AVA8_DEFAULT_MODULARS];
	int led_indicator[AVA8_DEFAULT_MODULARS];
	int fan_pct[AVA8_DEFAULT_MODULARS];
	int fan_cpm[AVA8_DEFAULT_MODULARS];

	int temp[AVA8_DEFAULT_MODULARS][AVA8_DEFAULT_MINER_CNT][5];
	int temp_mm[AVA8_DEFAULT_MODULARS];

	uint8_t cutoff[AVA8_DEFAULT_MODULARS];
	int temp_target[AVA8_DEFAULT_MODULARS];
	int temp_last_max[AVA8_DEFAULT_MODULARS];
	int temp_overheat[AVA8_DEFAULT_MODULARS];
	time_t last_temp_time[AVA8_DEFAULT_MODULARS];

	uint32_t set_voltage[AVA8_DEFAULT_MODULARS][AVA8_DEFAULT_MINER_CNT];
	uint32_t set_frequency[AVA8_DEFAULT_MODULARS][AVA8_DEFAULT_MINER_CNT][AVA8_DEFAULT_PLL_CNT];

	uint16_t get_vin[AVA8_DEFAULT_MODULARS][AVA8_DEFAULT_MINER_CNT];
	uint32_t get_voltage[AVA8_DEFAULT_MODULARS][AVA8_DEFAULT_MINER_CNT];
	uint32_t get_pll[AVA8_DEFAULT_MODULARS][AVA8_DEFAULT_MINER_CNT][AVA8_DEFAULT_PLL_CNT];

	int8_t factory_info[AVA8_DEFAULT_FACTORY_INFO_CNT];

	uint64_t local_works[AVA8_DEFAULT_MODULARS];
	uint64_t local_works_i[AVA8_DEFAULT_MODULARS][AVA8_DEFAULT_MINER_CNT];
	uint64_t hw_works[AVA8_DEFAULT_MODULARS];
	uint64_t hw_works_i[AVA8_DEFAULT_MODULARS][AVA8_DEFAULT_MINER_CNT];
	uint64_t chip_matching_work[AVA8_DEFAULT_MODULARS][AVA8_DEFAULT_MINER_CNT][AVA8_DEFAULT_ASIC_MAX];

	uint32_t error_code[AVA8_DEFAULT_MODULARS][AVA8_DEFAULT_MINER_CNT + 1];
	uint32_t error_crc[AVA8_DEFAULT_MODULARS][AVA8_DEFAULT_MINER_CNT];
	uint8_t error_polling_cnt[AVA8_DEFAULT_MODULARS];

	uint8_t power_good[AVA8_DEFAULT_MODULARS];
	char pmu_version[AVA8_DEFAULT_MODULARS][AVA8_DEFAULT_PMU_CNT][5];
	uint64_t diff1[AVA8_DEFAULT_MODULARS];

	uint16_t vin_adc_ratio[AVA8_DEFAULT_MODULARS];
	uint16_t vout_adc_ratio[AVA8_DEFAULT_MODULARS];

	bool conn_overloaded;
};

struct avalon8_iic_info {
	uint8_t iic_op;
	union {
		uint32_t aucParam[2];
		uint8_t slave_addr;
	} iic_param;
};

struct avalon8_dev_description {
	uint8_t dev_id_str[8];
	int mod_type;
	uint8_t miner_count; /* it should not greater than AVA8_DEFAULT_MINER_CNT */
	uint8_t asic_count; /* asic count each miner, it should not great than AVA8_DEFAULT_ASIC_MAX */
	uint16_t vin_adc_ratio;
	uint16_t vout_adc_ratio;
	uint32_t set_voltage;
};

#define AVA8_WRITE_SIZE (sizeof(struct avalon8_pkg))
#define AVA8_READ_SIZE AVA8_WRITE_SIZE

#define AVA8_SEND_OK 0
#define AVA8_SEND_ERROR -1

extern char *set_avalon8_fan(char *arg);
extern char *set_avalon8_freq(char *arg);
extern char *set_avalon8_voltage(char *arg);
extern char *set_avalon8_voltage_level(char *arg);
extern char *set_avalon8_voltage_offset(char *arg);
extern int opt_avalon8_temp_target;
extern int opt_avalon8_polling_delay;
extern int opt_avalon8_aucspeed;
extern int opt_avalon8_aucxdelay;
extern int opt_avalon8_smart_speed;
extern bool opt_avalon8_iic_detect;
extern int opt_avalon8_freq_sel;
extern uint32_t opt_avalon8_th_pass;
extern uint32_t opt_avalon8_th_fail;
extern uint32_t opt_avalon8_th_init;
extern uint32_t opt_avalon8_th_ms;
extern uint32_t opt_avalon8_th_timeout;
extern uint32_t opt_avalon8_nonce_mask;
#endif /* USE_AVALON8 */
#endif	/* _AVALON8_H_ */
