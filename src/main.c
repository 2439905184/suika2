﻿/* -*- coding: utf-8-with-signature; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika 2
 * Copyright (C) 2001-2016, TABATA Keiichi. All rights reserved.
 */

/*
 * ゲームループのメインモジュール
 *
 * [Changes]
 *  - 2016/05/27 作成
 */

#include "suika.h"

/* false assertion */
#define COMMAND_DISPATCH_NOT_IMPLEMENTED	(0)

/*
 * 入力の状態
 *  - ControlキーとSpaceキーは、フレームをまたがって押下状態になる
 *  - その他のキー・マウスボタンはフレームごとに押下状態がクリアされる
 */
bool is_left_button_pressed;
bool is_right_button_pressed;
bool is_return_pressed;
bool is_space_pressed;
bool is_escape_pressed;
bool is_up_pressed;
bool is_down_pressed;
bool is_page_up_pressed;
bool is_page_down_pressed;
bool is_control_pressed;
int mouse_pos_x;
int mouse_pos_y;

/*
 * 前方参照
 */
static bool dispatch_command(int *x, int *y, int *w, int *h);

/*
 * ゲームループの中身
 */
bool game_loop_iter(int *x, int *y, int *w, int *h)
{
	if (!is_save_mode()) {
		/* コマンドを実行する */
		if (!dispatch_command(x, y, w, h))
			return false;
	} else {
		/* セーブ画面を実行する */
		run_save_mode(x, y, w, h);
	}

	/* サウンドのフェード処理を実行する */
	process_sound_fading();

	/*
	 * 入力の状態をリセットする
	 *  - Control, Space以外は1フレームごとにリセットする
	 */
	is_left_button_pressed = false;
	is_right_button_pressed = false;
	is_return_pressed = false;
	is_escape_pressed = false;

	return true;
}

/*
 * コマンドをディスパッチする
 */
static bool dispatch_command(int *x, int *y, int *w, int *h)
{
	/* ラベルをスキップする */
	while (get_command_type() == COMMAND_LABEL) {
		if (!move_to_next_command())
			return false;
	}
	
	/* コマンドをディスパッチする */
	switch (get_command_type()) {
	case COMMAND_MESSAGE:
	case COMMAND_SERIF:
		if (!message_command(x, y, w, h))
			return false;
		break;
	case COMMAND_BG:
		if (!bg_command(x, y, w, h))
			return false;
		break;
	case COMMAND_BGM:
		if (!bgm_command())
			return false;
		break;
	case COMMAND_CH:
		if (!ch_command(x, y, w, h))
			return false;
		break;
	case COMMAND_CLICK:
		if (!click_command())
			return false;
		break;
	case COMMAND_WAIT:
		if (!wait_command())
			return false;
		break;
	case COMMAND_GOTO:
		if (!goto_command())
			return false;
		break;
	case COMMAND_LOAD:
		if (!load_command())
			return false;
		break;
	case COMMAND_VOL:
		if (!vol_command())
			return false;
		break;
	case COMMAND_SET:
		if (!set_command())
			return false;
		break;
	case COMMAND_IF:
		if (!if_command())
			return false;
		break;
	case COMMAND_SELECT:
		if (!select_command(x, y, w, h))
			return false;
		break;
	case COMMAND_SE:
		if (!se_command())
			return false;
		break;
	case COMMAND_MENU:
		if (!menu_command(x, y, w, h))
			return false;
		break;
	default:
		/* コマンドに対応するcaseを追加し忘れている */
		assert(COMMAND_DISPATCH_NOT_IMPLEMENTED);
		break;
	}

	return true;
}
