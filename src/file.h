﻿/* -*- coding: utf-8-with-signature; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika 2
 * Copyright (C) 2001-2016, TABATA Keiichi. All rights reserved.
 */

/*
 * [Changes]
 *  - 2016/06/28 作成
 */

#ifndef SUIKA_FILE_H
#define SUIKA_FILE_H

#include "types.h"

/*
 * ファイル読み込みストリーム
 */
struct rfile;

/*
 * ファイル書き込みストリーム
 */
struct wfile;

/*
 * ファイル読み込みストリームを開く
 */
struct rfile *open_rfile(const char *dir, const char *file, bool no_error);

/*
 * ファイル読み込みストリームから読み込む
 */
size_t read_rfile(struct rfile *rf, void *buf, size_t size);

/*
 * ファイル読み込みストリームから1行読み込む
 */
void *gets_rfile(struct rfile *rf, void *buf, size_t size);

/*
 * ファイル読み込みストリームを閉じる
 */
void close_rfile(struct rfile *rf);

/*
 * ファイル書き込みストリームを開く
 */
struct wfile *open_wfile(const char *dir, const char *file);

/*
 * ファイル書き込みストリームに書き込む
 */
size_t write_wfile(struct wfile *wf, const void *buf, size_t size);

/*
 * ファイル読み込みストリームを閉じる
 */
void close_wfile(struct wfile *wf);

#endif
