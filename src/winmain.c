﻿/* -*- coding: utf-8-with-signature; indent-tabs-mode: t; tab-width: 4; c-basic-offset: 4; -*- */

/*
 * Suika 2
 * Copyright (C) 2001-2016, TABATA Keiichi. All rights reserved.
 */

/*
 * [Changes]
 *  2014-05-24 作成 (conskit)
 *  2016-05-29 作成 (suika)
 */

#include <windows.h>

#include "suika.h"
#include "dsound.h"

/* リソースIDのため */
#include "resource.h"

/* VC6対応 */
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL	0x020A
#endif

/* ログ1行のサイズ */
#define LOG_BUF_SIZE	(4096)

/* フレームレート */
#define FPS				(30)

/* 1フレームの時間 */
#define FRAME_MILLI		(33)

/* 1回にスリープする時間 */
#define SLEEP_MILLI		(5)

/* ウィンドウタイトル */
static const char szTitle[] = "suika";

/* ウィンドウクラス名 */
static const char szWindowClass[] = "suika";

/* Windowsオブジェクト */
static HWND hWndMain;
static HDC hWndDC;
static HDC hBitmapDC;
static HBITMAP hBitmap;

/* イメージオブジェクト */
static struct image *BackImage;

/* 計測用の時刻 */
DWORD dwStartTime;

/* ログファイル */
FILE *LogFp;

/* 前方参照 */
static BOOL InitApp(HINSTANCE hInstance, int nCmdShow);
static void CleanupApp(void);
static BOOL OpenLogFile(void);
static BOOL InitWindow(HINSTANCE hInstance, int nCmdShow);
static BOOL CreateBackImage(void);
static void GameLoop(void);
static void SyncBackImage(int x, int y, int w, int h);
static BOOL SyncEvents(void);
static BOOL WaitForNextFrame();
static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
								LPARAM lParam);
static int ConvertKeyCode(int nVK);
static void OnPaint(void);

/*
 * WinMain
 */
int WINAPI WinMain(
	HINSTANCE hInstance,
	UNUSED(HINSTANCE hPrevInstance),
	UNUSED(LPSTR lpszCmd),
	int nCmdShow)
{
	int result = 1;

	/* 基盤レイヤの初期化処理を行う */
	if(InitApp(hInstance, nCmdShow))
	{
		/* アプリケーション本体の初期化を行う */
		if(on_event_init())
		{
			/* イベントループを実行する */
			GameLoop();

			/* アプリケーション本体の終了処理を行う */
			on_event_cleanup();

			result = 0;
		}
	}

	/* 互換レイヤの終了処理を行う */
	CleanupApp();

	return result;
}

/* 基盤レイヤの初期化処理を行う */
static BOOL InitApp(HINSTANCE hInstance, int nCmdShow)
{
#ifdef SSE_VERSIONING
	/* ベクトル命令の対応を確認する */
	x86_check_cpuid_flags();
#endif

	/* COMの初期化を行う */
	CoInitialize(0);

	/* ログファイルをオープンする */
	if(!OpenLogFile())
		return FALSE;

	/* パッケージの初期化処理を行う */
	if(!init_file())
		return FALSE;

	/* コンフィグの初期化処理を行う */
	if(!init_conf())
		return FALSE;

	/* ウィンドウを作成する */
	if(!InitWindow(hInstance, nCmdShow))
		return FALSE;

	/* DirectSoundを初期化する */
	if(!DSInitialize(hWndMain))
		return FALSE;

	/* バックイメージを作成する */
	if(!CreateBackImage())
		return FALSE;

	return TRUE;
}

/* 基盤レイヤの終了処理を行う */
static void CleanupApp(void)
{
	/* ウィンドウのデバイスコンテキストを破棄する */
	if(hWndDC != NULL)
		ReleaseDC(hWndMain, hWndDC);

	/* バックイメージのビットマップを破棄する */
	if(hBitmap != NULL)
		DeleteObject(hBitmap);

	/* バックイメージのデバイスコンテキストを破棄する */
	if(hBitmapDC != NULL)
		DeleteDC(hBitmapDC);

	/* バックイメージを破棄する */
	if(BackImage != NULL)
		destroy_image(BackImage);

	/* DirectSoundの終了処理を行う */
	DSCleanup();

	/* コンフィグの終了処理を行う */
	cleanup_conf();

	/* ログファイルをクローズする */
	if(LogFp != NULL)
		fclose(LogFp);
}

