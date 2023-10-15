#include "pch.h"
#include "WindowsModern.h"

extern "C" {
#include <windows.h>
#include <stdio.h>

#include "../puzzles.h"
}
#include "IPuzzleCanvas.h"
#include "PuzzleHelpData.h"
#include <ppltasks.h>

using namespace PuzzleCommon;

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


static char* ToChars(Platform::String^ input)
{
	char* ret;
	auto start = input->Data();
	int newlen = WideCharToMultiByte(CP_ACP, 0, start, input->Length(), NULL, 0, NULL, NULL);
	ret = snewn(newlen + 1, char);
	WideCharToMultiByte(CP_ACP, 0, start, input->Length(), ret, newlen, NULL, NULL);
	ret[newlen] = '\0';

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

void winmodern_status_bar(void *handle, const char *text)
{
	frontend *fe = (frontend *)handle;
	IPuzzleStatusBar ^bar = *((IPuzzleStatusBar^*)fe->statusbar);

	bar->UpdateStatusBar(FromChars(text));
}

int winmodern_start_draw(void *handle)
{
	frontend *fe = (frontend *)handle;
	IPuzzleCanvas ^canvas = *((IPuzzleCanvas^*)fe->canvas);

	return canvas->StartDraw();
}

void winmodern_draw_update(void *handle, int x, int y, int w, int h)
{
	frontend *fe = (frontend *)handle;
	IPuzzleCanvas ^canvas = *((IPuzzleCanvas^*)fe->canvas);
	
	canvas->UpdateArea(x, y, w, h);
}

void winmodern_clip(void *handle, int x, int y, int w, int h)
{
	frontend *fe = (frontend *)handle;
	IPuzzleCanvas ^canvas = *((IPuzzleCanvas^*)fe->canvas);
	
	canvas->StartClip(x, y, w, h);
}

void winmodern_unclip(void *handle)
{
	frontend *fe = (frontend *)handle;
	IPuzzleCanvas ^canvas = *((IPuzzleCanvas^*)fe->canvas);
	
	canvas->EndClip();
}

void winmodern_draw_text(void *handle, int x, int y, int fonttype, int fontsize,
	int align, int colour, const char *text)
{
	frontend *fe = (frontend *)handle;
	IPuzzleCanvas ^canvas = *((IPuzzleCanvas^*)fe->canvas);

	GameFontType gamefont = fonttype == FONT_VARIABLE ?
		GameFontType::VariableWidth : 
		GameFontType::FixedWidth;

	GameFontHAlign halign = align & ALIGN_HCENTRE ?
		GameFontHAlign::HorizontalCenter :
		align & ALIGN_HRIGHT ?
		GameFontHAlign::HorizontalRight :
		GameFontHAlign::HorizontalLeft;

	GameFontVAlign valign = align & ALIGN_VCENTRE ?
		GameFontVAlign::VerticalCentre :
		GameFontVAlign::VerticalBase;

	canvas->DrawText(x, y, gamefont, halign, valign, fontsize, colour, FromChars(text));
}

void winmodern_draw_rect(void *handle, int x, int y, int w, int h, int colour)
{
	frontend *fe = (frontend *)handle;
	IPuzzleCanvas ^canvas = *((IPuzzleCanvas^*)fe->canvas);

	canvas->DrawRect(x, y, w, h, colour);
}

void winmodern_draw_line(void *handle, int x1, int y1, int x2, int y2,
	int colour)
{
	frontend *fe = (frontend *)handle;
	IPuzzleCanvas ^canvas = *((IPuzzleCanvas^*)fe->canvas);

	canvas->DrawLine(x1, y1, x2, y2, colour);
}

void winmodern_draw_poly(void *handle, const int *coords, int npoints,
	int fillcolour, int outlinecolour)
{
	frontend *fe = (frontend *)handle;
	IPuzzleCanvas ^canvas = *((IPuzzleCanvas^*)fe->canvas);

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
	IPuzzleCanvas ^canvas = *((IPuzzleCanvas^*)fe->canvas);

	canvas->DrawCircle(cx, cy, radius, fillcolour, outlinecolour);
}

float winmodern_draw_scale(void *handle)
{
	frontend *fe = (frontend *)handle;
	return fe->scale;
}

blitter *winmodern_blitter_new(void *handle, int w, int h)
{
	frontend *fe = (frontend *)handle;

	blitter *bl = snew(blitter);
	bl->handle = fe->next_blitter_id++;
	bl->w = w;
	bl->h = h;

	if (fe->canvas)
	{
		IPuzzleCanvas ^canvas = *((IPuzzleCanvas^*)fe->canvas);
		canvas->BlitterNew(bl->handle, w, h);
	}

	return bl;
}

void winmodern_blitter_free(void *handle, blitter *bl)
{
	frontend *fe = (frontend *)handle;

	if (fe->canvas)
	{
		IPuzzleCanvas ^canvas = *((IPuzzleCanvas^*)fe->canvas);
		canvas->BlitterFree(bl->handle);
	}

	sfree(bl);
}

void winmodern_blitter_save(void *handle, blitter *bl, int x, int y)
{
	frontend *fe = (frontend *)handle;
	IPuzzleCanvas ^canvas = *((IPuzzleCanvas^*)fe->canvas);

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
	IPuzzleCanvas ^canvas = *((IPuzzleCanvas^*)fe->canvas);
	
	if (x == BLITTER_FROMSAVED)
		x = bl->x;
	if (y == BLITTER_FROMSAVED)
		y = bl->y;
	
	canvas->BlitterLoad(bl->handle, max(x, 0), max(y, 0), bl->rw, bl->rh);
}

void winmodern_end_draw(void *handle)
{
	frontend *fe = (frontend *)handle;
	IPuzzleCanvas ^canvas = *((IPuzzleCanvas^*)fe->canvas);

	canvas->EndDraw();
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
	IPuzzleCanvas ^canvas = *((IPuzzleCanvas^*)fe->canvas);

	canvas->SetLineWidth(width);
}

void winmodern_line_dotted(void *handle, bool dotted)
{
	frontend *fe = (frontend *)handle;
	IPuzzleCanvas ^canvas = *((IPuzzleCanvas^*)fe->canvas);

	canvas->SetLineDotted(dotted);
}

void winmodern_add_print_colour(void *handle, int id)
{
	frontend *fe = (frontend *)handle;
	IPuzzleCanvas ^canvas = *((IPuzzleCanvas^*)fe->canvas);

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
	winmodern_draw_scale,
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

	IPuzzleTimer ^timer = *((IPuzzleTimer^*)fe->timer);
	timer->EndTimer();
}

void winmodern_activate_timer(frontend *fe)
{
	if (!fe || !fe->timer) return;

	IPuzzleTimer ^timer = *((IPuzzleTimer^*)fe->timer);
	timer->StartTimer();
}

void winmodern_fatal(const char *msg)
{
	throw ref new Platform::FailureException(FromChars(msg));
}

static concurrency::cancellation_token *cancellationToken = nullptr;
static bool cancellable = false;

void winmodern_check_abort(void)
{
	if (cancellable && (!cancellationToken || cancellationToken->is_canceled()))
	{
		cancellable = false;
		throw std::exception();
	}
}

static char* preset_colours[] = { "008900", "7373e6", "890089"};

char *winmodern_getenv(const char *key)
{
	auto settings = Windows::Storage::ApplicationData::Current->RoamingSettings->Values;

	if (!strcmp(key, "MAP_VIVID_COLOURS"))
	{
		if(settings->HasKey("env_MAP_VIVID_COLOURS") && safe_cast<bool>(settings->Lookup("env_MAP_VIVID_COLOURS")))
			return "Y";
	}

	if (!strcmp(key, "FIXED_PENCIL_MARKS"))
	{
		if (settings->HasKey("env_FIXED_PENCIL_MARKS") && safe_cast<bool>(settings->Lookup("env_FIXED_PENCIL_MARKS")))
			return "Y";
	}

	if (!strcmp(key, "DISABLE_VICTORY"))
	{
		if (settings->HasKey("env_DISABLE_VICTORY") && safe_cast<bool>(settings->Lookup("env_DISABLE_VICTORY")))
			return "Y";
	}

	if (!strcmp(key, "COLOURPRESET_ENTRY"))
	{
		int val = 0;
		if (settings->HasKey("env_COLOURPRESET_ENTRY"))
			val = safe_cast<int>(settings->Lookup("env_COLOURPRESET_ENTRY"));
		val = max(0, min(lenof(preset_colours), val));
		return preset_colours[val];
	}

	if (!strcmp(key, "COLOURPRESET_PENCIL"))
	{
		int val = 1;
		if (settings->HasKey("env_COLOURPRESET_PENCIL"))
			val = safe_cast<int>(settings->Lookup("env_COLOURPRESET_PENCIL"));
		val = max(0, min(lenof(preset_colours), val));
		return preset_colours[val];
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
bool winmodern_read_chars(void *wctx, void *buf, int len)
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
			return false;
	}

	memcpy(buf, p, len);
	*ctx += len;

	return true;
}

// Writing function for serialization
void winmodern_write_chars(void *vctx, const void *buf, int len)
{
	write_save_context *ctx;
	ctx = (write_save_context *)vctx;
	
	if (ctx->pos + len > ctx->len)
	{
		ctx->len += len + 2048;
		ctx->buf = sresize(ctx->buf, ctx->len, char);
	}
	
	memcpy(ctx->buf + ctx->pos, buf, len);
	ctx->pos += len;
}

namespace PuzzleCommon
{
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

		auto settings = ApplicationData::Current->RoamingSettings->Values;

		for (i = 0; i < gamecount; i++)
		{
			const game* g = gamelist[i];
			p = FromConstGame(g);
			p->Index = i;
			p->Synopsis = helpitems[i].synopsis;

			ret->AddPuzzle(p);

			bool isFav = false;
			if (settings->HasKey("fav_" + p->Name))
				isFav = safe_cast<bool>(settings->Lookup("fav_" + p->Name));
			else /* Default list of favourites */
				isFav = p->Name == "Light Up" ||
				p->Name == "Net" ||
				p->Name == "Solo" ||
				p->Name == "Unruly" ||
				p->Name == "Untangle";

			if (isFav)
				ret->AddFavourite(p);
		}

		return ret;
	}

	Puzzle^ WindowsModern::GetCurrentPuzzle()
	{
		return FromConstGame(ourgame);
	}

	static void populate_preset_menu(frontend *fe,
		struct preset_menu *menu, PresetList ^list, int maximumDepth)
	{
		int i;
		for (i = 0; i < menu->n_entries; i++)
		{
			struct preset_menu_entry *entry = &menu->entries[i];

			if (entry->params)
			{
				auto p = ref new Preset();
				p->Index = entry->id;
				p->Name = FromChars(entry->title);
				list->AddPreset(p);
			}
			else if (maximumDepth != 1)
			{
				auto p = ref new Preset();
				p->Index = entry->id;
				p->Name = FromChars(entry->title);
				list->AddPreset(p);
				p->SubMenu = ref new PresetList();
				populate_preset_menu(fe, entry->submenu, p->SubMenu, maximumDepth - 1);
			}
			else
				populate_preset_menu(fe, entry->submenu, list, 1);
		}
	}


	PresetList^ WindowsModern::GetPresetList(bool includeCustom, int maximumDepth)
	{
		if (!presets)
		{
			presets = ref new PresetList();

			if (!fe->preset_menu)
				fe->preset_menu = midend_get_presets(this->me, NULL);

			populate_preset_menu(fe, fe->preset_menu, presets, maximumDepth);

			if (this->ourgame->can_configure)
			{
				presets->Custom = true;
				if (includeCustom)
				{
					auto p = ref new Preset();
					p->Index = -1;
					p->Name = "Custom...";
					presets->AddPreset(p);
				}
			}
		}

		int presetId = midend_which_preset(me);
		presets->CheckPresetItem(presetId);

		return presets;
	}

	bool WindowsModern::IsCustomGame()
	{
		return midend_which_preset(me) == -1;
	}

	void WindowsModern::SetPreset(int i)
	{
		if (!fe->preset_menu)
			fe->preset_menu = midend_get_presets(this->me, NULL);

		game_params *params = preset_menu_lookup_by_id(fe->preset_menu, i);

		if (!params)
			throw ref new Platform::InvalidArgumentException("Invalid preset ID");

		midend_set_params(this->me, params);
	}

	IAsyncOperation<bool> ^WindowsModern::NewGame()
	{
		_generating = true;
		return concurrency::create_async([=](concurrency::cancellation_token token)
		{
			cancellationToken = &token;
			cancellable = true;

			bool success = false;
			try
			{
				midend_new_game(this->me);
				_wonGame = false;
				_generating = false;
				success = true;
			}
			catch (std::exception ex)
			{
				OutputDebugString(L"Operation has been canceled\n");
				// TODO this will leak memory everywhere. Can the objects created during generation be freed?
			}
			cancellable = false;
			cancellationToken = nullptr;

			return success;
		});
	}

	Platform::String ^WindowsModern::Solve()
	{
		const char *msg = midend_solve(me);
		if (msg)
			return FromChars(msg);

		_wonGame = true;
		return nullptr;
	}

	bool WindowsModern::IsRightButtonDisabled()
	{
		return (this->ourgame->flags & DISABLE_RBUTTON) != 0;
	}

	bool WindowsModern::CanPrint()
	{
		return this->ourgame->can_print;
	}

	bool WindowsModern::CanSolve()
	{
		return this->ourgame->can_solve;
	}

	void WindowsModern::RestartGame()
	{
		midend_restart_game(me);
	}

	bool WindowsModern::CanUndo()
	{
		return midend_can_undo(me);
	}

	void WindowsModern::Undo()
	{
		midend_process_key(me, 0, 0, UI_UNDO);
	}

	void WindowsModern::CheckGameCompletion()
	{
		int win = midend_status(me);

		if (win == +1 && !_wonGame)
		{
			_wonGame = true;
			if (midend_just_performed_undo(me))
				return;

			GameCompleted(this, nullptr);
		}

		if (midend_just_performed_redo(me))
			_wonGame = false;
	}

	bool WindowsModern::CanRedo()
	{
		return midend_can_redo(me);
	}

	void WindowsModern::Redo()
	{
		midend_process_key(me, 0, 0, UI_REDO);
	}

	bool WindowsModern::HasStatusbar()
	{
		return midend_wants_statusbar(me) != 0;
	}

	void WindowsModern::SendKey(VirtualKey key, bool shift, bool control)
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

		if (shift)
			button |= MOD_SHFT;
		if (control)
			button |= MOD_CTRL;

		midend_process_key(me, 0, 0, button);
		CheckGameCompletion();
	}

	void WindowsModern::SetInputScale(float scale)
	{
		fe->scale = scale;
	}

	void WindowsModern::SendClick(int x, int y, ButtonType type, ButtonState state)
	{
		x *= fe->scale;
		y *= fe->scale;
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

		CheckGameCompletion();
	}

	WindowsModern::~WindowsModern()
	{
		OutputDebugString(L"DESTROYING...\n");
		if (fe)
		{
			fe->canvas = NULL;
			fe->statusbar = NULL;
			fe->timer = NULL;
		}
		this->canvas = nullptr;
		this->statusbar = nullptr;
		this->timer = nullptr;
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
			cancellationToken = nullptr;
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

	bool WindowsModern::IsValidPuzzleName(Platform::String^ name)
	{
		char* find = ToChars(name);
		int i;
		for (i = 0; i < gamecount; i++)
		{
			if (!strcmp(find, gamelist[i]->name))
				break;
		}
		sfree(find);
		return i != gamecount;
	}

	WindowsModern::WindowsModern(Platform::String ^name, IPuzzleCanvas ^icanvas, IPuzzleStatusBar ^ibar, IPuzzleTimer ^itimer)
	{
		this->_generating = false;
		this->me = NULL;
		this->fe = snew(frontend);
		this->fe->preset_menu = NULL;
		this->fe->configs = NULL;
		this->fe->prefs = NULL;
		this->fe->scale = 1.0f;
		this->fe->next_blitter_id = 0;
		this->canvas = icanvas;
		this->fe->canvas = (void *)&this->canvas;
		this->statusbar = ibar;
		this->fe->statusbar = (void *)&this->statusbar;
		this->timer = itimer;
		this->fe->timer = (void *)&this->timer;
		this->_wonGame = false;
		
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

		auto settings = ApplicationData::Current->RoamingSettings->Values;
		auto prefKey = "pref_" + FromChars(g->name);
		if (settings->HasKey(prefKey)) {
			auto saved = safe_cast<Platform::String^>(settings->Lookup(prefKey));
			char* prefs = ToChars(saved);
			char* p = prefs;
			const char* err = midend_load_prefs(me, winmodern_read_chars, &p);
			sfree(prefs);
		}

		ReloadColors();
	}
	
	IAsyncOperation<bool> ^WindowsModern::LoadGameFromStorage(Platform::String ^name)
	{
		auto task = concurrency::create_task([=](){
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
		return concurrency::create_async([task]() { return task; });
	}

	IAsyncOperation<Platform::String^> ^WindowsModern::LoadGameFromTemporary()
	{
		auto task = concurrency::create_task([](){
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
		return concurrency::create_async([task]() { return task; });
	}

	IAsyncOperation<Platform::String^> ^WindowsModern::LoadGameFromFile(Windows::Storage::StorageFile ^file)
	{
		auto task = concurrency::create_task([file](){
			return FileIO::ReadTextAsync(file);
		}).then([=](concurrency::task<Platform::String^> prevTask){
			Platform::String ^saved;
			try
			{
				saved = prevTask.get();
			}
			catch (Platform::Exception^)
			{
				return ref new Platform::String(L"The file was corrupted or is not a valid puzzle file.");
			}

			return LoadGameFromString(saved);
		});
		return concurrency::create_async([task]() { return task; });
	}

	Platform::String ^WindowsModern::LoadGameFromString(Platform::String ^saved)
	{
		char *old = ToChars(saved);
		char *p = old;
		const char *err = midend_deserialise(me, winmodern_read_chars, &p);

		if (!err)
			_wonGame = midend_status(me) == 1;

		sfree(old);

		return FromChars(err);
	}

	IAsyncOperation<GameLaunchParameters^> ^WindowsModern::LoadAndIdentify(StorageFile ^inputFile)
	{
		auto task = concurrency::create_task([inputFile](){
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
		return concurrency::create_async([task]() { return task; });
	}

	Platform::String ^WindowsModern::SaveGameToString()
	{
		write_save_context *ctx = snew(write_save_context);
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


	IAsyncOperation<bool> ^WindowsModern::SaveGameToFile(Windows::Storage::StorageFile ^file)
	{
		auto task = concurrency::create_task([=](){
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
		return concurrency::create_async([task]() { return task; });
	}

	
	IAsyncOperation<bool> ^WindowsModern::SaveGameToStorage(Platform::String ^name)
	{
		auto task = concurrency::create_task([=]{
			ApplicationData::Current->LocalSettings->Values->Remove(name);

			return ApplicationData::Current->LocalFolder->CreateFileAsync(name + ".puzzle", CreationCollisionOption::ReplaceExisting);
		}).then([=](StorageFile ^file){
			return SaveGameToFile(file);
		}).then([=](bool prevTask){
			if (prevTask)
				ApplicationData::Current->LocalSettings->Values->Insert(name, name + ".puzzle");
			return prevTask;
		});
		return concurrency::create_async([task]() { return task; });
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

	static Windows::Foundation::Collections::IVector<ConfigItem^>^ get_configuration_generic(config_item* configs)
	{
		auto ret = ref new Vector<ConfigItem^>();
		config_item* i;

		for (i = configs; i->type != C_END; i++)
		{
			ConfigItem^ item = ref new ConfigItem();
			item->Label = FromChars(i->name);

			item->Field = i->type == C_BOOLEAN ? ConfigField::BOOLEAN :
				i->type == C_CHOICES ? ConfigField::ENUM :
				item->Label == "No. of balls" ? ConfigField::TEXT :
				item->Label == "Barrier probability" ? ConfigField::FLOAT :
				ConfigField::INTEGER;

			if (i->type == C_STRING)
				item->StringValue = FromChars(i->u.string.sval);
			else if (i->type == C_BOOLEAN)
				item->IntValue = i->u.boolean.bval;
			else if (i->type == C_CHOICES)
			{
				item->StringValues = ref new Vector<Platform::String^>();
				item->IntValue = i->u.choices.selected;

				/* Parse the list of choices. Copied from windows.c */
				char c = i->u.choices.choicenames[0];
				char* str;
				const char* q, * p = i->u.choices.choicenames + 1;
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

	Windows::Foundation::Collections::IVector<ConfigItem^>^ WindowsModern::GetConfiguration()
	{
		if (!this->ourgame->can_configure)
			return nullptr;

		sfree(fe->configs);

		char *title = NULL;
		fe->configs = midend_get_config(me, CFG_SETTINGS, &title);
		sfree(title);
		
		return get_configuration_generic(fe->configs);
	}

	Windows::Foundation::Collections::IVector<ConfigItem^>^ WindowsModern::GetPreferences()
	{
		sfree(fe->prefs);

		char* title = NULL;
		fe->prefs = midend_get_config(me, CFG_PREFS, &title);
		sfree(title);

		return get_configuration_generic(fe->prefs);
	}

	static const char* set_configuration_generic(Windows::Foundation::Collections::IVector<ConfigItem^>^ input, midend* me, config_item* configs, int whichcfg) {
		int idx = 0;
		config_item* i;
		for (i = configs; i->type != C_END; i++, idx++)
		{
			if (i->type == C_BOOLEAN)
				i->u.boolean.bval = input->GetAt(idx)->IntValue != 0;
			else if (i->type == C_CHOICES)
				i->u.choices.selected = input->GetAt(idx)->IntValue;
			else if (i->type == C_STRING)
				i->u.string.sval = ToChars(input->GetAt(idx)->StringValue);
		}
		return midend_set_config(me, whichcfg, configs);
	}

	Platform::String^ WindowsModern::SetConfiguration(Windows::Foundation::Collections::IVector<ConfigItem^>^ input)
	{
		return FromChars(set_configuration_generic(input, me, fe->configs, CFG_SETTINGS));
	}

	Platform::String^ WindowsModern::SetPreferences(Windows::Foundation::Collections::IVector<ConfigItem^>^ input)
	{
		auto err = FromChars(set_configuration_generic(input, me, fe->prefs, CFG_PREFS));
		if (!err) {
			auto settings = Windows::Storage::ApplicationData::Current->RoamingSettings->Values;

			write_save_context* ctx = snew(write_save_context);
			ctx->len = 1024;
			ctx->buf = snewn(ctx->len, char);
			ctx->pos = 0;

			midend_save_prefs(me, winmodern_write_chars, ctx);

			ctx->buf[ctx->pos] = '\0';

			Platform::String^ save = FromChars(ctx->buf);
			sfree(ctx->buf);
			sfree(ctx);

			settings->Insert("pref_" + FromChars(ourgame->name), save);
		}
		return err;
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
		}
		catch (...){}
	}

	void WindowsModern::GetMaxSize(int iw, int ih, int *ow, int *oh)
	{
		*ow = iw;
		*oh = ih;

		midend_size(me, ow, oh, TRUE, 1.0);
	}

	VirtualButtonCollection^ WindowsModern::GetVirtualButtonBar()
	{
		auto collection = ref new VirtualButtonCollection();
		auto ret = collection->Buttons;
		Platform::String ^name = FromChars(ourgame->name);
		int i;

		int nkeys = 0;
		key_label *keys = midend_request_keys(me, &nkeys);

		for (i = 0; i < nkeys; i++)
		{
			auto button = ref new VirtualButton();
			if (keys[i].button == '\b')
			{
				button->Name = L"\u232b";
				button->Key = Windows::System::VirtualKey::Back;
			}
			else
			{
				button->Name = FromChars(keys[i].label);
				if (keys[i].button >= 'a' && keys[i].button <= 'z')
					button->Key = (VirtualKey)(keys[i].button + ('A' - 'a'));
				else
					button->Key = (VirtualKey)(keys[i].button);
			}
			ret->Append(button);
		}

		if (name == "Guess" || name == "Map" || 
			name == "Same Game" || name == "Flood")
		{
			collection->ColorBlindKey = VirtualKey::L;
		}

		if (name == "Net")
		{
			auto button = ref new VirtualButton();
			button->Name = "Jumble";
			button->Key = VirtualKey::J;
			button->Icon = ref new Windows::UI::Xaml::Controls::SymbolIcon(Windows::UI::Xaml::Controls::Symbol::Shuffle);
			ret->Append(button);

			collection->RightAction = ButtonType::MIDDLE;
			collection->MiddleAction = ButtonType::RIGHT;
			collection->SwitchMiddle = true;

			collection->ToggleButton = VirtualButton::ToggleButton("Lock", (Windows::UI::Xaml::Controls::Symbol)0xE1F6);
		}
		else if (name == "Dominosa")
		{
			collection->ToggleButton = VirtualButton::ToggleButton("Mark lines", Windows::UI::Xaml::Controls::Symbol::Highlight);
		}
		else if (name == "Signpost")
		{
			collection->ToggleButton = VirtualButton::ToggleButton("Invert", Windows::UI::Xaml::Controls::Symbol::Switch);
		}
		else if (name == "Mines")
		{
			collection->ToggleButton = VirtualButton::ToggleButton("Flag", Windows::UI::Xaml::Controls::Symbol::Flag);
		}
		else if (name == "Pattern" || name == "Unruly" || name == "Mosaic")
		{
			collection->ToggleButton = VirtualButton::ToggleButton("Color", Windows::UI::Xaml::Controls::Symbol::Edit);
		}
		else if (ourgame->flags & REQUIRE_RBUTTON)
		{
			collection->ToggleButton = VirtualButton::ToggleButton("Mark", Windows::UI::Xaml::Controls::Symbol::Edit);
		}
		
		free_keys(keys, nkeys);

		return collection;
	}
}
