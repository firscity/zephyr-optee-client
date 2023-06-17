# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2023 EPAM Systems
# Author: Volodymyr Babchuk <volodymyr_babchuk@epam.com>

if (CONFIG_OPTEE_CLIENT)
    zephyr_include_directories(public)
endif()

add_subdirectory_ifdef(CONFIG_OPTEE_LIBTEEC libteec)
