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
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test_sensing, LOG_LEVEL_INF);

#define INTERVAL_10HZ (100 * USEC_PER_MSEC)
#define INTERVAL_20HZ (50 * USEC_PER_MSEC)
#define INTERVAL_40HZ (25 * USEC_PER_MSEC)
#define INVALID_INTERVAL_US 1 /* 1 us is smaller than the minimal interval */

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

static void acc_data_event_callback(sensing_sensor_handle_t handle, const void *buf)
{
	const struct sensing_sensor_info *info = sensing_get_sensor_info(handle);
	ARG_UNUSED(buf);

	zassert_equal(info->type, SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D,
			"Sensor type mismatch");
}

/**
 * @brief Test Open Sensor
 *
 * This test verifies sensing_open_sensor.
 */
ZTEST(sensing_tests, test_sensing_open_sensor)
{
	const struct sensing_callback_list acc_0_cb_list = {
		.on_data_event = &acc_data_event_callback,
	};
	const struct sensing_callback_list acc_1_cb_list = {
		.on_data_event = &acc_data_event_callback,
	};
	const struct sensing_sensor_info *info;
	sensing_sensor_handle_t acc_0;
	sensing_sensor_handle_t acc_1;
	int ret, num = 0;

	ret = sensing_get_sensors(&num, &info);
	zassert_equal(ret, 0, "Sensing Get Sensors failed");
	zassert_equal(num, ARRAY_SIZE(sensors), "Sensors num mismatch");
	zassert_not_null(info, "Null info");

	ret = sensing_open_sensor(&info[0], &acc_0_cb_list, &acc_0);
	zassert_equal(ret, 0, "Open ACC 0 failed");

	ret = sensing_open_sensor(&info[1], &acc_1_cb_list, &acc_1);
	zassert_equal(ret, 0, "Open Lid ACC failed");

	/* negative test */
	ret = sensing_open_sensor(NULL, &acc_1_cb_list, &acc_1);
	zassert_true(ret < 0, "Negative test failed");

	ret = sensing_open_sensor(&info[1], NULL, &acc_1);
	zassert_true(ret < 0, "Negative test failed");

	ret = sensing_open_sensor(&info[1], &acc_1_cb_list, NULL);
	zassert_true(ret < 0, "Negative test failed");
}

/**
 * @brief Test Open Sensor By dt
 *
 * This test verifies sensing_open_sensor_by_dt.
 */
ZTEST(sensing_tests, test_sensing_open_sensor_and_by_dt)
{
	const struct sensing_callback_list acc_0_cb_list = {
		.on_data_event = &acc_data_event_callback,
	};
	const struct sensing_callback_list acc_1_cb_list = {
		.on_data_event = &acc_data_event_callback,
	};
	sensing_sensor_handle_t acc_0;
	sensing_sensor_handle_t acc_1;
	int ret = 0;

	ret = sensing_open_sensor_by_dt(DEVICE_DT_GET(DT_NODELABEL(base_accel)),
					&acc_0_cb_list,
					&acc_0);
	zassert_equal(ret, 0, "Open ACC 0 By dt failed");

	ret = sensing_open_sensor_by_dt(DEVICE_DT_GET(DT_NODELABEL(lid_accel)),
					&acc_1_cb_list,
					&acc_1);
	zassert_equal(ret, 0, "Open Lid ACC By dt failed");

	/* negative test */
	ret = sensing_open_sensor_by_dt(NULL, &acc_1_cb_list, &acc_1);
	zassert_true(ret < 0, "Negative test failed");

	ret = sensing_open_sensor_by_dt(DEVICE_DT_GET(DT_NODELABEL(lid_accel)), NULL, &acc_1);
	zassert_true(ret < 0, "Negative test failed");

	ret = sensing_open_sensor_by_dt(DEVICE_DT_GET(DT_NODELABEL(lid_accel)), &acc_1_cb_list, NULL);
	zassert_true(ret < 0, "Negative test failed");
}

/**
 * @brief Test Close Sensor
 *
 * This test verifies sensing_close_sensor
 */