/* ログをオープンする */
static BOOL OpenLogFile(void)
{
	if (LogFp == NULL)
	{
		LogFp = fopen(LOG_FILE, "w");
		if (LogFp == NULL)
		{
			MessageBox(NULL, "ログファイルをオープンできません。", "エラー",
					   MB_OK | MB_ICONWARNING);
			return FALSE;
		}
	}
	return TRUE;
}

/* ウィンドウを作成する */
static BOOL InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	WNDCLASSEX wcex;
	DWORD style;
	int dw, dh, i;

	/* ディスプレイのサイズが足りない場合 */
	if(GetSystemMetrics(SM_CXVIRTUALSCREEN) < conf_window_width ||
	   GetSystemMetrics(SM_CYVIRTUALSCREEN) < conf_window_height)
	{
		MessageBox(NULL, "ディスプレイのサイズが足りません。", "エラー",
				   MB_OK | MB_ICONERROR);
		return FALSE;
	}

	/* ウィンドウクラスを登録する */
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style          = 0;
	wcex.lpfnWndProc    = WndProc;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = hInstance;
	wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SUIKA));
	wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground  = (HBRUSH)GetStockObject(conf_window_white ?
												 WHITE_BRUSH : BLACK_BRUSH);
	wcex.lpszMenuName   = NULL;
	wcex.lpszClassName  = szWindowClass;
	wcex.hIconSm		= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
	if(!RegisterClassEx(&wcex))
		return FALSE;

	/* ウィンドウのスタイルを決める */
	style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_OVERLAPPED;

	/* フレームのサイズを取得する */
	dw = GetSystemMetrics(SM_CXFIXEDFRAME) * 2;
	dh = GetSystemMetrics(SM_CYCAPTION) +
		 GetSystemMetrics(SM_CYFIXEDFRAME) * 2;

	/* ウィンドウを作成する */
	hWndMain = CreateWindowEx(0, szWindowClass, szTitle, style,
							  (int)CW_USEDEFAULT, (int)CW_USEDEFAULT,
							  conf_window_width + dw, conf_window_height + dh,
							  NULL, NULL, hInstance, NULL);
	if(!hWndMain)
		return FALSE;

	/* ウィンドウを表示する */
	ShowWindow(hWndMain, nCmdShow);
	UpdateWindow(hWndMain);

	/* デバイスコンテキストを取得する */
	hWndDC = GetDC(hWndMain);
	if(hWndDC == NULL)
		return FALSE;

	/* 0.1秒間(3フレーム)でウィンドウに関連するイベントを処理してしまう */
	for(i = 0; i < FPS / 10; i++)
		WaitForNextFrame();

	return TRUE;
}

/* バックイメージを作成する */
static BOOL CreateBackImage(void)
{
	BITMAPINFO bi;
	pixel_t *pixels;

	/* ビットマップを作成する */
	memset(&bi, 0, sizeof(BITMAPINFO));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = conf_window_width;
	bi.bmiHeader.biHeight = -conf_window_height; /* Top-down */
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	hBitmapDC = CreateCompatibleDC(NULL);
	if(hBitmapDC == NULL)
		return FALSE;

	/* DIBを作成する */
	hBitmap = CreateDIBSection(hBitmapDC, &bi, DIB_RGB_COLORS,
							   (VOID **)&pixels, NULL, 0);
	if(hBitmap == NULL || pixels == NULL)
		return FALSE;
	SelectObject(hBitmapDC, hBitmap);

	/* イメージを作成する */
	BackImage = create_image_with_pixels(conf_window_width, conf_window_height,
										 pixels);
	if(BackImage == NULL)
		return FALSE;
	if(conf_window_white)
		clear_image_white(BackImage);

	return TRUE;
}

