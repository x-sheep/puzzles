#include "pch.h"
#include "PuzzleData.h"

using namespace Windows::Foundation::Collections;
using namespace Platform::Collections;

namespace PuzzleModern
{
	PuzzleList::PuzzleList()
	{
		_items = ref new Vector<Platform::Object^>();
	}

	void PuzzleList::AddPuzzle(Puzzle^ p)
	{
		_items->Append(p);
	}

	Puzzle::Puzzle()
	{
	}

	PresetList::PresetList()
	{
		_items = ref new Vector<Platform::Object^>();
		Custom = false;
	}

	void PresetList::AddPreset(Preset^ p)
	{
		_items->Append(p);
	}

	Preset::Preset()
	{
	}

	ConfigItem::ConfigItem()
		: _label(nullptr), _sval(nullptr), _strings(nullptr)
	{
	}

	VirtualButtonCollection::VirtualButtonCollection()
	{
		_buttons = ref new Vector<VirtualButton^>();
		_colorBlind = nullptr;
	}

	VirtualButton::VirtualButton() : _type(VirtualButtonType::INPUT)
	{
	}

	VirtualButton ^VirtualButton::FromNumber(int c)
	{
		auto ret = ref new VirtualButton();

		if (c < 10)
		{
			ret->Name = c.ToString();
			ret->Key = Windows::System::VirtualKey(c + (int)Windows::System::VirtualKey::Number0);
		}
		else
		{
			c -= 10;
			wchar_t ch = (wchar_t)(c + L'A');
			ret->Name = ref new Platform::String(&ch, 1);
			ret->Key = Windows::System::VirtualKey(c + (int)Windows::System::VirtualKey::A);
		}
		return ret;
	}
}