ZTEST(sensing_tests, test_sensing_close_sensor)
{
	const struct sensing_callback_list acc_0_cb_list = {
		.on_data_event = &acc_data_event_callback,
	};
	const struct sensing_sensor_info *info;
	sensing_sensor_handle_t acc_0;
	int ret, num = 0;

	ret = sensing_get_sensors(&num, &info);
	zassert_equal(ret, 0, "Sensing Get Sensors failed");
	zassert_equal(num, ARRAY_SIZE(sensors), "Sensors num mismatch");
	zassert_not_null(info, "Null info");

	ret = sensing_open_sensor(&info[0], &acc_0_cb_list, &acc_0);
	zassert_equal(ret, 0, "Open ACC 0 failed");

	ret = sensing_close_sensor(&acc_0);
	zassert_equal(ret, 0, "Close ACC 0 failed");

	ret = sensing_open_sensor_by_dt(DEVICE_DT_GET(DT_NODELABEL(base_accel)),
					&acc_0_cb_list,
					&acc_0);
	zassert_equal(ret, 0, "Open ACC 0 By dt failed");

	ret = sensing_close_sensor(&acc_0);
	zassert_equal(ret, 0, "Close ACC 0 failed");

	/* negative test */
	ret = sensing_close_sensor(NULL);
	zassert_true(ret < 0, "Negative test failed");
}

/**
 * @brief Test Sensor Set Config
 *
 * This test verifies sensing_set_config
 */
ZTEST(sensing_tests, test_sensing_set_config)
{
	const struct sensing_callback_list acc_0_cb_list = {
		.on_data_event = &acc_data_event_callback,
	};
	const struct sensing_sensor_info *info;
	sensing_sensor_handle_t acc_0;
	struct sensing_sensor_config acc_0_config;
	int ret, num = 0;

	ret = sensing_get_sensors(&num, &info);
	ret = sensing_open_sensor(&info[0], &acc_0_cb_list, &acc_0);
	zassert_equal(ret, 0, "Open ACC 0 failed");

	/* This test verifies sensing_set_interval */
	acc_0_config.attri = SENSING_SENSOR_ATTRIBUTE_INTERVAL;
	acc_0_config.interval = INTERVAL_10HZ;

	ret = sensing_set_config(acc_0, &acc_0_config, 1);
	zassert_equal(ret, 0, "Set acc_0 0 interval 100 ms failed");

	acc_0_config.interval = INTERVAL_20HZ;

	ret = sensing_set_config(acc_0, &acc_0_config, 1);
	zassert_equal(ret, 0, "Set acc_0 0 interval 50 ms failed");

	acc_0_config.interval = INVALID_INTERVAL_US;
	ret = sensing_set_config(acc_0, &acc_0_config, 1);
	zassert_true(ret < 0, "Negative test failed");

	/* This test verifies sensing_set_sensitivity */
	acc_0_config.attri = SENSING_SENSOR_ATTRIBUTE_SENSITIVITY;
	acc_0_config.sensitivity = 100;
	ret = sensing_set_config(acc_0, &acc_0_config, 1);
	zassert_true(ret < 0, "Negative test failed");

	acc_0_config.attri = SENSING_SENSOR_ATTRIBUTE_SENSITIVITY;
	acc_0_config.data_field = 0;
	acc_0_config.sensitivity = 100;
	ret = sensing_set_config(acc_0, &acc_0_config, 1);
	zassert_equal(ret, 0, "Set acc_0 index 0 sensitivity 100 failed");

	acc_0_config.data_field = 2;
	acc_0_config.sensitivity = 50;
	ret = sensing_set_config(acc_0, &acc_0_config, 1);
	zassert_equal(ret, 0, "Set acc_0 index 2 sensitivity 50 failed");

	acc_0_config.data_field = SENSING_SENSITIVITY_INDEX_ALL;
	acc_0_config.sensitivity = 100;
	ret = sensing_set_config(acc_0, &acc_0_config, 1);
	zassert_equal(ret, 0, "Set acc_0 all index sensitivity 100 failed");

	acc_0_config.data_field = -2;
	ret = sensing_set_config(acc_0, &acc_0_config, 1);
	zassert_true(ret < 0, "Negative test failed");

	acc_0_config.data_field = 3;
	ret = sensing_set_config(acc_0, &acc_0_config, 1);
	zassert_true(ret < 0, "Negative test failed");

	/* This test verifies sensing set sensitivity and interval at the same time */
	struct sensing_sensor_config cfgs[2];
	cfgs[0].attri = SENSING_SENSOR_ATTRIBUTE_SENSITIVITY;
	cfgs[0].data_field = SENSING_SENSITIVITY_INDEX_ALL;
	cfgs[0].sensitivity = 100;

	cfgs[1].attri = SENSING_SENSOR_ATTRIBUTE_INTERVAL;
	cfgs[1].interval = INTERVAL_10HZ;

	ret = sensing_set_config(acc_0, cfgs, 2);
	zassert_equal(ret, 0, "Set acc_0 all index sensitivity 100 and interval 100 ms failed");

	/* Negative tests */
	ret = sensing_set_config(acc_0, NULL, 2);
	zassert_true(ret < 0, "Negative test failed");

	ret = sensing_set_config(acc_0, cfgs, -1);
	zassert_true(ret < 0, "Negative test failed");

	ret = sensing_set_config(acc_0, cfgs, 3);
	zassert_true(ret < 0, "Negative test failed");

	ret = sensing_set_config(NULL, cfgs, 2);
	zassert_true(ret < 0, "Negative test failed");

	ret = sensing_close_sensor(&acc_0);
	zassert_equal(ret, 0, "Close ACC 0 failed");
}

