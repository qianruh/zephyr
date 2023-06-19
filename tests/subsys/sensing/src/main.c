/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/ztest.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensing_sensor.h>

#define DT_DRV_COMPAT zephyr_sensing
#define DT_SENSOR_INFO(node)				\
	{							\
		.type = DT_PROP(node, sensor_type),				\
		.name = DT_NODE_FULL_NAME(node),			\
		.friendly_name = DT_PROP(node, friendly_name)				\
	}

struct sensor_info_t {
	int32_t type;
	const char *name;
	const char *friendly_name;
};

static struct sensor_info_t sensors[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_DRV_INST(0), DT_SENSOR_INFO, (,))
};

static bool lookup_sensor_in_dt(int32_t type, const char *name, const char *friendly_name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sensors); ++i) {
		if (sensors[i].type == type &&
				strcmp(sensors[i].name, name) &&
				strcmp(sensors[i].friendly_name, friendly_name)) {
			return true;
		}
	}

	return false;
}

/**
 * @brief Test Get Sensors
 *
 * This test verifies sensing_get_sensors.
 */
ZTEST(sensing_tests, test_sensing_get_sensors)
{
	const struct sensing_sensor_info *info;
	int ret, i, num = 0;

	/* Check the return value */
	ret = sensing_get_sensors(&num, &info);
	zassert_equal(ret, 0, "Sensing Get Sensors failed");
	zassert_equal(num, ARRAY_SIZE(sensors), "Sensors num mismatch");
	zassert_not_null(info, "Null info");

	for (i = 0; i < num; ++i) {
	/* Check the content */
	zassert_true(lookup_sensor_in_dt(info[i].type, info[i].name,
					info[i].friendly_name), "sensor info mismatch");
    }
}

ZTEST_SUITE(sensing_tests, NULL, NULL, NULL, NULL, NULL);