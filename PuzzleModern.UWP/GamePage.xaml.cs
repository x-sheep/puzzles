using PuzzleCommon;
using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using Windows.ApplicationModel.DataTransfer;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Graphics.Display;
using Windows.Storage;
using Windows.System;
using Windows.System.Threading;
using Windows.UI.Core;
using Windows.UI.Popups;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace PuzzleModern.UWP
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class GamePage : Page, IPuzzleStatusBar, IPuzzleTimer
    {
        public IObservableMap<string, object> DefaultViewModel
        {
            get { return (IObservableMap<string, object>)GetValue(_DefaultViewModel); }
            set { SetValue(_DefaultViewModel, value); }
        }

        public readonly DependencyProperty _DefaultViewModel = DependencyProperty.Register(
            nameof(DefaultViewModel),
            typeof(IObservableMap<string, object>),
            typeof(MainPage), null);

        Puzzle currentPuzzle;
        string _puzzleName;
        WindowsModern fe;
        VirtualButtonCollection _controls;

        Task savingWorkItem;

        bool _isFlyoutOpen, _generatingGame, _finishedOverlayAnimation, _hasGame, _isLoaded;

        public GamePage()
        {
            DefaultViewModel = new PropertySet();
            this.InitializeComponent();

            Loaded += OnLoaded;
            Unloaded += OnUnloaded;

            DrawCanvas.NeedsRedraw += ForceRedraw;
            Application.Current.Suspending += OnSuspending;
            Application.Current.Resuming += OnResuming;
            App.Current.SettingChanged += OnSettingChanged;
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            _puzzleName = e.Parameter.ToString();

            fe = new WindowsModern(_puzzleName, DrawCanvas, this, this);
            fe.GameCompleted += OnGameCompleted;
            _hasGame = ApplicationData.Current.LocalSettings.Values.ContainsKey(_puzzleName);

            currentPuzzle = fe.GetCurrentPuzzle();
            ButtonHelp.Text = "Help on " + currentPuzzle.Name;
            DefaultViewModel["PuzzleName"] = currentPuzzle.Name;
            DefaultViewModel["PageBackground"] = DrawCanvas.GetFirstColor();
            DefaultViewModel["BottomRowHeight"] = new GridLength(70);

            ResizeWindow((int)ActualWidth, (int)ActualHeight);

            if (!fe.CanSolve())
                ButtonSolve.Visibility = Visibility.Collapsed;

            _generatingGame = true;
            _isLoaded = false;

            //if (e.PageState)
            //	BeginActivatePuzzle(nullptr);
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            base.OnNavigatedFrom(e);
            Window.Current.CoreWindow.Dispatcher.AcceleratorKeyActivated -= OnAcceleratorKeyActivated;
            if (!_generatingGame && savingWorkItem?.IsCompleted != false)
                savingWorkItem = SaveToStorage();
        }

        private void OnGameCompleted(object sender, object e)
        {
            var settings = ApplicationData.Current.RoamingSettings.Values;
            if (settings["cfg_newgameprompt"] as bool? == false)
                return;

            var timer = new DispatcherTimer
            {
                Interval = TimeSpan.FromSeconds(0.7)
            };
            timer.Tick += async (s, d) =>
            {
                timer.Stop();

                var messageDialog = new MessageDialog("You have completed the puzzle!\n\n\n", "Congratulations");

                messageDialog.Commands.Add(new UICommand("New Game", null, PropertyValue.CreateInt32(0)));
                messageDialog.Commands.Add(new UICommand("Menu", null, PropertyValue.CreateInt32(1)));
                messageDialog.Commands.Add(new UICommand("Close", null, PropertyValue.CreateInt32(2)));

                messageDialog.DefaultCommandIndex = 0;
                messageDialog.CancelCommandIndex = 2;

                var command = await messageDialog.ShowAsync();
                if (command?.Id == null) return;

                int id = (int)command.Id;

                switch (id)
                {
                    case 0:
                        BeginNewGame();
                        break;
                    case 1:
                        Frame.GoBack();
                        break;
                }

            };
            timer.Start();
        }

        private void OnGenerationStart()
        {
            _generatingGame = true;
            BusyOverlay.Visibility = Visibility.Visible;
            BusyLabel.Visibility = Visibility.Visible;
            BusyIndicator.IsActive = true;
            _finishedOverlayAnimation = false;
            BusyOverlayDisappearingStoryboard.Stop();
            BusyOverlayAppearingStoryboard.Begin();
        }

        private void OnGenerationEnd()
        {
            _isLoaded = true;
            _controls = fe.GetVirtualButtonBar();

            VirtualButtonBar.Buttons = _controls;
            VirtualButtonBar.Visibility = _controls != null ? Visibility.Visible : Visibility.Collapsed;

            if (_controls.ToggleButton != null)
            {
                LeftRightLabel.Text = _controls.ToggleButton.Name;
                if (LeftRightPanel.Children.Count > 1)
                    LeftRightPanel.Children.RemoveAt(0);
                LeftRightPanel.Children.Insert(0, _controls.ToggleButton.Icon);
                ButtonLeftRight.Visibility = Visibility.Visible;
            }
            else
                ButtonLeftRight.Visibility = Visibility.Collapsed;

            bool isSwitched = ButtonLeftRight.IsChecked.Value;
            if (_controls.SwitchMiddle)
            {
                _leftAction = isSwitched ? _controls.MiddleAction : _controls.LeftAction;
                _middleAction = isSwitched ? _controls.LeftAction : _controls.MiddleAction;
                _rightAction = _controls.RightAction;
            }
            else
            {
                _leftAction = isSwitched ? _controls.RightAction : _controls.LeftAction;
                _rightAction = isSwitched ? _controls.LeftAction : _controls.RightAction;
                _middleAction = _controls.MiddleAction;
            }
            _touchAction = isSwitched ? _controls.HoldAction : _controls.TouchAction;
            _holdAction = isSwitched ? _controls.TouchAction : _controls.HoldAction;

            UpdateUndoButtons();
            ResizeWindow((int)ActualWidth, (int)ActualHeight);
            ResizeGame();

            _generatingGame = false;
            if (!_finishedOverlayAnimation)
            {
                BusyOverlayAppearingStoryboard.Stop();
                BusyOverlay.Visibility = Visibility.Collapsed;
            }
            else
            {
                BusyOverlayDisappearingStoryboard.Begin();
            }
            BusyIndicator.IsActive = false;
            BusyLabel.Visibility = Visibility.Collapsed;
            BusyCancelButton.Visibility = Visibility.Collapsed;

            if (_controls.ColorBlindKey != VirtualKey.None)
            {
                var settings = ApplicationData.Current.RoamingSettings.Values;
                var blindSetting = settings["cfg_colorblind"] as bool?;
                if (blindSetting == true)
                    fe.SendKey(_controls.ColorBlindKey, false, false);
                else if (blindSetting == null)
                    _ = DisplayColorblindPrompt();
            }

            _undoHotkey = _redoHotkey = true;
            foreach (var b in _controls.Buttons)
            {
                if (b.Key == VirtualKey.U)
                    _undoHotkey = false;
                if (b.Key == VirtualKey.R)
                    _redoHotkey = false;
            }

            var presets = fe.GetPresetList(true, 0);
            var menu = (MenuFlyout)PresetMenu.Flyout;
            menu.Items.Clear();
            foreach (var item in ItemsForPresetList(presets))
                menu.Items.Add(item);
            DrawCanvas.Focus(FocusState.Keyboard);
        }

        private async Task DisplayColorblindPrompt()
        {
            var settings = ApplicationData.Current.RoamingSettings.Values;
            var messageDialog = new MessageDialog("Colorblind Mode adds labels to each color, which makes the colors easier to differentiate.\n\nYou can change this later on the Settings screen.", "Enable Colorblind Mode?");

            messageDialog.Commands.Add(new UICommand("Use Colorblind Mode", c =>
            {
                if (_isLoaded && !_generatingGame)
                    fe.SendKey(_controls.ColorBlindKey, false, false);
                settings["cfg_colorblind"] = true;
            }));
            messageDialog.Commands.Add(new UICommand("Don't use", c => { settings["cfg_colorblind"] = false; }));

            messageDialog.DefaultCommandIndex = 0;
            messageDialog.CancelCommandIndex = 1;

            await messageDialog.ShowAsync();
        }

        void ResizeGame()
        {
            if (fe == null)
                return;

            int iw = (int)GameBorder.ActualWidth;
            int ih = (int)GameBorder.ActualHeight;

            float scale = DisplayInformation.GetForCurrentView().LogicalDpi / 96;

            fe.GetMaxSize((int)Math.Ceiling(iw * scale), (int)Math.Ceiling(ih * scale), out var ow, out var oh);

            DrawCanvas.Width = ow / scale;
            DrawCanvas.Height = oh / scale;

            if (ow > 20 && oh > 20)
                fe.Redraw(DrawCanvas, this, this, true);

            fe.SetInputScale(scale);
        }

        private void OnSettingChanged(object sender, SettingsChangedEventArgs e)
        {
            var key = e.Key;
            if (_controls != null && key == "cfg_colorblind" && _controls.ColorBlindKey != VirtualKey.None && !_generatingGame)
                fe.SendKey(_controls.ColorBlindKey, false, false);
            if (key == "env_MAP_VIVID_COLOURS" && _puzzleName == "Map" && _isLoaded)
            {
                DrawCanvas.RemoveColors();
                fe.ReloadColors();
                ForceRedraw();
            }
            if (key == "env_FIXED_PENCIL_MARKS" && _isLoaded)
                ForceRedraw();
        }

        private void OnResuming(object sender, object e)
        {
            StartTimer();
        }

        private async void OnSuspending(object sender, Windows.ApplicationModel.SuspendingEventArgs e)
        {
            EndTimer();
            if (!_generatingGame)
            {
                var cb = e.SuspendingOperation.GetDeferral();
                if (savingWorkItem?.IsCompleted != false)
                    savingWorkItem = SaveToStorage();
                await savingWorkItem;
                cb.Complete();
            }
        }

        private void OnUnloaded(object sender, RoutedEventArgs e)
        {
            Window.Current.CoreWindow.Dispatcher.AcceleratorKeyActivated -= OnAcceleratorKeyActivated;
            Window.Current.CoreWindow.VisibilityChanged -= OnVisibilityChanged;
            Window.Current.CoreWindow.Activated -= OnActivated;
            DataTransferManager.GetForCurrentView().DataRequested -= OnDataRequested;
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            Window.Current.CoreWindow.Dispatcher.AcceleratorKeyActivated += OnAcceleratorKeyActivated;
            Window.Current.CoreWindow.VisibilityChanged += OnVisibilityChanged;
            Window.Current.CoreWindow.Activated += OnActivated;
            DataTransferManager.GetForCurrentView().DataRequested += OnDataRequested;
        }

        public void ForceRedraw()
        {
            if (_generatingGame)
                return;

            fe.Redraw(DrawCanvas, this, this, true);
        }

        public async Task BeginLoadGame(StorageFile file)
        {
            if (_isLoaded && _generatingGame)
                return;

            BusyLabel.Text = "Loading game";
            OnGenerationStart();

            var err = await fe.LoadGameFromFile(file);
            await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
            {
                _generatingGame = false;
                if (string.IsNullOrEmpty(err))
                    OnGenerationEnd();
                else
                {
                    _ = new MessageDialog(err.AddPeriods(), "Could not load game").ShowAsync();
                    if (!_isLoaded)
                        _ = BeginActivatePuzzle(null);
                    else
                        OnGenerationEnd();
                }
            });
        }

        public async Task BeginResumeGame()
        {
            if (_isLoaded && _generatingGame)
                return;

            BusyLabel.Text = "Loading game";
            OnGenerationStart();
            BusyCancelButton.Visibility = Visibility.Collapsed;

            bool loaded = false;
            try
            {
                var settings = ApplicationData.Current.LocalSettings.Values;
                if (settings.ContainsKey(_puzzleName))
                {
                    var savedObj = settings[_puzzleName];
                    // If loading the game crashes for whatever reason, the safest bet is to not load it again
                    settings.Remove(_puzzleName);

                    var savedName = savedObj.ToString();

                    var file = await ApplicationData.Current.LocalFolder.GetFileAsync(savedName);
                    var err = fe.LoadGameFromString(await FileIO.ReadTextAsync(file));
                    loaded = string.IsNullOrEmpty(err);
                }
            }
            catch { }
            await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
            {
                _generatingGame = false;
                if (loaded)
                    OnGenerationEnd();
                else
                    BeginNewGame();
            });
        }

        CancellationTokenSource generatingWorkItem;

        public void BeginNewGame()
        {
            if (_isLoaded && _generatingGame)
                return;

            BusyLabel.Text = "Creating puzzle";
            OnGenerationStart();
            BusyCancelButton.Visibility = fe.IsCustomGame() ? Visibility.Visible : Visibility.Collapsed;

            generatingWorkItem = new CancellationTokenSource();
            fe.NewGame().AsTask(generatingWorkItem.Token).ContinueWith(async t =>
            {
                generatingWorkItem = null;
                var success = await t;
                await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
                {
                    if (success)
                        OnGenerationEnd();
                    else
                        Frame.GoBack();
                });
            });
        }

        public async Task BeginActivatePuzzle(GameLaunchParameters p)
        {
            if (_isLoaded && (_generatingGame || p == null))
                return;

            _generatingGame = false;

            if (p?.SaveFile != null)
                await BeginLoadGame(p.SaveFile);
            else if (_hasGame)
                await BeginResumeGame();
            else
                BeginNewGame();
        }

        private void ResizeWindow(int newWidth, int newHeight)
        {
            var buttonCount = VirtualButtonBar.Buttons?.Buttons.Count ?? 0;
            var idealButtonWidth = buttonCount > 4 ? 48 : 96;
            var minimumButtonWidth = buttonCount > 4 ? 40 : 80;
            int keyboardHeight;
            var availableWidth = newWidth - 240;

            if (buttonCount * minimumButtonWidth > newWidth - 32)
            {
                DefaultViewModel["KeyboardTwoRows"] = true;
                keyboardHeight = 90;
                buttonCount = (buttonCount + 1) / 2;
            }
            else
            {
                DefaultViewModel["KeyboardTwoRows"] = false;
                keyboardHeight = 50;
            }
            var keyboardWidth = Math.Min(newWidth - 32, buttonCount * idealButtonWidth);
            DefaultViewModel["KeyboardWidth"] = keyboardWidth;
            DefaultViewModel["BottomRowHeight"] = new GridLength(keyboardHeight + 20);

            if (availableWidth - keyboardWidth < 0)
                DefaultViewModel["LargePageUndoMargin"] = new Thickness(16, 0, 16, 16 + keyboardHeight);
            else
                DefaultViewModel["LargePageUndoMargin"] = new Thickness(16, 0, 16, 16);
        }

        void UpdateUndoButtons()
        {
            if (_generatingGame)
                return;

            ButtonUndo.IsEnabled = fe.CanUndo();
            ButtonRedo.IsEnabled = fe.CanRedo();
        }

        private void OnDataRequested(DataTransferManager sender, DataRequestedEventArgs args)
        {
            if (_generatingGame)
            {
                args.Request.FailWithDisplayText("The app is currently processing a puzzle.\n\nPlease try again when the operation has finished.");
                return;
            }

            var data = args.Request.Data;
            data.Properties.Title = "Saved Game for " + _puzzleName;
            data.Properties.Description = fe.CanUndo() ? "Share your progress in " + _puzzleName : "Share this " + _puzzleName + " puzzle";
            data.Properties.FileTypes.Clear();
            data.Properties.FileTypes.Add(".puzzle");
            data.SetDataProvider(StandardDataFormats.StorageItems, async request =>
            {
                var deferral = request.GetDeferral();

                var opened = await ApplicationData.Current.TemporaryFolder.CreateFileAsync(_puzzleName + ".puzzle", CreationCollisionOption.ReplaceExisting);
                var result = await fe.SaveGameToFile(opened);
                request.SetData(new List<StorageFile> { opened });
                deferral.Complete();
            });
        }

        private void OnVisibilityChanged(CoreWindow sender, VisibilityChangedEventArgs args)
        {
            if (_generatingGame || !_isLoaded)
                return;

            if (!args.Visible)
            {
                if (savingWorkItem?.IsCompleted != false)
                    savingWorkItem = SaveToStorage();
            }
            else
                ForceRedraw();
        }

        private void pageRoot_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            ResizeWindow((int)e.NewSize.Width, (int)e.NewSize.Height);
        }

        private void DoubleAnimation_Completed(object sender, object e)
        {
            _finishedOverlayAnimation = true;
        }

        private void BusyOverlayDisappearingAnimation_Completed(object sender, object e)
        {
            BusyOverlay.Visibility = Visibility.Collapsed;
        }

        private void GameBorder_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            if (_generatingGame)
                return;
            ResizeGame();
        }

        public void UpdateStatusBar(string status)
        {
            StatusBar.Text = status;
        }

        private DateTime LastTime;
        ThreadPoolTimer PeriodicTimer;
        public void StartTimer()
        {
            PeriodicTimer?.Cancel();
            var period = TimeSpan.FromMilliseconds(5);

            LastTime = DateTime.Now;

            PeriodicTimer = ThreadPoolTimer.CreatePeriodicTimer(timer =>
            {
                _ = Dispatcher.RunAsync(CoreDispatcherPriority.High, () =>
                {
                    var newtime = DateTime.Now;
                    var delta = newtime - LastTime;

                    if (delta == default) return;

                    LastTime = newtime;
                    if (!_generatingGame)
                        fe.UpdateTimer(DrawCanvas, this, this, (float)delta.TotalSeconds);
                });
            }, period);
        }

        public void EndTimer()
        {
            if (PeriodicTimer != null)
            {
                PeriodicTimer.Cancel();
                PeriodicTimer = null;
            }
        }

        private async Task SaveToStorage()
        {
            if (_generatingGame) return;
            
            var serialized = fe.SaveGameToString();

            try
            {
                var file = await ApplicationData.Current.LocalFolder.CreateFileAsync(_puzzleName + ".puzzle", CreationCollisionOption.ReplaceExisting);
                await FileIO.WriteTextAsync(file, serialized);
                ApplicationData.Current.LocalSettings.Values[_puzzleName] = _puzzleName + ".puzzle";
            }
            catch
            {
                /// TODO log non-fatal exception
            }
        }
    }
}