/**
 * @brief Test Sensor Get Config
 *
 * This test verifies sensing_get_config
 */
ZTEST(sensing_tests, test_sensing_get_config)
{
	const struct sensing_callback_list acc_0_cb_list = {
		.on_data_event = &acc_data_event_callback,
	};
	const struct sensing_sensor_info *info;
	sensing_sensor_handle_t acc_0;
	struct sensing_sensor_config cfgs[2];
	int ret, num = 0;

	ret = sensing_get_sensors(&num, &info);
	ret = sensing_open_sensor(&info[0], &acc_0_cb_list, &acc_0);
	zassert_equal(ret, 0, "Open ACC 0 failed");

	ret = sensing_get_config(acc_0, cfgs, 2);
	zassert_equal(ret, 0, "sensing_get_config failed");
	zassert_equal(cfgs[0].interval, 0, "value is not equal to 0");
	zassert_equal(cfgs[0].sensitivity, 0, "value is not equal to 0");
	zassert_equal(cfgs[1].interval, 0, "value is not equal to 0");
	zassert_equal(cfgs[1].sensitivity, 0, "value is not equal to 0");

	cfgs[0].attri = SENSING_SENSOR_ATTRIBUTE_INTERVAL;
	cfgs[0].interval = INTERVAL_10HZ;
	cfgs[1].attri = SENSING_SENSOR_ATTRIBUTE_SENSITIVITY;
	cfgs[1].data_field = SENSING_SENSITIVITY_INDEX_ALL;
	cfgs[1].sensitivity = 100;

	ret = sensing_set_config(acc_0, cfgs, 2);
	zassert_equal(ret, 0, "ACC 0 sensing_set_config failed");

	cfgs[0].interval = 0;
	cfgs[1].sensitivity = 0;

	ret = sensing_get_config(acc_0, cfgs, 2);
	zassert_equal(ret, 0, "sensing_get_config failed");
	zassert_equal(cfgs[0].interval, INTERVAL_10HZ, "interval is not correct");
	zassert_equal(cfgs[1].sensitivity, 100, "sensitivity is not correct");

	ret = sensing_get_config(NULL, cfgs, 2);
	zassert_true(ret < 0, "Negative test failed");

	ret = sensing_get_config(acc_0, NULL, 3);
	zassert_true(ret < 0, "Negative test failed");

	ret = sensing_get_config(acc_0, cfgs, 3);
	zassert_true(ret < 0, "Negative test failed");

	ret = sensing_close_sensor(&acc_0);
	zassert_equal(ret, 0, "Close ACC 0 failed");
}

static atomic_t acc_0_samples;
static atomic_t acc_1_samples;

static void acc_0_callback(sensing_sensor_handle_t handle, const void *buf)
{
	const struct sensing_sensor_info *info = sensing_get_sensor_info(handle);
	ARG_UNUSED(buf);

	zassert_equal(info->type, SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D,
			"Sensor type mismatch");

	atomic_inc(&acc_0_samples);
}

