#include "pch.h"
#include "PuzzleData.h"

using namespace Windows::Data::Json;
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
		_toolButton = nullptr;
	}

	VirtualButton::VirtualButton() : _type(VirtualButtonType::INPUT), _icon(nullptr)
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

	GameLaunchParameters ^GameLaunchParameters::Parse(Platform::String ^serialized)
	{
		if (!serialized) return nullptr;

		// Puzzle name only
		if (serialized->Begin()[0] != L'{')
			return ref new GameLaunchParameters(serialized);

		auto ret = ref new GameLaunchParameters();
		
		auto json = JsonObject::Parse(serialized);
		if (json->HasKey("Name"))
			ret->Name = json->GetNamedString("Name");
		if (json->HasKey("TempFile"))
			ret->LoadTempFile = json->GetNamedBoolean("TempFile");
		if (json->HasKey("GameId"))
			ret->GameID = json->GetNamedString("GameId");
		if (json->HasKey("Error"))
			ret->Error = json->GetNamedString("Error");

		return ret;
	}

	Platform::String ^GameLaunchParameters::Stringify()
	{
		if (!_loadTempFile && !_gameID && !_error)
			return _name;

		auto json = ref new JsonObject();
		json->Insert("Name", JsonValue::CreateStringValue(_name));

		if (_loadTempFile)
			json->Insert("TempFile", JsonValue::CreateBooleanValue(true));
		if (_gameID)
			json->Insert("GameId", JsonValue::CreateStringValue(_gameID));
		if (_error)
			json->Insert("Error", JsonValue::CreateStringValue(_error));

		return json->Stringify();
	}
}
