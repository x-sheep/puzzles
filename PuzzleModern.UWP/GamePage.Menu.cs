using PuzzleCommon;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.ApplicationModel.DataTransfer;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Storage;
using Windows.Storage.Pickers;
using Windows.UI.Core;
using Windows.UI.Popups;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

namespace PuzzleModern.UWP
{
    public sealed partial class GamePage
    {
        private void BackButton_Click(object sender, RoutedEventArgs e)
        {
            if (Window.Current.Content is Frame rootFrame && rootFrame.CanGoBack)
                rootFrame.GoBack();
        }

        private void ButtonLeftRight_Checked(object sender, RoutedEventArgs e)
        {
            if (_controls.SwitchMiddle)
            {
                _leftAction = _controls.MiddleAction;
                _middleAction = _controls.LeftAction;
            }
            else
            {
                _leftAction = _controls.RightAction;
                _rightAction = _controls.LeftAction;
            }
            _touchAction = _controls.HoldAction;
            _holdAction = _controls.TouchAction;

            if (!_generatingGame)
                fe.SendKey(WindowsModern.ButtonMarkOn, false, false);
        }

        private void ButtonLeftRight_Unchecked(object sender, RoutedEventArgs e)
        {
            _leftAction = _controls.LeftAction;
            _middleAction = _controls.MiddleAction;
            _rightAction = _controls.RightAction;
            _touchAction = _controls.TouchAction;
            _holdAction = _controls.HoldAction;

            if (!_generatingGame)
                fe.SendKey(WindowsModern.ButtonMarkOff, false, false);
        }

        private void BusyCancelButton_Click(object sender, RoutedEventArgs e)
        {
            if (generatingWorkItem != null)
            {
                generatingWorkItem.Cancel();
                BusyCancelButton.Visibility = Visibility.Collapsed;
                BusyLabel.Text = "Canceling";
            }
        }

        private void ButtonBar_Click(object sender, ButtonBarPressedEventArgs e)
        {
            if (_generatingGame)
                return;

            fe.SendKey(e.Button.Key, false, false);

            UpdateUndoButtons();
        }


        private void ButtonUndo_Click(object sender, RoutedEventArgs e)
        {
            if (_generatingGame)
                return;

            fe.Undo();
            UpdateUndoButtons();
        }

        private void ButtonRedo_Click(object sender, RoutedEventArgs e)
        {
            if (_generatingGame)
                return;

            fe.Redo();
            UpdateUndoButtons();
        }

        private void ButtonNew_Click(object sender, RoutedEventArgs e)
        {
            if (_generatingGame)
                return;
            fe.Redraw(DrawCanvas, this, this, true);
            BeginNewGame();
        }

        private void ButtonRestart_Click(object sender, RoutedEventArgs e)
        {
            if (_generatingGame)
                return;

            fe.RestartGame();
            UpdateUndoButtons();
        }

        private void ButtonSolve_Click(object sender, RoutedEventArgs e)
        {
            if (_generatingGame)
                return;

            var msg = fe.Solve();
            if (!string.IsNullOrEmpty(msg))
                _ = new MessageDialog(msg.AddPeriods(), "Cannot solve").ShowAsync();
            UpdateUndoButtons();
        }

        private void SpecificGameID_Click(object sender, RoutedEventArgs e)
        {
            if (_generatingGame)
                return;

            _isFlyoutOpen = true;

            var popup = new SpecificDialog(fe.GetGameDesc(), "Game ID");
            popup.Closed += (s, a) => { _isFlyoutOpen = false; };
            popup.NewGameID += PromptPopupButtonOK_Click;
            _ = popup.ShowAsync();
        }

        private void SpecificRandomSeed_Click(object sender, RoutedEventArgs e)
        {
            if (_generatingGame)
                return;

            _isFlyoutOpen = true;

            var popup = new SpecificDialog(fe.GetRandomSeed(), "Random seed");
            popup.Closed += (s, a) => { _isFlyoutOpen = false; };
            popup.NewGameID += PromptPopupButtonOK_Click;
            _ = popup.ShowAsync();
        }

        private void PromptPopupButtonOK_Click(object sender, NewGameIDEventArgs e)
        {
            // The Random Seed and Game Description dialogs use the same handling
            var error = fe.SetGameID(e.NewID);

            if (!string.IsNullOrEmpty(error))
            {
                e.Error = error;
            }
            else
            {
                BeginNewGame();
            }
        }

        private async void SpecificLoadGame_Click(object sender, RoutedEventArgs e)
        {
            var openPicker = new FileOpenPicker
            {
                ViewMode = PickerViewMode.List,
                SuggestedStartLocation = PickerLocationId.DocumentsLibrary,
                CommitButtonText = "Load game",
            };
            openPicker.FileTypeFilter.Add(".puzzle");
            openPicker.FileTypeFilter.Add(".sav");

            var file = await openPicker.PickSingleFileAsync();
            if (file == null) return;

            await App.Current.ActivateFile(file);
        }