/**
 * @brief Test ACC 0
 *
 * This test verifies the ACC 0
 */
ZTEST(sensing_tests, test_acc_0)
{
	const struct sensing_callback_list acc_0_cb_list = {
		.on_data_event = &acc_0_callback,
	};
	const struct sensing_sensor_info *info;
	sensing_sensor_handle_t acc_0;
	struct sensing_sensor_config acc_0_config;
	int ret, num = 0;
	int elapse;
	int expect;

	ret = sensing_get_sensors(&num, &info);
	ret = sensing_open_sensor(&info[0], &acc_0_cb_list, &acc_0);
	zassert_equal(ret, 0, "Open ACC 0 failed");

	/* Test 10 Hz */
	acc_0_config.attri = SENSING_SENSOR_ATTRIBUTE_INTERVAL;
	acc_0_config.interval = INTERVAL_10HZ;
	atomic_set(&acc_0_samples, 0);

	ret = sensing_set_config(acc_0, &acc_0_config, 1);
	zassert_equal(ret, 0, "Set ACC 0 interval 100 ms failed");

	elapse = 10 * USEC_PER_SEC;
	expect = elapse / acc_0_config.interval;
	k_sleep(K_SECONDS(10));

	LOG_INF("ACC 0 Samples: %d expect: %d",
			atomic_get(&acc_0_samples), expect);
	zassert_within(atomic_get(&acc_0_samples), expect,
			1, "ACC 0 samples num out of range");

	/* Test 20 Hz */
	acc_0_config.interval = INTERVAL_10HZ;
	atomic_set(&acc_0_samples, 0);
	ret = sensing_set_config(acc_0, &acc_0_config, 1);
	zassert_equal(ret, 0, "Set ACC 0 interval 50 ms failed");

	elapse = 10 * USEC_PER_SEC;
	expect = elapse / acc_0_config.interval;
	k_sleep(K_SECONDS(10));

	LOG_INF("ACC 0 Samples: %d expect: %d",
			atomic_get(&acc_0_samples), expect);
	zassert_within(atomic_get(&acc_0_samples), expect,
			1, "ACC 0 samples num out of range");

	acc_0_config.interval = 0;
	ret = sensing_set_config(acc_0, &acc_0_config, 1);
	zassert_equal(ret, 0, "ACC 0 stop streaming failed");

	ret = sensing_close_sensor(&acc_0);
	zassert_equal(ret, 0, "Close ACC 0 failed");
}

static void acc_1_callback(sensing_sensor_handle_t handle, const void *buf)
{
	const struct sensing_sensor_info *info = sensing_get_sensor_info(handle);
	ARG_UNUSED(buf);

	zassert_equal(info->type, SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D,
			"Sensor type mismatch");

	atomic_inc(&acc_1_samples);
}

/**
 * @brief Test ACC 0 and 1 parallel
 *
 * This test verifies the ACC 0 and 1 run parallel
 */
