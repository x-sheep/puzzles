#include "pch.h"
#include "WindowsModern.h"
#include "PuzzleData.h"

extern "C" {
#include <windows.h>
#include <stdio.h>

#include "../../puzzles.h"
#include "../../gluefe.h"
}
#include "IPuzzleCanvas.h"
#include "PuzzleHelpData.h"
#include <ppltasks.h>

using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::System;
using namespace Windows::Storage;

/// <summary>
/// Convert an array of chars to a Platform String, using UTF8 encoding.
/// </summary>
/// <param name="input">A pointer to a char string.</param>
Platform::String ^FromChars(const char *input)
{
	if (!input)
		return nullptr;

	auto newlen = MultiByteToWideChar(CP_UTF8, 0, input, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[newlen];
	MultiByteToWideChar(CP_UTF8, 0, input, -1, wstr, newlen);

	auto ret = ref new Platform::String(wstr);
	delete[] wstr;

	return ret;
}

/// <summary>
/// Convert an array of chars to a Platform String, using UTF8 encoding.
/// </summary>
/// <param name="input">A pointer to a char string.</param>
/// <param name="kill">If true, will free the input array.</param>
Platform::String ^FromChars(char *input, bool kill)
{
	auto ret = FromChars(input);

	if (kill)
		sfree(input);

	return ret;
}

void winmodern_status_bar(void *handle, char *text)
{
	frontend *fe = (frontend *)handle;
	PuzzleModern::IPuzzleStatusBar ^bar = *((PuzzleModern::IPuzzleStatusBar^*)fe->statusbar);

	bar->UpdateStatusBar(FromChars(text));
}

void winmodern_start_draw(void *handle)
{
	frontend *fe = (frontend *)handle;
	PuzzleModern::IPuzzleCanvas ^canvas = *((PuzzleModern::IPuzzleCanvas^*)fe->canvas);

	canvas->StartDraw();
}

void winmodern_draw_update(void *handle, int x, int y, int w, int h)
{
	frontend *fe = (frontend *)handle;
	PuzzleModern::IPuzzleCanvas ^canvas = *((PuzzleModern::IPuzzleCanvas^*)fe->canvas);
	
	canvas->UpdateArea(x, y, w, h);
}

void winmodern_clip(void *handle, int x, int y, int w, int h)
{
	frontend *fe = (frontend *)handle;
	PuzzleModern::IPuzzleCanvas ^canvas = *((PuzzleModern::IPuzzleCanvas^*)fe->canvas);
	
	canvas->StartClip(x, y, w, h);
}

void winmodern_unclip(void *handle)
{
	frontend *fe = (frontend *)handle;
	PuzzleModern::IPuzzleCanvas ^canvas = *((PuzzleModern::IPuzzleCanvas^*)fe->canvas);
	
	canvas->EndClip();
}

void winmodern_draw_text(void *handle, int x, int y, int fonttype, int fontsize,
	int align, int colour, char *text)
{
	frontend *fe = (frontend *)handle;
	PuzzleModern::IPuzzleCanvas ^canvas = *((PuzzleModern::IPuzzleCanvas^*)fe->canvas);

	PuzzleModern::GameFontType gamefont = fonttype == FONT_VARIABLE ?
		PuzzleModern::GameFontType::VariableWidth : 
		PuzzleModern::GameFontType::FixedWidth;

	PuzzleModern::GameFontHAlign halign = align & ALIGN_HCENTRE ?
		PuzzleModern::GameFontHAlign::HorizontalCenter :
		align & ALIGN_HRIGHT ?
		PuzzleModern::GameFontHAlign::HorizontalRight :
		PuzzleModern::GameFontHAlign::HorizontalLeft;

	PuzzleModern::GameFontVAlign valign = align & ALIGN_VCENTRE ?
		PuzzleModern::GameFontVAlign::VerticalCentre :
		PuzzleModern::GameFontVAlign::VerticalBase;

	canvas->DrawText(x, y, gamefont, halign, valign, fontsize, colour, FromChars(text));
}

void winmodern_draw_rect(void *handle, int x, int y, int w, int h, int colour)
{
	frontend *fe = (frontend *)handle;
	PuzzleModern::IPuzzleCanvas ^canvas = *((PuzzleModern::IPuzzleCanvas^*)fe->canvas);

	canvas->DrawRect(x, y, w, h, colour);
}

void winmodern_draw_line(void *handle, int x1, int y1, int x2, int y2,
	int colour)
{
	frontend *fe = (frontend *)handle;
	PuzzleModern::IPuzzleCanvas ^canvas = *((PuzzleModern::IPuzzleCanvas^*)fe->canvas);

	canvas->DrawLine(x1, y1, x2, y2, colour);
}

void winmodern_draw_poly(void *handle, int *coords, int npoints,
	int fillcolour, int outlinecolour)
{
	frontend *fe = (frontend *)handle;
	PuzzleModern::IPuzzleCanvas ^canvas = *((PuzzleModern::IPuzzleCanvas^*)fe->canvas);

	auto pc = ref new Vector<Point>();
	int i, j;
	for (i = 0; i <= npoints; i++)
	{
		j = (i < npoints ? i : 0);
		pc->Append(Point((float)coords[j * 2], (float)coords[j * 2 + 1]));
	}

	canvas->DrawPolygon(pc, fillcolour, outlinecolour);
}

void winmodern_draw_circle(void *handle, int cx, int cy, int radius,
	int fillcolour, int outlinecolour)
{
	frontend *fe = (frontend *)handle;
	PuzzleModern::IPuzzleCanvas ^canvas = *((PuzzleModern::IPuzzleCanvas^*)fe->canvas);

	canvas->DrawCircle(cx, cy, radius, fillcolour, outlinecolour);
}

blitter *winmodern_blitter_new(void *handle, int w, int h)
{
	frontend *fe = (frontend *)handle;
	PuzzleModern::IPuzzleCanvas ^canvas = *((PuzzleModern::IPuzzleCanvas^*)fe->canvas);

	blitter *bl = snew(blitter);
	bl->handle = canvas->BlitterNew(w, h);
	bl->w = w;
	bl->h = h;
	return bl;
}

void winmodern_blitter_free(void *handle, blitter *bl)
{
	frontend *fe = (frontend *)handle;
	PuzzleModern::IPuzzleCanvas ^canvas = *((PuzzleModern::IPuzzleCanvas^*)fe->canvas);
	
	canvas->BlitterFree(bl->handle);
	sfree(bl);
}

void winmodern_blitter_save(void *handle, blitter *bl, int x, int y)
{
	frontend *fe = (frontend *)handle;
	PuzzleModern::IPuzzleCanvas ^canvas = *((PuzzleModern::IPuzzleCanvas^*)fe->canvas);

	/* Resize the blitter rectangle to never use a negative x or y */
	if (x < 0)
	{
		bl->x = 0;
		bl->rw = bl->w + x;
	}
	else
	{
		bl->x = x;
		bl->rw = bl->w;
	}
	if (y < 0)
	{
		bl->y = 0;
		bl->rh = bl->h + y;
	}
	else
	{
		bl->y = y;
		bl->rh = bl->h;
	}

	canvas->BlitterSave(bl->handle, bl->x, bl->y, bl->rw, bl->rh);
}

void winmodern_blitter_load(void *handle, blitter *bl, int x, int y)
{
	frontend *fe = (frontend *)handle;
	PuzzleModern::IPuzzleCanvas ^canvas = *((PuzzleModern::IPuzzleCanvas^*)fe->canvas);
	
	if (x == BLITTER_FROMSAVED)
		x = bl->x;
	if (y == BLITTER_FROMSAVED)
		y = bl->y;
	
	canvas->BlitterLoad(bl->handle, max(x, 0), max(y, 0), bl->rw, bl->rh);
}

void winmodern_end_draw(void *handle)
{
	frontend *fe = (frontend *)handle;
	PuzzleModern::IPuzzleCanvas ^canvas = *((PuzzleModern::IPuzzleCanvas^*)fe->canvas);

	canvas->EndDraw();
}

void winmodern_changed_state(void *handle, int can_undo, int can_redo)
{
}

static char *winmodern_text_fallback(void *handle, const char *const *strings,
	int nstrings)
{
	/* Any UTF-8 string is accepted. */
	return dupstr(strings[0]);
}

void winmodern_line_width(void *handle, float width)
{
	frontend *fe = (frontend *)handle;
	PuzzleModern::IPuzzleCanvas ^canvas = *((PuzzleModern::IPuzzleCanvas^*)fe->canvas);

	canvas->SetLineWidth(width);
}

void winmodern_line_dotted(void *handle, int dotted)
{
	frontend *fe = (frontend *)handle;
	PuzzleModern::IPuzzleCanvas ^canvas = *((PuzzleModern::IPuzzleCanvas^*)fe->canvas);

	canvas->SetLineDotted(dotted != 0);
}

void winmodern_add_print_colour(void *handle, int id)
{
	frontend *fe = (frontend *)handle;
	PuzzleModern::IPuzzleCanvas ^canvas = *((PuzzleModern::IPuzzleCanvas^*)fe->canvas);

	int hatch = 0;
	float r = 0, g = 0, b = 0;
	print_get_colour(fe->printdr, id, FALSE, &hatch, &r, &g, &b);

	// Hatch brushes are currently not supported by Windows RT.
	// All puzzles which use the hatch brush have been edited.
	canvas->AddColor(r, g, b);
}
void winmodern_begin_doc(void *handle, int pages)
{
}
void winmodern_begin_page(void *handle, int number)
{
}
void winmodern_begin_puzzle(void *handle, float xm, float xc,
	float ym, float yc, int pw, int ph, float wmm)
{
	frontend *fe = (frontend *)handle;
	fe->printw = pw;
	fe->printh = ph;
}
void winmodern_end_puzzle(void *handle)
{
}
void winmodern_end_page(void *handle, int page)
{
}
void winmodern_end_doc(void *handle)
{
}

const struct drawing_api winmodern_drawing = {
	winmodern_draw_text,
	winmodern_draw_rect,
	winmodern_draw_line,
	winmodern_draw_poly,
	winmodern_draw_circle,
	winmodern_draw_update,
	winmodern_clip,
	winmodern_unclip,
	winmodern_start_draw,
	winmodern_end_draw,
	winmodern_status_bar,
	winmodern_blitter_new,
	winmodern_blitter_free,
	winmodern_blitter_save,
	winmodern_blitter_load,
	winmodern_add_print_colour,
	winmodern_begin_doc,
	winmodern_begin_page,
	winmodern_begin_puzzle,
	winmodern_end_puzzle,
	winmodern_end_page,
	winmodern_end_doc,
	winmodern_line_width, winmodern_line_dotted,
	winmodern_text_fallback,
};

void winmodern_get_random_seed(void **randseed, int *randseedsize)
{
	unsigned int *p = snew(unsigned int);

	*p = Windows::Security::Cryptography::CryptographicBuffer::GenerateRandomNumber();

	*randseed = (void *)p;
	*randseedsize = sizeof(unsigned int);
}

void winmodern_deactivate_timer(frontend *fe)
{
	if (!fe || !fe->timer) return;

	PuzzleModern::IPuzzleTimer ^timer = *((PuzzleModern::IPuzzleTimer^*)fe->timer);
	timer->EndTimer();
}

void winmodern_activate_timer(frontend *fe)
{
	if (!fe || !fe->timer) return;

	PuzzleModern::IPuzzleTimer ^timer = *((PuzzleModern::IPuzzleTimer^*)fe->timer);
	timer->StartTimer();
}

void winmodern_fatal(const char *msg)
{
	throw ref new Platform::FailureException(FromChars(msg));
}

static Windows::Foundation::IAsyncAction^ currentWorkItem = nullptr;
static bool cancellable = false;

void winmodern_check_abort(void)
{
	if (cancellable && currentWorkItem && currentWorkItem->Status == Windows::Foundation::AsyncStatus::Canceled)
	{
		cancellable = false;
		throw std::exception();
	}
}

char *winmodern_getenv(const char *key)
{
	auto settings = Windows::Storage::ApplicationData::Current->RoamingSettings->Values;

	if (!strcmp(key, "MAP_VIVID_COLOURS"))
	{
		if(settings->HasKey("env_MAP_VIVID_COLOURS") && safe_cast<bool>(settings->Lookup("env_MAP_VIVID_COLOURS")))
			return "Y";
	}

	return NULL;
}

#ifdef DEBUGGING
void winmodern_debug(const char *msg)
{
	OutputDebugString(FromChars(msg)->Data());
}
#endif

struct frontend_adapter winmodern_adapter = {
	winmodern_get_random_seed,
	winmodern_getenv,
	NULL, /* frontend_default_colour */
	winmodern_fatal,
#ifdef DEBUGGING
	winmodern_debug,
#endif
	winmodern_deactivate_timer,
	winmodern_activate_timer,
	winmodern_check_abort,
};

// Reading function for deserialization
int winmodern_read_chars(void *wctx, void *buf, int len)
{
	int i;
	char *p;
	char **ctx;

	ctx = (char**)wctx;
	p = *ctx;

	// Verify copy
	for (i = 0; i < len; i++)
	{
		if (!p[i])
			return FALSE;
	}

	memcpy(buf, p, len);
	*ctx += len;

	return TRUE;
}

// Writing function for serialization
void winmodern_write_chars(void *vctx, void *buf, int len)
{
	PuzzleModern::write_save_context *ctx;
	ctx = (PuzzleModern::write_save_context *)vctx;
	
	if (ctx->pos + len > ctx->len)
	{
		ctx->len += len + 2048;
		ctx->buf = sresize(ctx->buf, ctx->len, char);
	}
	
	memcpy(ctx->buf + ctx->pos, buf, len);
	ctx->pos += len;
}

namespace PuzzleModern
{
	char *WindowsModern::ToChars(Platform::String ^input)
	{
		char *ret;
		auto start = input->Data();
		int newlen = WideCharToMultiByte(CP_ACP, 0, start, input->Length(), NULL, 0, NULL, NULL);
		ret = snewn(newlen + 1, char);
		WideCharToMultiByte(CP_ACP, 0, start, input->Length(), ret, newlen, NULL, NULL);
		ret[newlen] = '\0';

		return ret;
	}

	Puzzle^ WindowsModern::FromConstGame(const game *g)
	{
		auto p = ref new Puzzle();
		p->HelpName = FromChars(g->htmlhelp_topic);
		p->Name = FromChars(g->name);
		return p;
	}

	PuzzleList^ WindowsModern::GetPuzzleList()
	{
		PuzzleList ^ret = ref new PuzzleList();
		Puzzle ^p;
		int i;

		for (i = 0; i < gamecount; i++)
		{
			const game* g = gamelist[i];
			p = FromConstGame(g);
			p->Index = i;
			p->Synopsis = helpitems[i].synopsis;

			ret->AddPuzzle(p);
		}

		return ret;
	}

	Puzzle^ WindowsModern::GetCurrentPuzzle()
	{
		return FromConstGame(ourgame);
	}

	PresetList^ WindowsModern::GetPresetList(bool includeCustom)
	{
		PresetList ^ret = ref new PresetList();
		Preset ^p;
		int i;
		fe->npresets = midend_num_presets(this->me);

		char *name = NULL;
		game_params *params = NULL;

		for (i = 0; i < fe->npresets; i++)
		{
			midend_fetch_preset(this->me, i, &name, &params);

			p = ref new Preset();
			p->Index = i;
			p->Name = FromChars(name);
			ret->AddPreset(p);
		}

		if (this->ourgame->can_configure)
		{
			ret->Custom = true;
			if (includeCustom)
			{
				p = ref new Preset();
				p->Index = -1;
				p->Name = "Custom...";
				ret->AddPreset(p);
			}
		}

		int n = midend_which_preset(me);
		ret->Current = n >= 0 ? n : -1;

		return ret;
	}

	int WindowsModern::GetCurrentPreset()
	{
		return midend_which_preset(me);
	}

	void WindowsModern::SetPreset(int i)
	{
		if (i < 0 || i >= this->fe->npresets)
			throw ref new Platform::InvalidArgumentException("Preset index is out of bounds");

		char *name = NULL;
		game_params *params = NULL;
		midend_fetch_preset(this->me, i, &name, &params);
		midend_set_params(this->me, params);
	}

	void WindowsModern::NewGame(Windows::Foundation::IAsyncAction^ workItem)
	{
		_generating = true;
		currentWorkItem = workItem;
		cancellable = true;
		try
		{
			midend_new_game(this->me);
			_generating = false;
		}
		catch (std::exception ex)
		{
			OutputDebugString(L"Operation has been canceled\n");
			// TODO this will leak memory everywhere. Can the objects created during generation be freed?
		}
		cancellable = false;
		currentWorkItem = nullptr;
	}

	Platform::String ^WindowsModern::Solve()
	{
		char *msg = midend_solve(me);
		if (msg)
			return FromChars(msg);
		return nullptr;
	}

	bool WindowsModern::CanPrint()
	{
		return this->ourgame->can_print != 0;
	}

	bool WindowsModern::CanSolve()
	{
		return this->ourgame->can_solve != 0;
	}

	void WindowsModern::RestartGame()
	{
		midend_restart_game(me);
	}

	bool WindowsModern::CanUndo()
	{
		return midend_can_undo(me) != 0;
	}

	void WindowsModern::Undo()
	{
		midend_process_key(me, 0, 0, 'u');
	}

	bool WindowsModern::CanRedo()
	{
		return midend_can_redo(me) != 0;
	}

	void WindowsModern::Redo()
	{
		midend_process_key(me, 0, 0, '\x12');
	}

	int WindowsModern::GameWon()
	{
		return midend_status(me);
	}

	bool WindowsModern::HasStatusbar()
	{
		return midend_wants_statusbar(me) != 0;
	}

	void WindowsModern::SendKey(VirtualKey key, bool modifier)
	{
		int button;

		switch (key)
		{
		case VirtualKey::Left:
			button = CURSOR_LEFT;
			break;
		case VirtualKey::Right:
			button = CURSOR_RIGHT;
			break;
		case VirtualKey::Up:
			button = CURSOR_UP;
			break;
		case VirtualKey::Down:
			button = CURSOR_DOWN;
			break;
		case VirtualKey::Enter:
			button = CURSOR_SELECT;
			break;
		case VirtualKey::Space:
			button = CURSOR_SELECT2;
			break;
		default:
			button = (int)key;
			break;
		}

		if (key >= VirtualKey::NumberPad0 && key <= VirtualKey::NumberPad9)
			button = MOD_NUM_KEYPAD | (int)(key - (VirtualKey::NumberPad0 - VirtualKey::Number0));

		if (modifier)
			button |= MOD_SHFT;

		midend_process_key(me, 0, 0, button);
	}

	void WindowsModern::SetInputScale(float scale)
	{
		_inputScale = scale;
	}

	void WindowsModern::SendClick(int x, int y, ButtonType type, ButtonState state)
	{
		x *= _inputScale;
		y *= _inputScale;
		switch (type)
		{
		case ButtonType::LEFT:
			if (state == ButtonState::DOWN || state == ButtonState::TAP)
				midend_process_key(me, x, y, LEFT_BUTTON);
			if (state == ButtonState::DRAG)
				midend_process_key(me, x, y, LEFT_DRAG);
			if (state == ButtonState::UP || state == ButtonState::TAP)
				midend_process_key(me, x, y, LEFT_RELEASE);
			break;
		case ButtonType::MIDDLE:
			if (state == ButtonState::DOWN || state == ButtonState::TAP)
				midend_process_key(me, x, y, MIDDLE_BUTTON);
			if (state == ButtonState::DRAG)
				midend_process_key(me, x, y, MIDDLE_DRAG);
			if (state == ButtonState::UP || state == ButtonState::TAP)
				midend_process_key(me, x, y, MIDDLE_RELEASE);
			break;
		case ButtonType::RIGHT:
			if (state == ButtonState::DOWN || state == ButtonState::TAP)
				midend_process_key(me, x, y, RIGHT_BUTTON);
			if (state == ButtonState::DRAG)
				midend_process_key(me, x, y, RIGHT_DRAG);
			if (state == ButtonState::UP || state == ButtonState::TAP)
				midend_process_key(me, x, y, RIGHT_RELEASE);
			break;
		}
	}

	WindowsModern::WindowsModern()
		: me(NULL), fe(NULL), _inputScale(1)
	{
		_generating = false;
	}

	WindowsModern::~WindowsModern()
	{
		OutputDebugString(L"DESTROYING...\n");
		if (!_generating)
		{
			Destroy();
		}
		else
		{
			// TODO: Freeing this object while generation is in progress will cause some objects to not be freed.
			// If the app is terminated while suspended, this memory will free again.
			// Figure out a way to abort generation while also freeing all resources.
			OutputDebugString(L"WARNING: Puzzle generation still in progress!\n");
			if (currentWorkItem)
				currentWorkItem->Cancel();
		}
	}

	void WindowsModern::ReloadColors()
	{
		// Initialize all colours
		int i, ncolours;
		float *colours;
		colours = midend_colours(me, &ncolours);
		for (i = 0; i < ncolours; i++)
		{
			canvas->AddColor(colours[i * 3 + 0], colours[i * 3 + 1], colours[i * 3 + 2]);
		}
		sfree(colours);
	}

	bool WindowsModern::CreateForGame(Platform::String ^name, IPuzzleCanvas ^icanvas, IPuzzleTimer ^itimer)
	{
		this->me = NULL;
		this->fe = snew(frontend);
		this->fe->npresets = 0;
		this->fe->configs = NULL;
		this->canvas = icanvas;
		this->fe->canvas = (void *)&this->canvas;
		this->timer = itimer;
		this->fe->timer = (void *)&this->timer;
		
		char *find = ToChars(name);
		int i;
		for (i = 0; i < gamecount; i++)
		{
			if (!strcmp(find, gamelist[i]->name))
				break;
		}
		sfree(find);

		if (i == gamecount)
			throw ref new Platform::InvalidArgumentException("name");

		set_frontend_adapter(&winmodern_adapter);
		const game *g = gamelist[i];
		this->me = midend_new(this->fe, g, &winmodern_drawing, this->fe);
		this->ourgame = g;

		ReloadColors();

		auto settings = ApplicationData::Current->LocalSettings->Values;

		if (settings->HasKey(name))
			return true;

		return false;
	}
	
	concurrency::task<bool> WindowsModern::LoadGameFromStorage(Platform::String ^name)
	{
		return concurrency::create_task([=](){
			auto settings = ApplicationData::Current->LocalSettings->Values;
			auto savedObj = settings->Lookup(name);
			// If loading the game crashes for whatever reason, the safest bet is to not load it again
			settings->Remove(name);

			auto savedName = safe_cast<Platform::String^>(savedObj);

			return ApplicationData::Current->LocalFolder->GetFileAsync(savedName);
		}).then([=](StorageFile ^file){
			return LoadGameFromFile(file);
		}).then([=](Platform::String^ error){
			// If loading fails, just make a new game
			return error == nullptr;
		});
	}

	concurrency::task<Platform::String^> WindowsModern::LoadGameFromTemporary()
	{
		return concurrency::create_task([](){
			return ApplicationData::Current->TemporaryFolder->GetFileAsync("temp.puzzle");
		}).then([=](concurrency::task<StorageFile^> prevTask){
			StorageFile ^file = nullptr;
			try
			{
				file = prevTask.get();
			}
			catch (Platform::Exception^)
			{
			}

			return LoadGameFromFile(file);
		});
	}

	concurrency::task<Platform::String^> WindowsModern::LoadGameFromFile(Windows::Storage::StorageFile ^file)
	{
		return concurrency::create_task([file](){
			return FileIO::ReadTextAsync(file);
		}).then([=](concurrency::task<Platform::String^> prevTask){
			Platform::String ^saved;
			try
			{
				saved = prevTask.get();
			}
			catch (Platform::Exception^)
			{
				return ref new Platform::String(L"The file was corrupted.");
			}

			return LoadGameFromString(saved);
		});
	}

	Platform::String ^WindowsModern::LoadGameFromString(Platform::String ^saved)
	{
		char *old = ToChars(saved);
		char *p = old;
		const char *err = midend_deserialise(me, winmodern_read_chars, &p);
		sfree(old);

		return FromChars(err);
	}

	concurrency::task<GameLaunchParameters^> WindowsModern::LoadAndIdentify(StorageFile ^inputFile)
	{
		return concurrency::create_task([inputFile](){
			return FileIO::ReadTextAsync(inputFile);
		}).then([inputFile](concurrency::task<Platform::String^> prevTask){
			Platform::String ^saved;
			try
			{
				saved = prevTask.get();
			}
			catch (Platform::Exception^)
			{
				auto ret = ref new GameLaunchParameters();
				ret->Error = inputFile ? "The file was corrupted." : "Not a puzzle file";
				return ret;
			}

			char *old = ToChars(saved);
			char *p = old;
			char *name = NULL;
			const char *err = identify_game(&name, winmodern_read_chars, &p);
			sfree(old);

			if (name)
			{
				int i;
				for (i = 0; i < gamecount; i++)
				{
					if (!strcmp(name, gamelist[i]->name))
						break;
				}

				if (i == gamecount)
					err = "This file is for a puzzle that isn't part of this collection";
			}

			auto ret = ref new GameLaunchParameters();
			if (err)
				ret->Error = FromChars(err);
			else
			{
				ret->Name = FromChars(name, true);
				ret->SaveFile = inputFile;
			}
			return ret;
		});
	}

	Platform::String ^WindowsModern::SaveGameToString()
	{
		PuzzleModern::write_save_context *ctx = snew(PuzzleModern::write_save_context);
		ctx->len = 2048;
		ctx->buf = snewn(ctx->len, char);
		ctx->pos = 0;

		midend_serialise(me, winmodern_write_chars, ctx);

		ctx->buf[ctx->pos] = '\0';

		Platform::String ^save = FromChars(ctx->buf);
		sfree(ctx->buf);
		sfree(ctx);

		return save;
	}


	concurrency::task<bool> WindowsModern::SaveGameToFile(Windows::Storage::StorageFile ^file)
	{
		return concurrency::create_task([=](){
			auto save = SaveGameToString();
			return FileIO::WriteTextAsync(file, save);
		}).then([=](concurrency::task<void> prevTask){
			try
			{
				prevTask.get();
			}
			catch (Platform::Exception^)
			{
				return false;
			}

			return true;
		});
	}

	
	concurrency::task<bool> WindowsModern::SaveGameToStorage(Platform::String ^name)
	{
		return concurrency::create_task([=]{
			ApplicationData::Current->LocalSettings->Values->Remove(name);

			return ApplicationData::Current->LocalFolder->CreateFileAsync(name + ".puzzle", CreationCollisionOption::ReplaceExisting);
		}).then([=](StorageFile ^file){
			return SaveGameToFile(file);
		}).then([=](bool prevTask){
			if (prevTask)
				ApplicationData::Current->LocalSettings->Values->Insert(name, name + ".puzzle");
			return prevTask;
		});
	}

	Platform::String^ WindowsModern::GetRandomSeed()
	{
		return FromChars(midend_get_random_seed(me), true);
	}

	Platform::String^ WindowsModern::SetGameID(Platform::String ^newId)
	{
		char *input = ToChars(newId);

		const char *err = midend_game_id(me, input);
		sfree(input);

		if (err)
			return FromChars(err);

		return nullptr;
	}

	Platform::String^ WindowsModern::GetGameDesc()
	{
		return FromChars(midend_get_game_id(me), true);
	}

	Windows::Foundation::Collections::IVector<ConfigItem^>^ WindowsModern::GetConfiguration()
	{
		if (!this->ourgame->can_configure)
			return nullptr;

		auto ret = ref new Vector<ConfigItem^>();

		sfree(fe->configs);

		char *title = NULL;
		fe->configs = midend_get_config(me, CFG_SETTINGS, &title);
		sfree(title);
		config_item *i;
		
		for (i = fe->configs; i->type != C_END; i++)
		{
			ConfigItem ^item = ref new ConfigItem();
			item->Field = i->type == C_BOOLEAN ? ConfigField::BOOLEAN : 
				i->type == C_CHOICES ? ConfigField::ENUM : 
				i->ival == CONFIGS_STRING ? ConfigField::TEXT :
				i->ival == CONFIGS_FLOAT ? ConfigField::FLOAT : 
				ConfigField::INTEGER;

			item->Label = FromChars(i->name);
			if (i->type == C_STRING)
				item->StringValue = FromChars(i->sval);
			item->IntValue = i->ival;

			if (i->type == C_CHOICES)
			{
				item->StringValues = ref new Vector<Platform::String^>();

				/* Parse the list of choices. Copied from windows.c */
				char c = i->sval[0];
				char *str, *q, *p = i->sval + 1;
				while (*p) {
					q = p;
					while (*q && *q != c) q++;
					str = snewn(q - p + 1, char);
					strncpy(str, p, q - p);
					str[q - p] = '\0';
					item->StringValues->Append(FromChars(str));
					sfree(str);
					if (*q) q++;
					p = q;
				}
			}
			ret->Append(item);
		}

		return ret;
	}

	Platform::String^ WindowsModern::SetConfiguration(Windows::Foundation::Collections::IVector<ConfigItem^>^ input)
	{
		int idx = 0;
		config_item *i;
		for (i = fe->configs; i->type != C_END; i++, idx++)
		{
			if (i->type == C_BOOLEAN || i->type == C_CHOICES)
				i->ival = input->GetAt(idx)->IntValue;
			if (i->type == C_STRING)
				i->sval = ToChars(input->GetAt(idx)->StringValue);
		}
		char *err = midend_set_config(me, CFG_SETTINGS, fe->configs);
		if (err)
			return FromChars(err);
		return nullptr;
	}

	void WindowsModern::Redraw(IPuzzleCanvas ^icanvas, IPuzzleStatusBar ^ibar, IPuzzleTimer ^timer, bool force)
	{
		// Keep the references up to date
		this->canvas = icanvas;
		this->fe->canvas = (void *)&this->canvas;
		this->statusbar = ibar;
		this->fe->statusbar = (void *)&this->statusbar;
		this->timer = timer;
		this->fe->timer = (void *)&this->timer;

		if (force)
		{
			icanvas->ClearAll();
			midend_force_redraw(me);
		}
		else
			midend_redraw(me);

	}

	Size WindowsModern::PrintGame(IPuzzleCanvas ^icanvas, bool addSolution)
	{
		IPuzzleCanvas ^previous = this->canvas;
		this->canvas = icanvas;
		this->fe->canvas = (void *)&this->canvas;

		document *doc = document_new(1, 1, 1);
		midend_print_puzzle(me, doc, addSolution);

		fe->printdr = drawing_new(&winmodern_drawing, NULL, fe);
		fe->printw = 0;
		fe->printh = 0;

		icanvas->SetPrintMode(true);
		icanvas->StartDraw();
		document_print(doc, fe->printdr);

		drawing_free(fe->printdr);
		document_free(doc);
		icanvas->EndDraw();
		icanvas->SetPrintMode(false);
		this->canvas = previous;
		this->fe->canvas = (void *)&this->canvas;
		
		return Size(fe->printw, fe->printh);
	}

	void WindowsModern::UpdateTimer(IPuzzleCanvas ^icanvas, IPuzzleStatusBar ^ibar, IPuzzleTimer ^timer, float delta)
	{
		this->canvas = icanvas;
		this->fe->canvas = (void *)&this->canvas;
		this->statusbar = ibar;
		this->fe->statusbar = (void *)&this->statusbar;
		this->timer = timer;
		this->fe->timer = (void *)&this->timer;
		midend_timer(me, delta);
	}

	void WindowsModern::Destroy()
	{
		try
		{
			if (me)
			{
				midend_free(me);
				me = NULL;
			}
			if (fe)
			{
				sfree(fe);
				fe = NULL;
			}
			this->ourgame = NULL;
			this->canvas = nullptr;
			this->statusbar = nullptr;
			this->timer = nullptr;
		}
		catch (...){}
	}

	void WindowsModern::GetMaxSize(int iw, int ih, int *ow, int *oh)
	{
		*ow = iw;
		*oh = ih;

		midend_size(me, ow, oh, TRUE);
	}

	VirtualButtonCollection^ WindowsModern::GetVirtualButtonBar()
	{
		auto collection = ref new VirtualButtonCollection();
		auto ret = collection->Buttons;
		VirtualButton ^button;
		int i;
		char *params = midend_get_game_id(me);

		if (!strcmp(ourgame->name, "Undead"))
		{
			button = ref new VirtualButton();
			button->Name = "Ghost";
			button->Key = VirtualKey::G;
			ret->Append(button);

			button = ref new VirtualButton();
			button->Name = "Vampire";
			button->Key = VirtualKey::V;
			ret->Append(button);

			button = ref new VirtualButton();
			button->Name = "Zombie";
			button->Key = VirtualKey::Z;
			ret->Append(button);

			ret->Append(VirtualButton::Backspace());

			collection->ToolButton = VirtualButton::Pencil();
		}
		if (!strcmp(ourgame->name, "Filling"))
		{
			for (i = 1; i <= 9; i++)
				ret->Append(VirtualButton::FromNumber(i));

			ret->Append(VirtualButton::Backspace());
		}
		if (!strcmp(ourgame->name, "Guess") || !strcmp(ourgame->name, "Map") || 
			!strcmp(ourgame->name, "Same Game") || !strcmp(ourgame->name, "Flood"))
		{
			button = ref new VirtualButton();
			button->Name = "Labels";
			button->Key = VirtualKey::L;
			button->Type = VirtualButtonType::COLORBLIND;
			collection->ColorBlindKey = button;
		}
		if (!strcmp(ourgame->name, "Net"))
		{
			button = ref new VirtualButton();
			button->Name = "Jumble";
			button->Key = VirtualKey::J;
			button->Type = VirtualButtonType::TOOL;
			button->Icon = ref new Windows::UI::Xaml::Controls::SymbolIcon(Windows::UI::Xaml::Controls::Symbol::Shuffle);
			ret->Append(button);
			collection->ToolButton = button;
		}
		if (!strcmp(ourgame->name, "Solo"))
		{
			int x, y;
			int j = 0;
			if (sscanf(params, "%dx%d", &x, &y) == 2) {
				j = x*y; // Rectangle mode
			}
			else
				j = atoi(params);
			
			for (i = 1; i <= j; i++)
				ret->Append(VirtualButton::FromNumber(i));

			ret->Append(VirtualButton::Backspace());
			collection->ToolButton = VirtualButton::Pencil();
		}
		if (!strcmp(ourgame->name, "Keen") || !strcmp(ourgame->name, "Towers"))
		{
			int n = atoi(params);
			for (i = 1; i <= n; i++)
				ret->Append(VirtualButton::FromNumber(i));
			ret->Append(VirtualButton::Backspace());
			collection->ToolButton = VirtualButton::Pencil();
		}
		if (!strcmp(ourgame->name, "Unequal"))
		{
			// Size 10 and up include a 0 button
			int s = 1;
			int n = atoi(params);
			if (n > 9)
			{
				n--;
				s--;
			}
			for (i = s; i <= n; i++)
				ret->Append(VirtualButton::FromNumber(i));
			ret->Append(VirtualButton::Backspace());
			collection->ToolButton = VirtualButton::Pencil();
		}
		if (!strcmp(ourgame->name, "Dominosa"))
		{
			collection->ToolButton = VirtualButton::ToggleButton("Mark lines", Windows::UI::Xaml::Controls::Symbol::Highlight);
		}
		if (!strcmp(ourgame->name, "Signpost"))
		{
			collection->ToolButton = VirtualButton::ToggleButton("Invert", Windows::UI::Xaml::Controls::Symbol::Switch);
		}
		if (!strcmp(ourgame->name, "Magnets") || !strcmp(ourgame->name, "Map") || !strcmp(ourgame->name, "Train Tracks") || !strcmp(ourgame->name, "Loopy"))
		{
			collection->ToolButton = VirtualButton::Pencil();
		}
		if (!strcmp(ourgame->name, "Mines"))
		{
			collection->ToolButton = VirtualButton::ToggleButton("Flag", Windows::UI::Xaml::Controls::Symbol::Flag);
		}
		if (!strcmp(ourgame->name, "Pattern"))
		{
			collection->ToolButton = VirtualButton::ToggleButton("Color", Windows::UI::Xaml::Controls::Symbol::Edit);
		}

		sfree(params);
		return collection;
	}
}
