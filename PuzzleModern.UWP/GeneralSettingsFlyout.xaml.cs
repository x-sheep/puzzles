using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Storage;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace PuzzleModern.UWP
{
    public sealed partial class GeneralSettingsFlyout
    {
        bool _loaded = false;
        public GeneralSettingsFlyout()
        {
            this.InitializeComponent();

            var settings = ApplicationData.Current.RoamingSettings.Values;
            ColorblindSwitch.IsOn = settings["cfg_colorblind"] as bool? ?? false;
            VictoryFlashSwitch.IsOn = settings["env_DISABLE_VICTORY"] as bool? != true;
            NewGameSwitch.IsOn = settings["cfg_newgameprompt"] as bool? ?? true;

            if (settings["env_MAP_VIVID_COLOURS"] as bool? == true)
            {
                MapPaletteBox.SelectedIndex = 1;
                VividPalettePreview.Visibility = Visibility.Visible;
                DefaultPalettePreview.Visibility = Visibility.Collapsed;
            }
            if (settings["env_FIXED_PENCIL_MARKS"] as bool? == true)
            {
                FixedPencilMarksBox.SelectedIndex = 1;
                FixedPencilPreview.Visibility = Visibility.Visible;
                SequentialPencilPreview.Visibility = Visibility.Collapsed;
            }

            _loaded = true;
        }

        private void NewGameSwitch_Toggled(object sender, RoutedEventArgs e)
        {
            if (!_loaded) return;
            ApplicationData.Current.RoamingSettings.Values["cfg_newgameprompt"] = NewGameSwitch.IsOn;
            App.Current.NotifySettingChanged("cfg_newgameprompt", NewGameSwitch.IsOn);
        }

        private void ColorblindSwitch_Toggled(object sender, RoutedEventArgs e)
        {
            if (!_loaded) return;

            ApplicationData.Current.RoamingSettings.Values["cfg_colorblind"] = ColorblindSwitch.IsOn;
            App.Current.NotifySettingChanged("cfg_colorblind", ColorblindSwitch.IsOn);
        }

        private void VictoryFlashSwitch_Toggled(object sender, RoutedEventArgs e)
        {
            if (!_loaded) return;

            ApplicationData.Current.RoamingSettings.Values["env_DISABLE_VICTORY"] = !VictoryFlashSwitch.IsOn;
            App.Current.NotifySettingChanged("env_DISABLE_VICTORY", !VictoryFlashSwitch.IsOn);
        }

        private void MapPaletteBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (!_loaded) return;
            bool isVivid = MapPaletteBox.SelectedIndex == 1;
            if (isVivid)
            {
                VividPalettePreview.Visibility = Visibility.Visible;
                DefaultPalettePreview.Visibility = Visibility.Collapsed;
            }
            else
            {
                VividPalettePreview.Visibility = Visibility.Collapsed;
                DefaultPalettePreview.Visibility = Visibility.Visible;
            }

            ApplicationData.Current.RoamingSettings.Values["env_MAP_VIVID_COLOURS"] = isVivid;
            App.Current.NotifySettingChanged("env_MAP_VIVID_COLOURS", isVivid);
        }

        private void FixedPencilMarksBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (!_loaded) return;
            bool isFixed = FixedPencilMarksBox.SelectedIndex == 1;
            if (isFixed)
            {
                FixedPencilPreview.Visibility = Visibility.Visible;
                SequentialPencilPreview.Visibility = Visibility.Collapsed;
            }
            else
            {
                FixedPencilPreview.Visibility = Visibility.Collapsed;
                SequentialPencilPreview.Visibility = Visibility.Visible;
            }

            ApplicationData.Current.RoamingSettings.Values["env_FIXED_PENCIL_MARKS"] = isFixed;
            App.Current.NotifySettingChanged("env_FIXED_PENCIL_MARKS", isFixed);
        }
    }
}