ZTEST(sensing_tests, test_acc_0_and_acc_1)
{
	const struct sensing_callback_list acc_0_cb_list = {
		.on_data_event = &acc_0_callback,
	};

	const struct sensing_callback_list acc_1_cb_list = {
		.on_data_event = &acc_1_callback,
	};

	const struct sensing_sensor_info *info;
	sensing_sensor_handle_t acc_0;
	sensing_sensor_handle_t acc_1;
	struct sensing_sensor_config acc_0_config;
	struct sensing_sensor_config acc_1_config;
	int ret, num = 0;
	int elapse_0, elapse_1;
	int expect_0, expect_1;

	ret = sensing_get_sensors(&num, &info);
	zassert_equal(ret, 0, "sensing_get_sensors failed");

	/* Open ACC 0 */
	ret = sensing_open_sensor(&info[0], &acc_0_cb_list, &acc_0);
	zassert_equal(ret, 0, "Open ACC 0 failed");

	/* Open ACC 1 */
	ret = sensing_open_sensor(&info[1], &acc_1_cb_list, &acc_1);
	zassert_equal(ret, 0, "Open ACC 1 failed");

	/* Test 10 Hz */
	acc_0_config.attri = SENSING_SENSOR_ATTRIBUTE_INTERVAL;
	acc_0_config.interval = INTERVAL_10HZ;
	atomic_set(&acc_0_samples, 0);

	ret = sensing_set_config(acc_0, &acc_0_config, 1);
	zassert_equal(ret, 0, "Set ACC 0 interval 100 ms failed");

	elapse_0 = 10 * USEC_PER_SEC;
	expect_0 = elapse_0 / acc_0_config.interval;
	k_sleep(K_SECONDS(10));

	LOG_INF("ACC 0 Samples: %d expect: %d",
			atomic_get(&acc_0_samples), expect_0);
	zassert_within(atomic_get(&acc_0_samples), expect_0,
			1, "ACC 0 samples num out of range");

	/* Test 20 Hz */
	acc_1_config.attri = SENSING_SENSOR_ATTRIBUTE_INTERVAL;
	acc_1_config.interval = INTERVAL_20HZ;
	atomic_set(&acc_1_samples, 0);

	ret = sensing_set_config(acc_1, &acc_1_config, 1);
	zassert_equal(ret, 0, "Set ACC 1 interval 50 ms failed");

	elapse_1 = 10 * USEC_PER_SEC;
	expect_1 = elapse_1 / acc_1_config.interval;
	elapse_0 += 10 * USEC_PER_SEC;
	expect_0 = elapse_0 / acc_0_config.interval;
	k_sleep(K_SECONDS(10));

	LOG_INF("ACC 0 Samples: %d expect: %d",
			atomic_get(&acc_0_samples), expect_0);
	zassert_within(atomic_get(&acc_0_samples), expect_0,
			1, "ACC 0 samples num out of range");

	LOG_INF("ACC 1 Samples: %d expect: %d",
			atomic_get(&acc_1_samples), expect_1);
	zassert_within(atomic_get(&acc_1_samples), expect_1,
			1, "ACC 1 samples num out of range");

	acc_0_config.interval = 0;
	ret = sensing_set_config(acc_0, &acc_0_config, 1);
	zassert_equal(ret, 0, "ACC 0 stop streaming failed");

	acc_1_config.interval = 0;
	ret = sensing_set_config(acc_1, &acc_1_config, 1);
	zassert_equal(ret, 0, "ACC 1 stop streaming failed");

	k_sleep(K_SECONDS(2));

	ret = sensing_close_sensor(&acc_0);
	zassert_equal(ret, 0, "Close ACC 0 failed");

	ret = sensing_close_sensor(&acc_1);
	zassert_equal(ret, 0, "Close ACC 1 failed");

	k_sleep(K_SECONDS(2));

	LOG_INF("ACC 0 Samples: %d expect: %d",
			atomic_get(&acc_0_samples), expect_0);
	zassert_within(atomic_get(&acc_0_samples), expect_0,
			1, "ACC 0 samples num out of range");

	LOG_INF("ACC 1 Samples: %d expect: %d",
			atomic_get(&acc_1_samples), expect_1);
	zassert_within(atomic_get(&acc_1_samples), expect_1,
			1, "ACC 1 samples num out of range");
}

static atomic_t user_0_samples;
static atomic_t user_1_samples;

static void user_0_callback(sensing_sensor_handle_t handle, const void *buf)
{
	const struct sensing_sensor_info *info = sensing_get_sensor_info(handle);
	ARG_UNUSED(buf);

	zassert_equal(info->type, SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D,
			"Sensor type mismatch");

	atomic_inc(&user_0_samples);
}

static void user_1_callback(sensing_sensor_handle_t handle, const void *buf)
{
	const struct sensing_sensor_info *info = sensing_get_sensor_info(handle);
	ARG_UNUSED(buf);

	zassert_equal(info->type, SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D,
			"Sensor type mismatch");

	atomic_inc(&user_1_samples);
}

/**
 * @brief Test multiple instances of ACC 0
 *
 * This test verifies two users use ACC 0 at the same time.
 */

