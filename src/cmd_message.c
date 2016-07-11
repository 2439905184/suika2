﻿/* -*- coding: utf-8-with-signature; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika 2
 * Copyright (C) 2001-2016, TABATA Keiichi. All rights reserved.
 */

/*
 * [Changes]
 *  - 2016/06/24 作成
 */

#include "suika.h"

/* 句読点のUnicodeコードポイント */
#define CHAR_TOUTEN	(0x3001)
#define CHAR_KUTEN	(0x3002)

/* 繰り返し動作中であるか */
static bool repeatedly;

/* コマンドの経過時刻を表すストップウォッチ */
static stop_watch_t sw;

/* 描画する文字の総数 */
static int total_chars;

/* 前回までに描画した文字数 */
static int drawn_chars;

/* 描画位置 */
static int pen_x;
static int pen_y;

/* メッセージボックスの幅 */
static int msgbox_x;
static int msgbox_y;
static int msgbox_w;
static int msgbox_h;

/* 描画する矩形 */
static int draw_x;
static int draw_y;
static int draw_w;
static int draw_h;

/* 描画するメッセージ */
static const char *msg;

/* クリックアニメーションの初回描画が完了したか */
static bool is_click_first;

/* クリックアニメーションの表示状態 */
static bool is_click_visible;

/* スペースキーによる非表示が実行中であるか */
static bool is_hidden;

/* ヒストリ画面から戻ったばかりであるか */
static bool history_flag;

/*
 * 前方参照
 */
static bool init(void);
static bool register_message_for_history(void);
static bool process_serif_command(void);
static void draw_namebox(void);
static bool play_voice(void);
static void draw_msgbox(void);
static int get_frame_chars(void);
static void draw_click(void);
static bool cleanup(void);

/*
 * メッセージ・セリフコマンド
 */
bool message_command(int *x, int *y, int *w, int *h)
{
	draw_x = 0;
	draw_y = 0;
	draw_w = 0;
	draw_h = 0;

	/* 初期化処理を行う */
	if (!repeatedly)
		if (!init())
			return false;

	/* セーブ画面への遷移を処理する */
	if (is_right_button_pressed) {
		start_save_mode(false);
		repeatedly = false;
		show_click(false);
		return true;
	}

	/* ヒストリ画面への遷移を処理する */
	if (is_up_pressed) {
		start_history_mode();
		repeatedly = false;
		return true;
	}

	/* 繰り返し動作を行う */
	if (!is_hidden && is_space_pressed) {
		is_hidden = true;
		if (get_command_type() == COMMAND_SERIF)
			show_namebox(false);
		show_msgbox(false);
		show_click(false);
		draw_x = 0;
		draw_y = 0;
		draw_w = conf_window_width;
		draw_h = conf_window_height;
	} else if(is_hidden && !is_space_pressed) {
		is_hidden = false;
		if (get_command_type() == COMMAND_SERIF)
			show_namebox(true);
		show_msgbox(true);
		show_click(true);
		draw_x = 0;
		draw_y = 0;
		draw_w = conf_window_width;
		draw_h = conf_window_height;
	} else if (!is_hidden) {
		if (is_control_pressed)
			stop_sound(VOICE_STREAM);
		if (drawn_chars < total_chars)
			draw_msgbox();
		else
			draw_click();
	}

	/* 終了処理を行う */
	if (!repeatedly)
		if (!cleanup())
			return false;

	/* ステージを描画する */
	draw_stage_rect(draw_x, draw_y, draw_w, draw_h);

	/* 描画した範囲をウィンドウの更新範囲とする */
	*x = draw_x;
	*y = draw_y;
	*w = draw_w;
	*h = draw_h;

	return true;
}

