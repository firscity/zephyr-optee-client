/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <optee_msg_supplicant.h>
#include <tee_client_api.h>

#include <zephyr/drivers/tee.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tee_supplicant);

#define TEE_SUPP_THREAD_PRIO	7

#define TEE_REQ_PARAM_MAX	5

static struct k_thread main_thread;
static K_THREAD_STACK_DEFINE(main_stack, 8192);

struct tee_supp_request {
	uint32_t cmd;
	uint32_t num_param;
	struct tee_param params[TEE_REQ_PARAM_MAX];
};

struct tee_supp_response {
	uint32_t ret;
	uint32_t num_param;
	struct tee_param params[TEE_REQ_PARAM_MAX];
};

union tee_supp_msg {
	struct tee_supp_request req;
	struct tee_supp_response rsp;
};

static int receive_request(const struct device *dev, struct tee_supp_request *ts_req)
{
	LOG_ERR("Receiving request...");
	int rc = tee_suppl_recv(dev, &ts_req->cmd, &ts_req->num_param, ts_req->params);
	if (rc) {
		LOG_ERR("TEE supplicant receive failed, rc = %d", rc);
	}

	LOG_ERR("Received request");
	return rc;
}

static int send_response(const struct device *dev, struct tee_supp_response *rsp)
{
	LOG_ERR("Sending response...");
	int rc = tee_suppl_send(dev, rsp->ret, rsp->num_param, rsp->params);
	if (rc) {
		LOG_ERR("TEE supplicant send response failed, rc = %d", rc);
	}

	LOG_ERR("Sent response");
	return rc;
}

static int load_ta(uint32_t num_params, struct tee_param *params)
{
	return 0;
}

static int process_request(const struct device *dev)
{
	int rc;
	union tee_supp_msg ts_msg = {
		.req.num_param = TEE_REQ_PARAM_MAX,
	};

	rc = receive_request(dev, &ts_msg.req);
	if (rc) {
		return rc;
	}

	LOG_INF("Receive OPTEE request cmd #%d", ts_msg.req.cmd);
	switch (ts_msg.req.cmd) {
	case OPTEE_MSG_RPC_CMD_LOAD_TA:
		rc = load_ta(ts_msg.req.num_param, ts_msg.req.params);
		break;
	default:
		return TEEC_ERROR_NOT_SUPPORTED;
	}

	ts_msg.rsp.ret = rc;
	return send_response(dev, &ts_msg.rsp);
}

static void tee_supp_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	const struct device *dev = p1;
	int rc = 0;

	while (1) {
		rc = process_request(dev);
		if (rc) {
			LOG_ERR("Failed to process request, rc = %d", rc);
			break;
		}
	}
}

int tee_supp_init(const struct device *dev)
{
	const struct device *tee_dev = DEVICE_DT_GET_ONE(linaro_optee_tz);

	if (!tee_dev) {
		LOG_ERR("No TrustZone device found!");
		return -ENODEV;
	}

	k_thread_create(&main_thread, main_stack, K_THREAD_STACK_SIZEOF(main_stack), tee_supp_main,
			(void *) tee_dev, NULL, NULL, TEE_SUPP_THREAD_PRIO, 0, K_NO_WAIT);

	LOG_INF("Started tee_supplicant thread");
	return 0;
}

SYS_INIT(tee_supp_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