ZTEST(sensing_tests, test_acc_0_with_2_users)
{
	const struct sensing_callback_list user_0_cb_list = {
		.on_data_event = &user_0_callback,
	};

	const struct sensing_callback_list user_1_cb_list = {
		.on_data_event = &user_1_callback,
	};

	const struct sensing_sensor_info *info;
	sensing_sensor_handle_t user_0;
	sensing_sensor_handle_t user_1;
	struct sensing_sensor_config acc_0_config;
	int ret, num = 0;
	int elapse_0, elapse_1;
	int expect_0, expect_1;

	ret = sensing_get_sensors(&num, &info);
	zassert_equal(ret, 0, "sensing_get_sensors failed");

	/* User 0 Open ACC 0 */
	ret = sensing_open_sensor(&info[0], &user_0_cb_list, &user_0);
	zassert_equal(ret, 0, "Open ACC 0 failed");

	/* User 1 Open ACC 0 */
	ret = sensing_open_sensor(&info[0], &user_1_cb_list, &user_1);
	zassert_equal(ret, 0, "Open ACC 0 failed");

	/* Test 10 Hz */
	acc_0_config.attri = SENSING_SENSOR_ATTRIBUTE_INTERVAL;
	acc_0_config.interval = INTERVAL_10HZ;
	atomic_set(&user_0_samples, 0);

	ret = sensing_set_config(user_0, &acc_0_config, 1);
	zassert_equal(ret, 0, "Set ACC 0 interval 100 ms failed");

	elapse_0 = 10 * USEC_PER_SEC;
	expect_0 = elapse_0 / acc_0_config.interval;
	k_sleep(K_SECONDS(10));

	LOG_INF("User 0 Samples: %d expect: %d",
			atomic_get(&user_0_samples), expect_0);
	zassert_within(atomic_get(&user_0_samples), expect_0,
			1, "User 0 samples num out of range");

	/* Test 20 Hz */
	acc_0_config.interval = INTERVAL_20HZ;
	atomic_set(&user_1_samples, 0);

	ret = sensing_set_config(user_1, &acc_0_config, 1);
	zassert_equal(ret, 0, "User 1 set ACC interval 50 ms failed");

	elapse_1 = 10 * USEC_PER_SEC;
	expect_1 = elapse_1 / INTERVAL_20HZ;
	elapse_0 += 10 * USEC_PER_SEC;
	expect_0 = elapse_0 / INTERVAL_10HZ;
	k_sleep(K_SECONDS(10));

	LOG_INF("User 0 Samples: %d expect: %d",
			atomic_get(&user_0_samples), expect_0);
	zassert_within(atomic_get(&user_0_samples), expect_0,
			1, "User 0 samples num out of range");

	LOG_INF("User 1 Samples: %d expect: %d",
			atomic_get(&user_1_samples), expect_1);
	zassert_within(atomic_get(&user_1_samples), expect_1,
			1, "User 1 samples num out of range");

	acc_0_config.interval = 0;
	ret = sensing_set_config(user_0, &acc_0_config, 1);
	zassert_equal(ret, 0, "User 0 stop streaming failed");

	ret = sensing_close_sensor(&user_0);
	zassert_equal(ret, 0, "Close user 0 failed");

	ret = sensing_set_config(user_1, &acc_0_config, 1);
	zassert_equal(ret, 0, "User 1 stop streaming failed");

	ret = sensing_close_sensor(&user_1);
	zassert_equal(ret, 0, "Close user 1 failed");

	k_sleep(K_SECONDS(2));

	LOG_INF("User 0 Samples: %d expect: %d",
			atomic_get(&user_0_samples), expect_0);
	zassert_within(atomic_get(&user_0_samples), expect_0,
			1, "User 0samples num out of range");

	LOG_INF("User 1 Samples: %d expect: %d",
			atomic_get(&user_1_samples), expect_1);
	zassert_within(atomic_get(&user_1_samples), expect_1,
			1, "User 1 samples num out of range");
}

/**
 * @brief Test set arbitrate interval of ACC0, ACC1, ACC2
 *
 * This test verifies arbitrate ACC0, ACC1, ACC2 interval.
 */