        private async void SpecificSaveGame_Click(object sender, RoutedEventArgs e)
        {
            if (_generatingGame)
                return;

            var savePicker = new FileSavePicker
            {
                SuggestedStartLocation = PickerLocationId.DocumentsLibrary,
                CommitButtonText = "Save game",
                SuggestedFileName = _puzzleName
            };
            savePicker.FileTypeChoices.Add("Saved game", new List<string>
            {
                ".puzzle",
                ".sav"
            });

            var file = await savePicker.PickSaveFileAsync();
            if (file == null) return;

            var success = await fe.SaveGameToFile(file);
            if (!success)
                await new MessageDialog("The game could not be saved.").ShowAsync();
        }

        private void SpecificShareGame_Click(object sender, RoutedEventArgs e)
        {
            if (!_generatingGame)
                DataTransferManager.ShowShareUI();
        }

        private void ButtonSettings_Click(object sender, RoutedEventArgs e)
        {
            var cfgs = fe.GetPreferences();
            _isFlyoutOpen = true;
            var flyout = new GeneralSettingsFlyout(_puzzleName, cfgs.ToArray());
            flyout.Unloaded += (s, a) => { _isFlyoutOpen = false; };
            flyout.PreferencesChanged += (s, a) =>
            {
                a.Error = fe.SetPreferences(a.NewConfig);
                if (_isLoaded)
                {
                    if (_puzzleName == "Pearl")
                        ResizeGame();
                    ForceRedraw();
                }
            };
            flyout.ShowIndependent();
        }

        List<MenuFlyoutItemBase> ItemsForPresetList(PresetList list)
        {
            var ret = new List<MenuFlyoutItemBase>();
            foreach (var sub in list.Items)
            {
                if (sub.SubMenu != null)
                {
                    var item = new MenuFlyoutSubItem();
                    item.Text = sub.Name;

                    foreach (var subItem in ItemsForPresetList(sub.SubMenu))
                    {
                        item.Items.Add(subItem);
                    }

                    ret.Add(item);
                }
                else
                {
                    var item = new ToggleMenuFlyoutItem();
                    item.Text = sub.Name;
                    item.IsChecked = sub.Checked;

                    item.Tag = sub.Index;
                    item.Click += PresetMenuFlyout_Click;

                    ret.Add(item);
                }
            }
            return ret;
        }
        void PresetMenuFlyout_Click(object sender, RoutedEventArgs e)
        {
            if (_generatingGame)
                return;

            var item = (MenuFlyoutItem)sender;
            var tag = (int)item.Tag;
            if (item is ToggleMenuFlyoutItem checkItem)
                checkItem.IsChecked = !checkItem.IsChecked;

            if (tag >= 0)
            {
                fe.SetPreset(tag);
                BeginNewGame();
            }
            else
            {
                OpenParamsDialog();
            }
        }

        void OpenParamsDialog()
        {
            if (_generatingGame || _isFlyoutOpen)
                return;

            var cfgs = fe.GetConfiguration();
            var dialog = new ParamsDialog(_puzzleName, cfgs.ToArray());

            dialog.NewConfiguration += (s, e) =>
            {
                var error = fe.SetConfiguration(e.NewConfig);
                if (!string.IsNullOrEmpty(error))
                {
                    e.Error = error;
                }
                else
                {
                    BeginNewGame();
                    (s as ContentDialog)?.Hide();
                    _isFlyoutOpen = false;
                }
            };
            dialog.Closed += (s, e) => { _isFlyoutOpen = false; };

            _isFlyoutOpen = true;
            _ = dialog.ShowAsync();
        }

        private void ButtonAbout_Click(object sender, RoutedEventArgs e)
        {
            _isFlyoutOpen = true;
            var flyout = new AboutFlyout();
            flyout.Unloaded += (s, a) => { _isFlyoutOpen = false; };
            flyout.ShowIndependent();
        }

        private void ButtonHelpContents_Click(object sender, RoutedEventArgs e)
        {
            _isFlyoutOpen = true;
            var flyout = new HelpFlyout();
            flyout.Unloaded += (s, a) => { _isFlyoutOpen = false; };
            flyout.ShowIndependent();
        }

        private void ButtonHelp_Click(object sender, RoutedEventArgs e)
        {
            _isFlyoutOpen = true;
            var flyout = new HelpFlyout(currentPuzzle.HelpName);
            flyout.Unloaded += (s, a) => { _isFlyoutOpen = false; };
            flyout.ShowIndependent();
        }

        private void pageRoot_DragOver(object sender, DragEventArgs e)
        {
            e.AcceptedOperation = DataPackageOperation.Copy;
            e.DragUIOverride.Caption = "Load game";
            e.DragUIOverride.IsContentVisible = false;
        }

        private async void pageRoot_Drop(object sender, DragEventArgs e)
        {
            if (e.DataView.Contains(StandardDataFormats.StorageItems))
            {
                var items = await e.DataView.GetStorageItemsAsync();
                var item = items.FirstOrDefault() as StorageFile;
                if (item != null)
                    _ = App.Current.ActivateFile(item);
            }
        }
    }
}
