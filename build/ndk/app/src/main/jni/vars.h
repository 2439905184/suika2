﻿/* -*- coding: utf-8-with-signature; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika 2
 * Copyright (C) 2001-2016, TABATA Keiichi. All rights reserved.
 */

/*
 * [Changes]
 *  - 2016/06/29 作成
 */

#ifndef SUIKA_VARS_H
#define SUIKA_VARS_H

#include "types.h"

/*
 * 変数の数
 */
#define VAR_SIZE	(1024)

/* 変数の初期化処理を行う */
void init_vars(void);

/* 変数の終了処理を行う */
void cleanup_vars(void);

/* 変数を取得する */
int32_t get_variable(int index);

/* 変数を設定する */
void set_variable(int index, int32_t val);

/* 変数を文字列で指定して取得する */
bool get_variable_by_string(const char *var, int32_t *val);

/* 変数を文字列で指定して設定する */
bool set_variable_by_string(const char *var, int32_t val);

#endif