ZTEST(sensing_tests, test_acc_0_1_2_interval_arbitrate)
{

	const struct sensing_callback_list acc_0_cb_list = {
		.on_data_event = &acc_data_event_callback,
	};

	const struct sensing_callback_list acc_1_cb_list = {
		.on_data_event = &acc_data_event_callback,
	};

	const struct sensing_callback_list acc_2_cb_list = {
		.on_data_event = &acc_data_event_callback,
	};

	const struct sensing_sensor_info *info;
	sensing_sensor_handle_t acc_0;
	sensing_sensor_handle_t acc_1;
	sensing_sensor_handle_t acc_2;
	struct sensing_sensor_config acc_0_config;
	struct sensing_sensor_config acc_1_config;
	struct sensing_sensor_config acc_2_config;
	const struct sensing_sensor_api *api;
	const struct device *dev;
	int ret, num = 0;
	uint32_t value;

	dev = DEVICE_DT_GET(DT_INST(0, zephyr_sensing_phy_3d_sensor));

	ret = sensing_get_sensors(&num, &info);
	zassert_equal(ret, 0, "sensing_get_sensors failed");

	/* Open ACC 0 */
	ret = sensing_open_sensor(&info[0], &acc_0_cb_list, &acc_0);
	zassert_equal(ret, 0, "Open ACC 0 failed");

	/* Open ACC 1 */
	ret = sensing_open_sensor(&info[0], &acc_1_cb_list, &acc_1);
	zassert_equal(ret, 0, "Open ACC 1 failed");

	/* Open ACC 2 */
	ret = sensing_open_sensor(&info[0], &acc_2_cb_list, &acc_2);
	zassert_equal(ret, 0, "Open ACC 2 failed");

	/* Set ACC 0, 1, 2 interval 10, 20, 40hz */
	acc_0_config.attri = SENSING_SENSOR_ATTRIBUTE_INTERVAL;
	acc_0_config.interval = INTERVAL_10HZ;

	ret = sensing_set_config(acc_0, &acc_0_config, 1);
	zassert_equal(ret, 0, "Set ACC 0 interval failed");

	acc_1_config.attri = SENSING_SENSOR_ATTRIBUTE_INTERVAL;
	acc_1_config.interval = INTERVAL_20HZ;

	ret = sensing_set_config(acc_1, &acc_1_config, 1);
	zassert_equal(ret, 0, "Set ACC 0 interval failed");

	acc_2_config.attri = SENSING_SENSOR_ATTRIBUTE_INTERVAL;
	acc_2_config.interval = INTERVAL_40HZ;

	ret = sensing_set_config(acc_2, &acc_2_config, 1);
	zassert_equal(ret, 0, "Set ACC 2 interval failed");

	LOG_INF("Sleep a while for sensing to arbitrate and set interval");
	k_sleep(K_MSEC(200));

	/* Test ACC 0, 1, 2 arbitrate interval */
	api = dev->api;
	value = 0;
	ret = api->get_interval(dev, &value);
	zassert_equal(ret, 0, "Get arbitrate interval failed");
	zassert_equal(value, INTERVAL_40HZ, "Value is not equal to INTERVAL_40HZ");

	/* Test whether arbitrate interval work well when set ACC 2 interval 0 */
	acc_2_config.interval = 0;
	ret = sensing_set_config(acc_2, &acc_2_config, 1);
	zassert_equal(ret, 0, "Set ACC 2 interval failed");

	LOG_INF("Sleep a while for sensing to arbitrate and set interval");
	k_sleep(K_MSEC(200));

	ret = api->get_interval(dev, &value);
	zassert_equal(ret, 0, "Get arbitrate interval failed");
	zassert_equal(value, INTERVAL_20HZ, "Value is not equal to INTERVAL_20HZ");

	/* Test whether arbitrate interval work well when close ACC 1 */
	ret = sensing_close_sensor(&acc_1);
	zassert_equal(ret, 0, "Close ACC 1 failed");

	LOG_INF("Sleep a while for sensing to arbitrate and set interval");
	k_sleep(K_MSEC(200));

	ret = api->get_interval(dev, &value);
	zassert_equal(ret, 0, "Get arbitrate interval failed");
	zassert_equal(value, INTERVAL_10HZ, "Value is not equal to INTERVAL_10HZ");

	ret = sensing_close_sensor(&acc_0);
	zassert_equal(ret, 0, "Close ACC 0 failed");

	ret = sensing_close_sensor(&acc_2);
	zassert_equal(ret, 0, "Close ACC 2 failed");
}

ZTEST_SUITE(sensing_tests, NULL, NULL, NULL, NULL, NULL);