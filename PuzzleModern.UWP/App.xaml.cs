using PuzzleCommon;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.ApplicationModel;
using Windows.ApplicationModel.Activation;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Storage;
using Windows.UI.Core;
using Windows.UI.StartScreen;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

namespace PuzzleModern.UWP
{
    /// <summary>
    /// Provides application-specific behavior to supplement the default Application class.
    /// </summary>
    sealed partial class App : Application
    {
        public static new App Current => (App)Application.Current;

        public event EventHandler<SettingsChangedEventArgs> SettingChanged;

        public void NotifySettingChanged(string key, object value)
        {
            SettingChanged?.Invoke(this, new SettingsChangedEventArgs
            {
                Key = key,
                Value = value
            });
        }

        /// <summary>
        /// Initializes the singleton application object.  This is the first line of authored code
        /// executed, and as such is the logical equivalent of main() or WinMain().
        /// </summary>
        public App()
        {
            this.InitializeComponent();
            this.Suspending += OnSuspending;
        }

        /// <summary>
        /// Invoked when the application is launched normally by the end user.  Other entry points
        /// will be used such as when the application is launched to open a specific file.
        /// </summary>
        /// <param name="e">Details about the launch request and process.</param>
        protected override void OnLaunched(LaunchActivatedEventArgs e)
        {
            if (!string.IsNullOrWhiteSpace(e.Arguments) && WindowsModern.IsValidPuzzleName(e.Arguments))
            {
                ActivatePuzzle(new GameLaunchParameters(e.Arguments));
            }
            // Open the selector page
            else
                ActivatePuzzle(null);
        }

        protected override async void OnFileActivated(FileActivatedEventArgs args)
        {
            var item = args.Files.FirstOrDefault();
            if (item == null || !item.IsOfType(StorageItemTypes.File)) return;

            await ActivateFile((StorageFile)item);
        }

        public async Task ActivateFile(StorageFile file)
        {
            var result = await WindowsModern.LoadAndIdentify(file);

            await Window.Current.Dispatcher.RunAsync(CoreDispatcherPriority.Normal, async () =>
            {
                var msg = result.Error;

                if (!string.IsNullOrEmpty(msg))
                {
                    ActivatePuzzle(null);
                    await new Windows.UI.Popups.MessageDialog(msg.AddPeriods(), "Could not load game").ShowAsync();
                }
                else
                {
                    ActivatePuzzle(result);
                }
            });
        }

        public void ActivatePuzzle(GameLaunchParameters launch)
        {
            Frame rootFrame = Window.Current.Content as Frame;
            GamePage page;

            // Do not repeat app initialization when the Window already has content,
            // just ensure that the window is active
            if (rootFrame == null)
            {
                // Create a Frame to act as the navigation context and associate it with
                // a SuspensionManager key
                rootFrame = new Frame();
                Window.Current.Content = rootFrame;

                // Set the default language
                rootFrame.Language = Windows.Globalization.ApplicationLanguages.Languages[0];
                rootFrame.NavigationFailed += OnNavigationFailed;

                if (rootFrame.Content == null)
                {
                    /* Always put the Selector page first on the navigation stack */
                    rootFrame.Navigate(typeof(MainPage), null);
                    if (launch != null)
                        rootFrame.Navigate(typeof(GamePage), launch.Name);
                }
            }
            else
            {
                if (rootFrame.Content == null)
                {
                    /* Always put the Selector page first on the navigation stack */
                    rootFrame.Navigate(typeof(MainPage), null);
                    if (launch != null)
                        rootFrame.Navigate(typeof(GamePage), launch.Name);
                }
                else if (launch != null)
                {
                    string puzzleName = launch.Name;
                    string currentName = null;
                    if (rootFrame.CurrentSourcePageType.Name == typeof(GamePage).Name)
                    {
                        page = (GamePage)rootFrame.Content;
                        currentName = (string)page.DefaultViewModel["PuzzleName"];
                    }
                    if (puzzleName != currentName)
                    {
                        /* Navigate back to the selector page */
                        while (rootFrame.CanGoBack)
                            rootFrame.GoBack();

                        rootFrame.Navigate(typeof(GamePage), launch.Name);
                    }
                }
            }

            // Ensure the current window is active
            Window.Current.Activate();
            page = rootFrame.Content as GamePage;
            page?.BeginActivatePuzzle(launch);

            if (launch?.Name != null)
            {
                foreach (var puzzle in WindowsModern.GetPuzzleList().Items)
                {
                    if (puzzle.Name == launch.Name)
                        UpdateJumpList(puzzle);
                }
            }
        }

        async void UpdateJumpList(Puzzle puzzle)
        {
            if (!JumpList.IsSupported()) return;

            var list = await JumpList.LoadCurrentAsync();
            list.SystemGroupKind = JumpListSystemGroupKind.None;

            var existing = list.Items.FirstOrDefault(i => i.Arguments == puzzle.Name);
            list.Items.Remove(existing);

            var item = JumpListItem.CreateWithArguments(puzzle.Name, puzzle.Name);
            item.Logo = puzzle.IconUri;
            item.GroupName = "Recent";
            list.Items.Insert(0, item);

            while (list.Items.Count > 8)
                list.Items.RemoveAt(list.Items.Count - 1);

            await list.SaveAsync();
        }

        /// <summary>
        /// Invoked when Navigation to a certain page fails
        /// </summary>
        /// <param name="sender">The Frame which failed navigation</param>
        /// <param name="e">Details about the navigation failure</param>
        void OnNavigationFailed(object sender, NavigationFailedEventArgs e)
        {
            throw new Exception("Failed to load Page " + e.SourcePageType.FullName);
        }

        /// <summary>
        /// Invoked when application execution is being suspended.  Application state is saved
        /// without knowing whether the application will be terminated or resumed with the contents
        /// of memory still intact.
        /// </summary>
        /// <param name="sender">The source of the suspend request.</param>
        /// <param name="e">Details about the suspend request.</param>
        private void OnSuspending(object sender, SuspendingEventArgs e)
        {
            var deferral = e.SuspendingOperation.GetDeferral();
            //TODO: Save application state and stop any background activity
            deferral.Complete();
        }
    }

    static class Exts
    {
        public static string AddPeriods(this string input)
        {
            if (input.LastOrDefault() != '.')
                return input + ".";
            return input;
        }

        public static bool IsDark(this Windows.UI.Color c)
        {
            return (5 * c.G + 2 * c.R + c.B) <= 8 * 128;
        }
    }

    public class SettingsChangedEventArgs
    {
        public string Key { get; set; }
        public object Value { get; set; }
    }
}
