#include "pch.h"

using namespace Windows::System;
using namespace Windows::Data::Json;
using namespace Windows::Foundation::Collections;
using namespace Platform::Collections;

namespace PuzzleCommon
{
	PuzzleList::PuzzleList()
	{
		_items = ref new Vector<Puzzle^>();
		_favourites = ref new Vector<Puzzle^>();
	}

	void PuzzleList::AddPuzzle(Puzzle^ p)
	{
		_items->Append(p);
	}

	void PuzzleList::AddFavourite(Puzzle^ p)
	{
		for (unsigned int i = 0; i < _favourites->Size; i++)
		{
			if (_favourites->GetAt(i)->Name > p->Name)
			{
				_favourites->InsertAt(i, p);
				return;
			}
		}
		_favourites->Append(p);
	}

	void PuzzleList::RemoveFavourite(Puzzle^ p)
	{
		unsigned int idx = 0;
		if (_favourites->IndexOf(p, &idx))
			_favourites->RemoveAt(idx);
	}

	bool PuzzleList::IsFavourite(Puzzle ^p)
	{
		unsigned int idx = 0;
		return _favourites->IndexOf(p, &idx);
	}

	Puzzle::Puzzle()
	{
	}

	PresetList::PresetList()
	{
		_items = ref new Vector<Preset^>();
		Custom = false;
	}

	void PresetList::AddPreset(Preset^ p)
	{
		_items->Append(p);
	}


	Platform::String ^PresetList::GetCurrentPresetName()
	{
		for each(auto item in Items)
		{
			if (item->Checked && item->SubMenu)
				return item->SubMenu->GetCurrentPresetName();
			else if (item->Checked)
				return item->Name;
		}
		return "Custom";
	}


	bool PresetList::CheckPresetItem(int id)
	{
		bool result = false;
		for each(auto item in Items)
		{
			if (item->SubMenu)
				item->Checked = item->SubMenu->CheckPresetItem(id);
			else
				item->Checked = item->Index == id;
			result |= item->Checked;
		}
		return result;
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
		_colorBlind = VirtualKey::None;
		_toolButton = nullptr;
		_leftAction = ButtonType::LEFT;
		_middleAction = ButtonType::MIDDLE;
		_rightAction = ButtonType::RIGHT;
		_touchAction = ButtonType::LEFT;
		_holdAction = ButtonType::RIGHT;
	}

	VirtualButton::VirtualButton() : _icon(nullptr)
	{
	}
}