/* ゲームループを実行する */
static void GameLoop(void)
{
	int x, y, w, h;

	/* 最初にイベントを処理してしまう */
	if(!SyncEvents())
		return;

	/* 最初のフレームの開始時刻を取得する */
	dwStartTime = GetTickCount();
	while(TRUE)
	{
		/* 描画を行う */
		if(!on_event_frame(&x, &y, &w, &h))
			break;	/* スクリプトの終端に達した */

		/* 描画を反映する */
		if(w !=0 && h !=0)
			SyncBackImage(x, y, w, h);

		/* 次の描画までスリープする */
		if(!WaitForNextFrame())
			break;	/* 閉じるボタンが押された */

		/* 次のフレームの開始時刻を取得する */
		dwStartTime = GetTickCount();
	}
}

/* ウィンドウにバックイメージを転送する */
static void SyncBackImage(int x, int y, int w, int h)
{
	BitBlt(hWndDC, x, y, w, h, hBitmapDC, x, y, SRCCOPY);
}

/* キューにあるイベントを処理する */
static BOOL SyncEvents(void)
{
	MSG msg;

	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if(msg.message == WM_QUIT)
			return FALSE;
		if(!TranslateAccelerator(msg.hwnd, NULL, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return TRUE;
}

/* 次のフレームの開始時刻までイベント処理とスリープを行う */
static BOOL WaitForNextFrame(void)
{
	DWORD end, lap, wait;

	/* 次のフレームの開始時刻になるまでイベント処理とスリープを行う */
	do {
		/* イベントがある場合は処理する */
		if(!SyncEvents())
			return FALSE;

		/* 経過時刻を取得する */
		end = GetTickCount();
		lap = dwStartTime - end;

		/* 次のフレームの開始時刻になった場合はスリープを終了する */
		if (lap > FRAME_MILLI) {
			dwStartTime = end;
			break;
		}

		/* スリープする時間を求める */
		wait = (FRAME_MILLI - lap > SLEEP_MILLI) ? SLEEP_MILLI :
		    FRAME_MILLI - lap;

		/* スリープする */
		Sleep(wait);
	} while(wait > 0);

	return TRUE;
}

/* ウィンドウプロシージャ */
static LRESULT CALLBACK WndProc(HWND hWnd,
								UINT message,
								WPARAM wParam,
								LPARAM lParam)
{
	int kc;
	switch(message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_LBUTTONDOWN:
		on_event_mouse_press(MOUSE_LEFT, LOWORD(lParam), HIWORD(lParam));
		return 0;
	case WM_LBUTTONUP:
		on_event_mouse_release(MOUSE_LEFT, LOWORD(lParam), HIWORD(lParam));
		return 0;
	case WM_RBUTTONDOWN:
		on_event_mouse_press(MOUSE_RIGHT, LOWORD(lParam), HIWORD(lParam));
		return 0;
	case WM_RBUTTONUP:
		on_event_mouse_release(MOUSE_RIGHT, LOWORD(lParam), HIWORD(lParam));
		return 0;
	case WM_KEYDOWN:
		/* オートリピートの場合を除外する */
		if((HIWORD(lParam) & 0x4000) != 0)
			return 0;
		kc = ConvertKeyCode((int)wParam);
		if(kc != -1)
			on_event_key_press(kc);
		return 0;
	case WM_KEYUP:
		kc = ConvertKeyCode((int)wParam);
		if(kc != -1)
			on_event_key_release(kc);
		return 0;
	case WM_MOUSEMOVE:
		on_event_mouse_move(LOWORD(lParam), HIWORD(lParam));
		return 0;
	case WM_MOUSEWHEEL:
		if((int)(short)HIWORD(wParam) > 0) {
			on_event_key_press(KEY_UP);
			on_event_key_release(KEY_UP);
		} else if((int)(short)HIWORD(wParam) < 0) {
			on_event_key_press(KEY_DOWN);
			on_event_key_release(KEY_DOWN);
		}
		return 0;
	case WM_PAINT:
		OnPaint();
		return 0;
	default:
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/* キーコードの変換を行う */
static int ConvertKeyCode(int nVK)
{
	switch(nVK)
	{
	case VK_CONTROL:
		return KEY_CONTROL;
	case VK_SPACE:
		return KEY_SPACE;
	case VK_RETURN:
		return KEY_RETURN;
	case VK_ESCAPE:
		return KEY_ESCAPE;
	case VK_UP:
		return KEY_UP;
	case VK_DOWN:
		return KEY_DOWN;
	case VK_PRIOR:
		return KEY_PAGE_UP;
	case VK_NEXT:
		return KEY_PAGE_DOWN;
	default:
		break;
	}
	return -1;
}

/* ウィンドウの内容を更新する */
static void OnPaint(void)
{
	PAINTSTRUCT ps;
	HDC hDC;

	hDC = BeginPaint(hWndMain, &ps);
	BitBlt(hDC,
		   ps.rcPaint.left,
		   ps.rcPaint.top,
		   ps.rcPaint.right - ps.rcPaint.left,
		   ps.rcPaint.bottom - ps.rcPaint.top,
		   hBitmapDC,
		   ps.rcPaint.left,
		   ps.rcPaint.top,
		   SRCCOPY);
	EndPaint(hWndMain, &ps);
}

/*
 * platform.hの実装
 */

/*
 * INFOログを出力する
 */
bool log_info(const char *s, ...)
{
	char buf[LOG_BUF_SIZE];
	va_list ap;

	va_start(ap, s);
	if(LogFp != NULL)
	{
		/* メッセージボックスを表示する */
		vsnprintf(buf, sizeof(buf), s, ap);
		MessageBox(hWndMain, buf, "情報", MB_OK | MB_ICONINFORMATION);

		/* ファイルへ出力する */
		fprintf(LogFp, buf);
		fflush(LogFp);
		if(ferror(LogFp))
			return false;
	}
	va_end(ap);
	return true;
}

/*
 * WARNログを出力する
 */
bool log_warn(const char *s, ...)
{
	char buf[LOG_BUF_SIZE];
	va_list ap;

	va_start(ap, s);
	if(LogFp != NULL)
	{
		/* メッセージボックスを表示する */
		vsnprintf(buf, sizeof(buf), s, ap);
		MessageBox(hWndMain, buf, "警告", MB_OK | MB_ICONWARNING);

		/* ファイルへ出力する */
		fprintf(LogFp, buf);
		fflush(LogFp);
		if(ferror(LogFp))
			return false;
	}
	va_end(ap);
	return true;
}

/*
 * ERRORログを出力する
 */
bool log_error(const char *s, ...)
{
	char buf[LOG_BUF_SIZE];
	va_list ap;

	va_start(ap, s);
	if(LogFp != NULL)
	{
		/* メッセージボックスを表示する */
		vsnprintf(buf, sizeof(buf), s, ap);
		MessageBox(hWndMain, buf, "エラー", MB_OK | MB_ICONERROR);

		/* ファイルへ出力する */
		fprintf(LogFp, buf);
		fflush(LogFp);
		if(ferror(LogFp))
			return false;
	}
	va_end(ap);
	return true;
}

/*
 * データのディレクトリ名とファイル名を指定して有効なパスを取得する
 */
char *make_valid_path(const char *dir, const char *fname)
{
	char *buf;
	size_t len;

	if (dir == NULL)
		dir = "";

	/* パスのメモリを確保する */
	len = strlen(dir) + 1 + strlen(fname) + 1;
	buf = malloc(len);
	if (buf == NULL)
		return NULL;

	strcpy(buf, dir);
	if (strlen(dir) != 0)
		strcat(buf, "\\");
	strcat(buf, fname);

	return buf;
}

/*
 * バックイメージを取得する
 */
struct image *get_back_image(void)
{
	return BackImage;
}

/*
 * タイマをリセットする
 */
void reset_stop_watch(stop_watch_t *t)
{
	*t = GetTickCount();
}

/*
 * タイマのラップをミリ秒単位で取得する
 */
int get_stop_watch_lap(stop_watch_t *t)
{
	DWORD dwCur = GetTickCount();
	return (int32_t)(dwCur - *t);
}