/* 初期化処理を行う */
static bool init(void)
{
	/* ヒストリ画面用にメッセージ履歴を登録する */
	if (!register_message_for_history())
		return false;

	/* セリフの場合を処理する */
	if (!process_serif_command())
		return false;

	/* メッセージを取得する */
	msg = get_command_type() == COMMAND_MESSAGE ?
		get_line_string() : get_string_param(SERIF_PARAM_MESSAGE);

	/*メッセージの文字数を求める */
	total_chars = utf8_chars(msg);
	drawn_chars = 0;

	/* メッセージの描画位置を初期化する */
	pen_x = conf_msgbox_margin_left;
	pen_y = conf_msgbox_margin_top;

	/* メッセージボックスの矩形を取得する */
	get_msgbox_rect(&msgbox_x, &msgbox_y, &msgbox_w, &msgbox_h);

	/* メッセージボックスをクリアする */
	clear_msgbox();

	/* メッセージボックスを表示する */
	show_msgbox(true);

	/* クリックアニメーションを非表示の状態にする */
	show_click(false);
	is_click_first = true;
	is_click_visible = false;

	/* スペースキーによる非表示でない状態にする */
	is_hidden = false;
	
	/* 繰り返し動作を設定する */
	if (is_control_pressed) {
		/* 繰り返し動作せず、すぐに表示する */
	} else {
		/* コマンドが繰り返し呼び出されるようにする */
		repeatedly = true;

		/* 時間計測を開始する */
		reset_stop_watch(&sw);
	}

	/* 初回に描画する矩形を求める */
	if (check_menu_finish_flag()) {
		/* メニューコマンドが終了したばかりの場合 */
		draw_x = 0;
		draw_y = 0;
		draw_w = conf_window_width;
		draw_h = conf_window_height;
	} else {
		/* それ以外の場合 */
		union_rect(&draw_x, &draw_y, &draw_w, &draw_h, draw_x, draw_y,
			   draw_w, draw_h, msgbox_x, msgbox_y, msgbox_w,
			   msgbox_h);
	}
	return true;
}

/* ヒストリ画面用にメッセージ履歴を登録する */
static bool register_message_for_history(void)
{
	const char *name;
	const char *voice;
	const char *msg;

	/* ヒストリ画面から戻ったばかりの場合、2重登録を防ぐ */
	history_flag = check_history_flag();
	if (history_flag)
		return true;

	/* 名前、ボイスファイル名、メッセージを取得する */
	if (get_command_type() == COMMAND_SERIF) {
		name = get_string_param(SERIF_PARAM_NAME);
		voice = get_string_param(SERIF_PARAM_VOICE);
		msg = get_string_param(SERIF_PARAM_MESSAGE);
	} else {
		name = NULL;
		voice = NULL;
		msg = get_line_string();
	}

	/* ヒストリ画面用に登録する */
	if (!register_message(name, msg, voice))
		return false;

	return true;
}
		
/* セリフコマンドを処理する */
static bool process_serif_command(void)
{
	int namebox_x, namebox_y, namebox_w, namebox_h;

	/*
	 * 描画範囲を更新する
	 *  - セリフコマンド以外でも、名前ボックス領域を消すために描画する
	 */
	get_namebox_rect(&namebox_x, &namebox_y, &namebox_w, &namebox_h);
	union_rect(&draw_x, &draw_y, &draw_w, &draw_h, draw_x, draw_y, draw_w,
		   draw_h, namebox_x, namebox_y, namebox_w, namebox_h);

	/* セリフコマンドではない場合 */
	if (get_command_type() != COMMAND_SERIF) {
		/* 名前ボックスを表示しない */
		show_namebox(false);
		return true;
	}

	/* ボイスを再生する */
	if (!is_control_pressed && !history_flag)
		if (!play_voice())
			return false;

	/* 名前を描画する */
	draw_namebox();

	/* 名前ボックスを表示する */
	show_namebox(true);

	return true;
}


/* 名前ボックスを描画する */
static void draw_namebox(void)
{
	uint32_t c;
	int char_count, mblen, i, x, w;
	const char *name;

	/* 名前の文字列を取得する */
	name = get_string_param(SERIF_PARAM_NAME);

	/* 名前の文字数を取得する */
	char_count = utf8_chars(name);
	if (char_count == 0)
		return;

	/* 描画位置を決める */
	x = conf_namebox_margin_left;

	/* 名前ボックスをクリアする */
	clear_namebox();

	/* 1文字ずつ描画する */
	for (i = 0; i < char_count; i++) {
		/* 描画する文字を取得する */
		mblen = utf8_to_utf32(name, &c);
		if (mblen == -1)
			return;

		/* 描画する */
		w = draw_char_on_namebox(x, conf_namebox_margin_top, c);

		/* 次の文字へ移動する */
		x += w;
		name += mblen;
	}
}

/* ボイスを再生する */
static bool play_voice(void)
{
	struct wave *w;
	const char *voice;

	/* ボイスのファイル名を取得する */
	voice = get_string_param(SERIF_PARAM_VOICE);
	if (strcmp(voice, "") == 0)
		return true;

	/* PCMストリームを開く */
	w = create_wave_from_file(CV_DIR, voice, false);
	if (w == NULL) {
		log_script_exec_footer();
		return false;
	}

	/* PCMストリームを再生する */
	set_mixer_input(VOICE_STREAM, w);

	return true;
}

/* メッセージボックスの描画を行う */
static void draw_msgbox(void)
{
	uint32_t c;
	int char_count, mblen, w, h, i;

	/* 今回のフレームで描画する文字数を取得する */
	char_count = get_frame_chars();
	if (char_count == 0)
		return;

	/* 1文字ずつ描画する */
	for (i = 0; i < char_count; i++) {
		/* 描画する文字を取得する */
		mblen = utf8_to_utf32(msg, &c);
		if (mblen == -1) {
			drawn_chars = total_chars;
			return;
		}

		/* 描画する文字の幅を取得する */
		w = get_glyph_width(c);

		/* メッセージボックスの幅を超える場合、改行する */
		if ((pen_x + w >= msgbox_w - conf_msgbox_margin_right) &&
		    (c != CHAR_TOUTEN && c != CHAR_KUTEN)) {
			pen_y += conf_msgbox_margin_line;
			pen_x = conf_msgbox_margin_left;
		}

		/* 描画する */
		h = draw_char_on_msgbox(pen_x, pen_y, c);

		/* 更新領域を求める */
		union_rect(&draw_x, &draw_y, &draw_w, &draw_h, draw_x, draw_y,
			   draw_w, draw_h, msgbox_x + pen_x, msgbox_y + pen_y,
			   w, h);

		/* 次の文字へ移動する */
		pen_x += w;
		msg += mblen;
		drawn_chars++;
	}
}

/* 今回のフレームで描画する文字数を取得する */
static int get_frame_chars(void)
{
	float lap;
	int char_count;

	/* 繰り返し動作しない場合 */
	if (!repeatedly) {
		/* すべての文字を描画する */
		return total_chars;
	}

	/* セーブ画面かヒストリ画面から復帰した場合 */
	if (check_restore_flag() || history_flag) {
		/* すべての文字を描画する */
		return total_chars;
	}

	/* 入力によりスキップされた場合 */
	if (is_control_pressed) {
		/* 繰り返し動作を停止する */
		repeatedly = false;

		/* 残りの文字をすべて描画する */
		return total_chars - drawn_chars;
	}
	if (is_return_pressed || is_left_button_pressed) {
		/* 残りの文字をすべて描画する */
		return total_chars - drawn_chars;
	}

	/* 経過時間を取得する */
	lap = (float)get_stop_watch_lap(&sw) / 1000.0f;

	/* 今回描画する文字数を取得する */
	char_count = (int)ceil(conf_msgbox_speed * lap) - drawn_chars;
	if (char_count > total_chars - drawn_chars)
		char_count = total_chars - drawn_chars;

	return char_count;
}

/* クリックアニメーションを描画する */
static void draw_click(void)
{
	int click_x, click_y, click_w, click_h;
	int lap;

	/* 入力があったら終了する */
	if (is_control_pressed) {
		repeatedly = false;
		return;
	}
	if (!is_click_first && (is_return_pressed || is_left_button_pressed)) {
		repeatedly = false;
		return;
	}

	/* クリックアニメーションの初回表示のとき */
	if (is_click_first) {
		is_click_first = false;
		is_click_visible = false;

		/* 時間計測を開始する */
		reset_stop_watch(&sw);
	}

	/* 経過時間を取得する */
	lap = get_stop_watch_lap(&sw);

	/* クリックアニメーションの点滅を行う */
	if (lap % (int)(conf_click_interval * 2 * 1000) <
	    (int)(conf_click_interval * 1000)) {
		if (!is_click_visible) {
			show_click(true);
			is_click_visible = true;
		} else {
			return;
		}
	} else {
		if (is_click_visible) {
			show_click(false);
			is_click_visible = false;
		} else {
			return;
		}
	}

	/* 描画範囲を求める */
	get_click_rect(&click_x, &click_y, &click_w, &click_h);
    union_rect(&draw_x, &draw_y, &draw_w, &draw_h, draw_x, draw_y, draw_w, draw_h,
               click_x, click_y, click_w, click_h);
}

/* 終了処理を行う */
static bool cleanup(void)
{
	/* コマンドの繰り返し動作を無効にする */
	repeatedly = false;

	/* クリックアニメーションを非表示にする */
	show_click(false);

	/* 次のコマンドに移動する */
	if (!move_to_next_command())
		return false;

	return true;
}
